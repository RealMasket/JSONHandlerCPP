#include "JsonHandler.hpp"
#include "JsonParser.hpp"
#include "BenchmarkUtils.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

std::string loadFile(const std::string& path)
{
    std::ifstream file(path);

    std::stringstream buffer;

    buffer << file.rdbuf();

    return buffer.str();
}

int main()
{
    size_t iterations = 100;
	for (int i = 0; i < 5; ++i)
	{
        std::string fileName;
		switch (i)
		{
		    case 0: fileName = "smallFile.json"; break;
		    case 1: fileName = "mediumFile.json"; break;
		    case 2: fileName = "bigFile.json"; break;
		    case 3: fileName = "largeFile.json"; iterations = 10; break;
		    case 4: fileName = "hugeFile.json"; break;
		}
        std::string json =
            loadFile("benchmark_data/" + fileName);

        double time = BenchmarkUtils::measureExecutionTime(
            [&]()
            {
                Json parsed = JsonParser::parse(json);
            },
            iterations
        );

        BenchmarkUtils::printResult(
            "JSON Parse Benchmark",
            time,
            iterations,
            fileName
        );
	}

	std::cin.get();
    return 0;
}