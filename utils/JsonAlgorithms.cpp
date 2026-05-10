#include "JsonAlgorithms.hpp"
#include "TypeException.hpp"
#include "AccessException.hpp"
#include "OperationException.hpp"
#include "JsonHandler.hpp"
#include "pch.h"

/**
 * @brief Merges another JSON object into the current object.
 * (Existing keys are overwritten based on the overwrite parameter.)
 * @param target The target JSON object to merge into.
 * @param other The JSON object to merge.
 * @param overwrite If true, existing keys will be overwritten.
 * @throws TypeException if the current value is not an object.
 */
void JsonAlgorithms::mergeObject(Json& target, const Json& other, bool overwrite) {
    if (!target.isObject()) throw TypeException("Expected JSON object but got " + target.typeName());
	auto& obj = target.asObject();
    for (const auto& [key, value] : other.asObject()) {
        if (overwrite || obj.find(key) == obj.end()) {
            obj.insert_or_assign(key, value);
        }
    }
}

/**
 * @brief Merges another JSON array into the current array.
 * @param target The target JSON array to merge into.
 * @param other The JSON array to merge.
 * @throws TypeException if the current value is not an array.
 */
void JsonAlgorithms::mergeArray(Json& target, const Json& other) {
    if (!target.isArray()) throw TypeException("Expected JSON array but got " + target.typeName());
    auto& arr = target.asArray();
    const auto& otherArr = other.asArray();
    std::copy(otherArr.cbegin(), otherArr.cend(), std::back_inserter(arr));
}

/**
 * @brief Sorts a JSON array of objects by a nested key path (e.g., "Marks.MathMark").
 * @param target The target JSON array to sort.
 * @param keyPath The key path to sort by (e.g., "Marks.MathMark").
 * @param ascending If true, sorts in ascending order; otherwise, descending.
 * @throws TypeException if the JSON value is not an array of objects.
 * @throws AccessException if the key path does not exist in the objects.
 */
void JsonAlgorithms::sortByPath(Json& target, const std::string & keyPath, bool ascending) {
    if (!target.isArray()) throw TypeException("Expected JSON array but got " + target.typeName());

    // Split the keyPath into individual keys
    std::vector<std::string> keys;
    size_t start = 0, end;
    while ((end = keyPath.find('.', start)) != std::string::npos) {
        keys.push_back(keyPath.substr(start, end - start));
        start = end + 1;
    }
    keys.push_back(keyPath.substr(start));

    // Lambda for accessing a nested value by key path
    auto getValueByPath = [&keys](const Json& json) -> const Json& {
        const Json* current = &json;
        for (const auto& key : keys) {
            if (!current->isObject() || !current->asObject().count(key)) {
                throw AccessException("Invalid key path: " + key);
            }
            current = &current->asObject().at(key);
        }
        return *current;
        };

    auto& arr = target.asArray();
    // Perform sorting
    std::sort(arr.begin(), arr.end(), [&](const Json& a, const Json& b) {
        const Json& valA = getValueByPath(a);
        const Json& valB = getValueByPath(b);

        if (valA.isNumber() && valB.isNumber()) {
            return ascending ? valA.asNumber() < valB.asNumber() : valA.asNumber() > valB.asNumber();
        }
        if (valA.isString() && valB.isString()) {
            return ascending ? valA.asString() < valB.asString() : valA.asString() > valB.asString();
        }

        throw OperationException("Cannot compare values of different types");
        });
}