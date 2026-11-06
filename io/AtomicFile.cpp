#include "AtomicFile.hpp"
#include "Utf8.hpp"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace MasketJSONDetail {
namespace {
[[noreturn]] void fail(const char* action, const fs::path& path, std::error_code error)
{
    throw FileException(std::string(action) + ": " + path.u8string() + " (" + error.message() + ")");
}
#ifdef _WIN32
std::error_code lastError() { return {static_cast<int>(GetLastError()), std::system_category()}; }
HANDLE nativeHandle(std::intptr_t value) { return reinterpret_cast<HANDLE>(value); }
#else
std::error_code lastError() { return {errno, std::generic_category()}; }
#endif
}

fs::path filePath(const std::string& filename)
{
    if (filename.empty() || filename.find('\0') != std::string::npos)
        throw FileException("Expected a nonempty filename without NUL bytes");
#ifdef _WIN32
    if (!JsonDetail::isValidUtf8(filename))
        throw FileException("Windows filenames must be valid UTF-8");
    return fs::u8path(filename);
#else
    return fs::path(filename);
#endif
}

AtomicFile::AtomicFile(const std::string& filename)
{
    std::error_code error;
    target_ = fs::absolute(filePath(filename), error);
    if (error) fail("Unable to resolve atomic output path", filePath(filename), error);
    if (!target_.has_filename() || target_.filename() == "." || target_.filename() == "..")
        throw FileException("Atomic output requires a regular file path: " + filename);
#ifdef _WIN32
    if (target_.filename().native().find(L':') != std::wstring::npos)
        throw FileException("Atomic output does not support alternate data streams: " + filename);
#endif
}

void AtomicFile::inspectTarget(bool captureMode)
{
#ifdef _WIN32
    (void)captureMode;
    const DWORD attributes = GetFileAttributesW(target_.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const auto error = lastError();
        if (error.value() == ERROR_FILE_NOT_FOUND || error.value() == ERROR_PATH_NOT_FOUND) return;
        fail("Unable to inspect atomic output", target_, error);
    }
    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        throw FileException("Atomic output must not be a directory or reparse point: " + target_.u8string());
#else
    struct stat status{};
    if (::lstat(target_.c_str(), &status) != 0) {
        const auto error = lastError();
        if (error.value() == ENOENT) return;
        fail("Unable to inspect atomic output", target_, error);
    }
    if (!S_ISREG(status.st_mode))
        throw FileException("Atomic output must be a regular file, not a symlink or device: " + target_.native());
    if (captureMode) mode_ = static_cast<unsigned>(status.st_mode & 0777);
#endif
}

void AtomicFile::create()
{
    inspectTarget(true);
#ifdef _WIN32
    static std::atomic<unsigned long long> sequence{0};
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
        const auto leaf = L".masket-json-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
            std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(sequence.fetch_add(1)) + L".tmp";
        temporary_ = (target_.parent_path() / leaf).native();
        const HANDLE handle = CreateFileW(temporary_.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            handle_ = reinterpret_cast<std::intptr_t>(handle);
            ownsTemporary_ = true;
            return;
        }
        const auto error = lastError();
        if (error.value() != ERROR_FILE_EXISTS && error.value() != ERROR_ALREADY_EXISTS)
            fail("Unable to create atomic temporary file", target_, error);
    }
    throw FileException("Unable to obtain a unique atomic temporary file: " + target_.u8string());
#else
    temporary_ = (target_.parent_path() / ".masket-json-XXXXXX").native();
    const int descriptor = ::mkstemp(temporary_.data());
    if (descriptor == -1) fail("Unable to create atomic temporary file", target_, lastError());
    handle_ = descriptor;
    ownsTemporary_ = true;
#endif
}

size_t AtomicFile::write(std::string_view text)
{
    const size_t count = (std::min)(text.size(), size_t{1024 * 1024});
#ifdef _WIN32
    DWORD written = 0;
    if (!WriteFile(nativeHandle(handle_), text.data(), static_cast<DWORD>(count), &written, nullptr))
        fail("Unable to write atomic temporary file", target_, lastError());
    return written;
#else
    ssize_t written;
    do { written = ::write(static_cast<int>(handle_), text.data(), count); } while (written < 0 && errno == EINTR);
    if (written < 0) fail("Unable to write atomic temporary file", target_, lastError());
    return static_cast<size_t>(written);
#endif
}

void AtomicFile::flush()
{
#ifdef _WIN32
    if (!FlushFileBuffers(nativeHandle(handle_)))
        fail("Unable to flush atomic temporary file", target_, lastError());
#else
    if (::fchmod(static_cast<int>(handle_), static_cast<mode_t>(mode_)) != 0)
        fail("Unable to set atomic output permissions", target_, lastError());
    int result;
    do { result = ::fsync(static_cast<int>(handle_)); } while (result < 0 && errno == EINTR);
    if (result != 0) fail("Unable to flush atomic temporary file", target_, lastError());
#endif
}

void AtomicFile::close()
{
    const auto handle = handle_;
    handle_ = -1; // Never retry close(): on Linux even an error may release the fd.
#ifdef _WIN32
    if (!CloseHandle(nativeHandle(handle))) fail("Unable to close atomic temporary file", target_, lastError());
#else
    if (::close(static_cast<int>(handle)) != 0) fail("Unable to close atomic temporary file", target_, lastError());
#endif
}

void AtomicFile::commit()
{
    inspectTarget(false);
#ifdef _WIN32
    // A same-volume rename only: never fall back to copy/delete, and never
    // remove the destination before replacing it.
    if (!MoveFileExW(temporary_.c_str(), target_.c_str(), MOVEFILE_REPLACE_EXISTING))
        fail("Unable to replace atomic output", target_, lastError());
#else
    if (::rename(temporary_.c_str(), target_.c_str()) != 0)
        fail("Unable to replace atomic output", target_, lastError());
#endif
    ownsTemporary_ = false;
}

AtomicFile::~AtomicFile() noexcept
{
#ifdef _WIN32
    if (handle_ != -1) CloseHandle(nativeHandle(handle_));
    if (ownsTemporary_) DeleteFileW(temporary_.c_str());
#else
    if (handle_ != -1) ::close(static_cast<int>(handle_));
    if (ownsTemporary_) ::unlink(temporary_.c_str());
#endif
}

void writeTextAtomic(const std::string& filename, std::string_view text)
{
    AtomicFile file(filename);
    prepareAndCommit(file, text);
}
}
