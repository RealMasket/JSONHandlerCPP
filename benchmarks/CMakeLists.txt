add_executable(ParseBenchmark
    ParseBenchmark.cpp
    BenchmarkUtils.cpp
)

target_link_libraries(ParseBenchmark
    PRIVATE
    JsonHandlerCPP
)