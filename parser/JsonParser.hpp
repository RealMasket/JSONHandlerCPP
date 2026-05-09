#pragma once
#include "../core/JsonHandler.hpp"

class JSON_API JsonParser
{
public:
    static Json parse(const std::string& str);
};

