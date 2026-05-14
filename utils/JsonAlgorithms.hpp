#pragma once
#include "JsonHandler.hpp"

class JSON_API JsonAlgorithms
{
public:
    /**
     * @brief Merges another JSON object into the current object.
     * (Existing keys are overwritten based on the overwrite parameter.)
     * @param target The target JSON object to merge into.
     * @param other The JSON object to merge.
     * @param overwrite If true, existing keys will be overwritten.
     * @throws TypeException if the current value is not an object.
     */
    static void mergeObject(Json& target, const Json& other, bool overwrite);
    /**
     * @brief Merges another JSON array into the current array.
     * @param target The target JSON array to merge into.
     * @param other The JSON array to merge.
     * @throws TypeException if the current value is not an array.
     */
    static void mergeArray(Json& target, const Json& other);
    /**
     * @brief Sorts a JSON array of objects by a nested key path (e.g., "Marks.MathMark").
     * @param target The target JSON array to sort.
     * @param keyPath The key path to sort by (e.g., "Marks.MathMark").
     * @param ascending If true, sorts in ascending order; otherwise, descending.
     * @throws TypeException if the JSON value is not an array of objects.
     * @throws AccessException if the key path does not exist in the objects.
     */
    static void sortByPath(Json& target, const std::string& keyPath, bool ascending);
};

