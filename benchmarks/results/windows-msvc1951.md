# Local Release benchmark

Historical baseline: measured before exact integer storage, at source commit
[fd56159](https://github.com/RealMasket/JSONHandlerCPP/commit/fd56159e594f5726927da1f7ea7fdf6b5618824b).
These timings do not describe later revisions of the numeric representation.

Measured on 2026-11-05 on Windows x64, Intel Core i5-12600KF.
Compiler: MSVC 19.51 (1951), Release; JSONHandler and Boost.JSON built as static
libraries in the same CMake build. Qt Core is the prebuilt MSVC 2022 x64 kit.
Versions: JSONHandlerCPP 1.0, Boost.JSON 1.88.0, Qt 6.11.0.

Each row is the median of 9 samples, with 100 iterations per sample and
5 warm-up calls per operation. Times below are microseconds per operation.

| Dataset | Operation | JSONHandler | Boost.JSON | Qt |
| --- | --- | ---: | ---: | ---: |
| tiny.json | parse | 2.023 | 0.410 | 0.775 |
| tiny.json | serialize | 1.250 | 0.945 | 1.123 |
| small.json | parse | 238.467 | 44.936 | 88.238 |
| small.json | serialize | 50.205 | 8.560 | 589.311 |
| medium.json | parse | 2,617.965 | 583.986 | 1,222.829 |
| medium.json | serialize | 780.601 | 136.926 | 1,654.455 |
| large.json | parse | 26,679.946 | 4,745.354 | 10,294.581 |
| large.json | serialize | 6,778.695 | 1,954.859 | 13,752.296 |
| nested.json | parse | 20.229 | 11.149 | 18.984 |
| nested.json | serialize | 13.457 | 0.850 | 64.531 |

For the 1,136,469-byte dataset in this run:

- Parsing took 26.68 ms in JSONHandler, 4.75 ms in Boost.JSON, and 10.29 ms in Qt.
- Serialization took 6.78 ms in JSONHandler, 1.95 ms in Boost.JSON, and 13.75 ms in Qt.

These are local observations, not general library rankings. The machine was
not isolated from desktop activity. Library order is fixed, tiny samples are
sensitive to timer/call overhead, and Qt's prebuilt optimization settings
differ. Numbers are in the shared representable domain, duplicate keys are
absent, and all serialized values passed a semantic round-trip check. No
custom allocators, parser reuse, streaming, or peak-memory measurements were used.

Read the [methodology and reproduction commands](../README.md) and the
[raw CSV with sample ranges](windows-msvc1951.csv) before comparing results.
