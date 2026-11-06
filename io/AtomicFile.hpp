#pragma once

// Private implementation shared by the library, CLI, and fault-injection tests.
// None of these declarations are part of the installed public API.
#include "FileException.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

#if defined(__GNUC__) && !defined(_WIN32)
#define MASKETJSON_IO_PRIVATE __attribute__((visibility("hidden")))
#else
#define MASKETJSON_IO_PRIVATE
#endif

namespace MasketJSONDetail {
MASKETJSON_IO_PRIVATE std::filesystem::path filePath(const std::string& filename);

class MASKETJSON_IO_PRIVATE AtomicFile {
public:
    explicit AtomicFile(const std::string& filename);
    ~AtomicFile() noexcept;
    AtomicFile(const AtomicFile&) = delete;
    AtomicFile& operator=(const AtomicFile&) = delete;

    void create();
    size_t write(std::string_view text);
    void flush();
    void close();
    void commit();

private:
    void inspectTarget(bool captureMode);
    std::filesystem::path target_;
    std::filesystem::path::string_type temporary_;
    std::intptr_t handle_ = -1;
    bool ownsTemporary_ = false;
    unsigned mode_ = 0600;
};

// Keep ordering and short-write handling testable without exporting test hooks
// or changing the process-wide behavior of the real filesystem implementation.
template<class File>
void prepareAndCommit(File& file, std::string_view text)
{
    file.create();
    while (!text.empty()) {
        const size_t count = file.write(text);
        if (count == 0 || count > text.size())
            throw FileException("Atomic output write made no valid progress");
        text.remove_prefix(count);
    }
    file.flush();
    file.close();
    file.commit();
}

MASKETJSON_IO_PRIVATE void writeTextAtomic(const std::string& filename, std::string_view text);
}

#undef MASKETJSON_IO_PRIVATE
