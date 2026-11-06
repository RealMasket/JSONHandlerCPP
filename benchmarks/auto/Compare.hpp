#pragma once
#include "Token.hpp"
#include <string>
#include <vector>

namespace AutoCheck {
struct Outcome {
    bool accepted=false;
    std::vector<Token> tokens;
    std::string canonical, error;
    size_t line=0, column=0;
    unsigned errorKind=0;
};
Outcome baseline(const std::string& source, bool lowMemory);
bool compare(const std::string& source);
void selfTest();
}
