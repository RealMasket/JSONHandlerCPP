#pragma once
#include "../core/JsonHandler.hpp"

class JsonAlgorithms
{
public:
    static void mergeObject(Json& target, const Json& other, bool overwrite);
    static void mergeArray(Json& target, const Json& other);
    static void sortByPath(Json& target, const std::string& keyPath, bool ascending);
};

