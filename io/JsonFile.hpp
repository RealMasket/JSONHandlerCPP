#pragma once
#include "JsonHandler.hpp"

class JsonFile
{
public:
	/**
	 * @brief Loads JSON data from a file and replaces the current value.
	 * @param filename The name of the file to load from.
	 * @throws std::runtime_error if the file cannot be opened or contains invalid JSON.
	 */
	static Json read(const std::string& filename);

	/**
	 * @brief Saves the JSON value to a file.
	 * @param filename The name of the file to save to.
	 * @throws std::runtime_error if the file cannot be opened.
	 */
	static void write(const std::string& filename, const Json& json);
};

