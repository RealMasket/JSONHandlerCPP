# Preparing a release

The current release version is **1.0.0**. The version is defined once in
`CMakeLists.txt`; the public `MasketJSON/Version.hpp`, CMake version file,
CLI, and archive names are generated from it. Release preparation does not
automatically create a tag or publish a GitHub Release.

## Build and check binary archives

Run each linkage type in its own build directory. For a shared build:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DMASKETJSON_ENABLE_PACKAGING=ON
cmake --build build-release --config Release --parallel 2
ctest --test-dir build-release -C Release --output-on-failure
```

Repeat with `-DBUILD_SHARED_LIBS=OFF` and a different build directory for static
linkage. Google Test is needed for these checks but is not shipped. Boost/Qt
are optional benchmark dependencies and are not needed or shipped by these
commands. Sanitized builds are rejected for release packaging.

`Package.ArchiveConsumer` runs CPack, verifies SHA-256, extracts the archive,
relocates the prefix into a path containing spaces, and builds/runs an independent
consumer. It also runs the CLI from the extracted package. The resulting ZIP
(Windows) or tar.gz (Linux) and `.sha256` file are in `build-release/packages`.
The CI Release jobs run the same checks and preserve these files as workflow
artifacts for 14 days. Download the four tested shared/static Windows/Linux
artifacts from a successful run for the exact release commit.

For packaging an already checked build without re-running tests:

```sh
cpack --config build-release/CPackConfig.cmake -C Release
```

Check the toolchain encoded in each archive name. C++ binary compatibility is
limited to compatible compilers, standard libraries, and runtimes. Windows
archives do not bundle the MSVC redistributable. Prefer a source build when
toolchains differ. This release does not claim macOS validation.

## Freeze the source and review

1. Keep the project version, `CHANGELOG.md`, and `RELEASE_NOTES.md` consistent.
   Keep new work under `Unreleased`; each released version gets its own section.
2. Commit all release changes and wait for all CI jobs to pass on that exact SHA.
3. Generate a source archive from the verified commit, using Git's tracked files
   instead of archiving a working directory with local builds or dependencies:

   ```sh
   cmake -E make_directory out/release
   git archive --format=tar.gz --prefix=MasketJSON-1.0.0/ --output=out/release/MasketJSON-1.0.0-source.tar.gz VERIFIED_COMMIT_SHA
   cmake -E sha256sum out/release/MasketJSON-1.0.0-source.tar.gz
   ```

4. Extract that source archive outside the repository and run a clean build and
   CTest. An offline build can use `-DBUILD_TESTING=OFF`, or tests can use a local
   Google Test source through `FETCHCONTENT_SOURCE_DIR_GOOGLETEST`.
5. Review the license, notes, binary archives, source archive, checksums, and CI
   results before creating the annotated `v1.0.0` tag and GitHub Release.
   Use the verified SHA for the tag and `RELEASE_NOTES.md` as the release body.
   Attach the tested archives and their checksums. A GitHub draft allows one
   final review before making the release public.

After publication, record its date in `CHANGELOG.md`. For the next release,
increment the project version and assess API/ABI compatibility before changing
`SOVERSION` or the CMake package's major-version compatibility policy.
