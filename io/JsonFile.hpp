#pragma once
#include "../core/JsonHandler.hpp"

class JsonFile
{
public:
	static Json read(const std::string& filename);
	static void write(const std::string& filename, const Json& json);
};

