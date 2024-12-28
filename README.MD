# JsonHandlerC++ Library

This is a custom JSON handling library created for personal use in future projects. It provides a simple API for parsing, manipulating, and serializing JSON data. The library is written in C++ and uses modern C++ features like `std::variant` and `std::map` to handle different JSON data types.

## Features

- Supports `null`, `bool`, `number`, `string`, `object`, and `array` JSON types.
- Ability to serialize C++ objects into JSON format and deserialize JSON back into C++ objects.
- Access JSON data via index (`[]`) for arrays and keys for objects.
- Push new elements to arrays with `push_back`.
- Error handling with exceptions for invalid operations or JSON structure.

## Usage

To use this library, include the header file `jsonHandlerC++.hpp` and link the compiled library to your project.

### Example
#### Basic JSON:
```
#include "jsonHandlerC++.hpp"
#include <iostream>

int main() {
    // Create a JSON object
    Json obj(Json::JsonObject{});

    // Add data to the object
    obj["name"] = "Adam";
    obj["age"] = 21.0;

    // Serialize the JSON object to a string
    std::cout << obj.serialize() << std::endl;

    return 0;
}
```

#### Example of Arrays:

```
#include "jsonHandlerC++.hpp"
#include <iostream>

int main() {
    // Create a JSON array
    Json::JsonArray jsonArray = { Json(1.0), Json(2.0), Json(3.5), Json("hello"), Json(true) };

    // Wrap the array in a Json object
    Json json = jsonArray;

    // Access elements in the JSON array
    std::cout << "First element: " << json[0].asNumber() << std::endl;
    std::cout << "Second element: " << json[1].asNumber() << std::endl;
    std::cout << "Third element: " << json[2].asNumber() << std::endl;
    std::cout << "Fourth element: " << json[3].asString() << std::endl;
    std::cout << "Fifth element: " << std::boolalpha << json[4].asBool() << std::endl;

    // Modify an element in the array
    json[1] = Json(42.5);

    // Add new elements to the array
    json.push_back(Json("new element"));
    json.push_back(Json(false));

    // Serialize the array
    std::cout << "Serialized JSON array: " << json.serialize(true, 0) << std::endl;

    // Parse a JSON array from a string
    std::string jsonString = "[10, 20, 30, \"world\", false]";
    Json parsedJson = Json::parse(jsonString);
    std::cout << "Parsed JSON array: " << parsedJson.serialize(true, 0) << std::endl;

    // Iterate through the array
    std::cout << "Iterating through the JSON array:" << std::endl;
    for (size_t i = 0; i < parsedJson.asArray().size(); ++i) {
        const Json& element = parsedJson[i];
        if (element.isNumber()) {
            std::cout << "Number: " << element.asNumber() << std::endl;
        }
        else if (element.isString()) {
            std::cout << "String: " << element.asString() << std::endl;
        }
        else if (element.isBool()) {
            std::cout << "Boolean: " << element.asBool() << std::endl;
        }
    }

    return 0;
}
```

### Supported Operations

- **Creating JSON objects**:
    - `Json(JsonObject)` - Creates a JSON object.
    - `Json(JsonArray)` - Creates a JSON array.
    - `Json(std::string)` - Creates a JSON string.
    - `Json(double)` - Creates a JSON number.
    - `Json(bool)` - Creates a JSON boolean.

- **Accessing JSON data**:
    - Use the `operator[]` to access data in objects and arrays.
    - `obj["key"]` for objects.
    - `obj[index]` for arrays.

- **Parsing JSON**:
    - `Json::parse(const std::string& str)` - Parses a JSON string into a `Json` object.

- **Serialization**:
    - `obj.serialize()` - Serializes a `Json` object to a JSON-formatted string.

### Error Handling

The library uses C++ exceptions (`std::runtime_error`) to signal errors like:
- Attempting to access an invalid index or key.
- Invalid JSON format during parsing.
- Type mismatches when converting between types.

## Building the Library

1. **Create the DLL**: Follow the instructions for creating a DLL in Visual Studio, linking the library with your project.
2. **Link the Library**: Add `jsonHandlerC++.hpp` to your project and link against the compiled DLL or static library.

### Dependencies

- C++17 or later for `std::variant` and other features.

## License

This library is intended for personal use only and is not distributed for commercial purposes. Feel free to use it in your projects.

## TODO

- Add more comprehensive error handling.
- Improve performance for larger JSON files.
- Expand API functionality for more complex operations.