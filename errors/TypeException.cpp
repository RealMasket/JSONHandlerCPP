#include "TypeException.hpp"

TypeException::TypeException(const std::string& message) : JsonException( "Type error: " + message )
{
}