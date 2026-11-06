#include <gtest/gtest.h>
#include "JsonFile.hpp"
#include "AtomicFile.hpp"
#include "OperationException.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;
namespace {
std::string bytes(const fs::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

class AtomicFileTests : public testing::Test {
protected:
    fs::path base;
    fs::path dir;
    bool owned = false;
    void SetUp() override {
        base = fs::absolute(fs::temp_directory_path());
        for (unsigned i = 0; i < 64 && !owned; ++i) {
            dir = base / ("masket-json-atomic-tests-" + std::to_string(std::random_device{}()));
            owned = fs::create_directory(dir);
        }
        ASSERT_TRUE(owned);
    }
    void TearDown() override {
        // Only recursively remove this fixture's newly created, direct child.
        if (owned && dir.parent_path() == base && !fs::is_symlink(dir)) {
            std::error_code error;
            fs::permissions(dir, fs::perms::owner_all, fs::perm_options::add, error);
            fs::remove_all(dir, error);
        }
    }
    void expectNoTemporaryFiles() {
        for (const auto& entry : fs::directory_iterator(dir))
            EXPECT_NE(entry.path().filename().u8string().find(".masket-json-"), size_t{0})
                << entry.path().u8string();
    }
};

TEST_F(AtomicFileTests, CreatesAndReplacesWithSelectedFormatting)
{
    const auto path = dir / "value.json";
    const Json value(Json::JsonObject{{"id", "001"}, {"value", 42}});
    JsonFile::writeAtomic(path.u8string(), value);
    EXPECT_EQ(bytes(path), value.serialize(true));
    JsonFile::writeAtomic(path.u8string(), value, false);
    EXPECT_EQ(bytes(path), value.serialize(false));
    EXPECT_EQ(JsonFile::read(path.u8string()), value);
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, UnicodePathsWorkAcrossReadWriteAndAtomicReplacement)
{
    const auto folder = dir / fs::u8path(u8"\u043f\u0430\u043f\u043a\u0430 \u6570\u636e");
    ASSERT_TRUE(fs::create_directory(folder));
    const auto path = folder / fs::u8path(u8"\u043d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0438 \U0001f600.json");
    JsonFile::write(path.u8string(), "original");
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    JsonFile::writeAtomic(path.u8string(), "updated");
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("updated"));
    EXPECT_EQ(std::distance(fs::directory_iterator(folder), fs::directory_iterator()), 1);
}

TEST_F(AtomicFileTests, SerializationFailureDoesNotTouchDestinationOrCreateTemporary)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    const auto original = bytes(path);
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string(), std::numeric_limits<double>::infinity()), OperationException);
    EXPECT_EQ(bytes(path), original);
    EXPECT_THROW(JsonFile::writeAtomic((dir / "new.json").u8string(), std::numeric_limits<double>::quiet_NaN()),
        OperationException);
    EXPECT_FALSE(fs::exists(dir / "new.json"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, InvalidPathsFailWithoutTruncatingAnExistingPrefix)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    EXPECT_THROW(JsonFile::writeAtomic("", 1), FileException);
    EXPECT_THROW(JsonFile::writeAtomic((dir / ".").u8string(), 1), FileException);
    EXPECT_THROW(JsonFile::writeAtomic((dir / "missing" / "value.json").u8string(), 1), FileException);
    const auto nulPath = path.u8string() + std::string("\0suffix", 7);
    EXPECT_THROW(JsonFile::writeAtomic(nulPath, 1), FileException);
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, DoesNotReplaceDirectories)
{
    const auto target = dir / "directory.json";
    ASSERT_TRUE(fs::create_directory(target));
    JsonFile::write((target / "marker.json").u8string(), "original");
    EXPECT_THROW(JsonFile::writeAtomic(target.u8string(), 1), FileException);
    EXPECT_EQ(JsonFile::read((target / "marker.json").u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, RejectsExistingAndDanglingSymlinks)
{
    const auto original = dir / "original.json";
    const auto link = dir / "link.json";
    JsonFile::write(original.u8string(), "original");
    std::error_code error;
    fs::create_symlink(original, link, error);
    if (error) GTEST_SKIP() << "Symlink creation unavailable: " << error.message();
    EXPECT_THROW(JsonFile::writeAtomic(link.u8string(), "updated"), FileException);
    EXPECT_TRUE(fs::is_symlink(link));
    EXPECT_EQ(JsonFile::read(original.u8string()), Json("original"));
    const auto dangling = dir / "dangling.json";
    fs::create_symlink(dir / "absent.json", dangling);
    EXPECT_THROW(JsonFile::writeAtomic(dangling.u8string(), 1), FileException);
    EXPECT_TRUE(fs::is_symlink(dangling));
    EXPECT_FALSE(fs::exists(dir / "absent.json"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, ReplacementDoesNotWriteThroughOtherHardLinks)
{
    const auto target = dir / "value.json";
    const auto other = dir / "other.json";
    JsonFile::write(target.u8string(), "original");
    std::error_code error;
    fs::create_hard_link(target, other, error);
    if (error) GTEST_SKIP() << "Hard links unavailable: " << error.message();
    JsonFile::writeAtomic(target.u8string(), "updated");
    EXPECT_EQ(JsonFile::read(target.u8string()), Json("updated"));
    EXPECT_EQ(JsonFile::read(other.u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, HandlesDocumentsLargerThanNativeWriteChunks)
{
    const auto path = dir / "large.json";
    const Json value(std::string(3 * 1024 * 1024 + 7, 'x'));
    JsonFile::writeAtomic(path.u8string(), value, false);
    EXPECT_EQ(bytes(path), value.serialize(false));
    EXPECT_EQ(JsonFile::read(path.u8string()), value);
    expectNoTemporaryFiles();
}

enum class Failure { None, Write, ZeroWrite, Flush, Close, Commit };
struct FaultFile {
    MasketJSONDetail::AtomicFile real;
    Failure failure;
    unsigned writes = 0;
    FaultFile(const fs::path& path, Failure stage) : real(path.u8string()), failure(stage) {}
    void create() { real.create(); }
    size_t write(std::string_view text) {
        if (failure == Failure::ZeroWrite) return 0;
        if (failure == Failure::Write && writes != 0) throw FileException("Injected partial-write failure");
        ++writes;
        return real.write(text.substr(0, 3));
    }
    void flush() {
        if (failure == Failure::Flush) throw FileException("Injected flush failure");
        real.flush();
    }
    void close() {
        real.close();
        if (failure == Failure::Close) throw FileException("Injected close failure");
    }
    void commit() {
        if (failure == Failure::Commit) throw FileException("Injected commit failure");
        real.commit();
    }
};

TEST_F(AtomicFileTests, RepeatsShortWritesUntilAllBytesAreWritten)
{
    const auto path = dir / "value.json";
    const std::string text = "{\"id\":\"001\",\"text\":\"all bytes survive\"}";
    {
        FaultFile file(path, Failure::None);
        MasketJSONDetail::prepareAndCommit(file, text);
        EXPECT_GT(file.writes, 1u);
    }
    EXPECT_EQ(bytes(path), text);
    expectNoTemporaryFiles();
}

class AtomicFailureTests : public AtomicFileTests, public testing::WithParamInterface<Failure> {};
TEST_P(AtomicFailureTests, PreservesOriginalAndRemovesTemporaryFile)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    const auto original = bytes(path);
    {
        FaultFile file(path, GetParam());
        EXPECT_THROW(MasketJSONDetail::prepareAndCommit(file, "{\"updated\":true}"), FileException);
    }
    EXPECT_EQ(bytes(path), original);
    expectNoTemporaryFiles();
}
INSTANTIATE_TEST_SUITE_P(EachStage, AtomicFailureTests,
    testing::Values(Failure::Write, Failure::ZeroWrite, Failure::Flush, Failure::Close, Failure::Commit),
    [](const testing::TestParamInfo<Failure>& parameter) {
        switch (parameter.param) {
        case Failure::Write: return "PartialWrite";
        case Failure::ZeroWrite: return "ZeroWrite";
        case Failure::Flush: return "Flush";
        case Failure::Close: return "Close";
        default: return "Commit";
        }
    });

TEST_P(AtomicFailureTests, DoesNotCreateAnEmptyDestinationOnFailure)
{
    const auto path = dir / "absent.json";
    {
        FaultFile file(path, GetParam());
        EXPECT_THROW(MasketJSONDetail::prepareAndCommit(file, "{\"updated\":true}"), FileException);
    }
    EXPECT_FALSE(fs::exists(path));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, CleanupDoesNotRemoveUnrelatedTemporaryFiles)
{
    const auto unrelated = dir / ".masket-json-existing.tmp";
    JsonFile::write(unrelated.u8string(), "unrelated");
    {
        FaultFile file(dir / "absent.json", Failure::Flush);
        EXPECT_THROW(MasketJSONDetail::prepareAndCommit(file, "{\"updated\":true}"), FileException);
    }
    EXPECT_EQ(JsonFile::read(unrelated.u8string()), Json("unrelated"));
    EXPECT_EQ(std::distance(fs::directory_iterator(dir), fs::directory_iterator()), 1);
}

#ifdef _WIN32
TEST_F(AtomicFileTests, LockedDestinationSurvivesNativeReplacementFailure)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    const HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(handle, INVALID_HANDLE_VALUE);
    struct Guard { HANDLE handle; ~Guard() { CloseHandle(handle); } } guard{handle};
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string(), "updated"), FileException);
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, ReadOnlyDestinationSurvivesReplacementFailure)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    const DWORD attributes = GetFileAttributesW(path.c_str());
    ASSERT_NE(attributes, INVALID_FILE_ATTRIBUTES);
    ASSERT_TRUE(SetFileAttributesW(path.c_str(), attributes | FILE_ATTRIBUTE_READONLY));
    struct Guard {
        const fs::path& path;
        DWORD attributes;
        ~Guard() { SetFileAttributesW(path.c_str(), attributes); }
    } guard{path, attributes};
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string(), "updated"), FileException);
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, RejectsInvalidUtf8AndAlternateDataStreamPaths)
{
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string() + ":stream", 1), FileException);
    EXPECT_THROW(JsonFile::writeAtomic((dir / "bad").u8string() + "\xc0\xaf", 1), FileException);
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}
#else
TEST_F(AtomicFileTests, RejectsFifosWithoutOpeningThem)
{
    const auto path = dir / "pipe.json";
    ASSERT_EQ(::mkfifo(path.c_str(), 0600), 0);
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string(), 1), FileException);
    EXPECT_TRUE(fs::is_fifo(path));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, PreservesBasicPermissionsAndCreatesPrivateNewFiles)
{
    const auto path = dir / "value.json";
    JsonFile::writeAtomic(path.u8string(), 1);
    EXPECT_EQ(fs::status(path).permissions() & fs::perms::mask,
        fs::perms::owner_read | fs::perms::owner_write);
    const auto mode = fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read;
    fs::permissions(path, mode);
    JsonFile::writeAtomic(path.u8string(), 2);
    EXPECT_EQ(fs::status(path).permissions() & fs::perms::mask, mode);
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, UnwritableDirectoryPreservesOriginal)
{
    if (::geteuid() == 0) GTEST_SKIP() << "Root bypasses directory permissions";
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    fs::permissions(dir, fs::perms::owner_read | fs::perms::owner_exec);
    EXPECT_THROW(JsonFile::writeAtomic(path.u8string(), "updated"), FileException);
    fs::permissions(dir, fs::perms::owner_all);
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}

TEST_F(AtomicFileTests, NativeRenameFailurePreservesOriginalAndCleansUp)
{
    if (::geteuid() == 0) GTEST_SKIP() << "Root bypasses directory permissions";
    const auto path = dir / "value.json";
    JsonFile::write(path.u8string(), "original");
    struct DenyRename : FaultFile {
        fs::path parent;
        explicit DenyRename(const fs::path& path) : FaultFile(path, Failure::None), parent(path.parent_path()) {}
        void close() {
            FaultFile::close();
            fs::permissions(parent, fs::perms::owner_read | fs::perms::owner_exec);
        }
    };
    {
        DenyRename file(path);
        EXPECT_THROW(MasketJSONDetail::prepareAndCommit(file, "{\"updated\":true}"), FileException);
        fs::permissions(dir, fs::perms::owner_all); // Restore before the RAII cleanup.
    }
    EXPECT_EQ(JsonFile::read(path.u8string()), Json("original"));
    expectNoTemporaryFiles();
}
#endif
}
