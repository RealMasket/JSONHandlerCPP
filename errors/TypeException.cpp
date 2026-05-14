#include "TypeException.hpp"
#include "pch.h"

TypeException::TypeException(const std::string& message) : JsonException( "Type error: " + message )
{
}