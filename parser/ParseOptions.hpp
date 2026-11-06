#pragma once

enum class ParsePreset {
    Balanced,
    LowMemory
};

struct ParseOptions {
    // Balanced retains the existing reservation policy. LowMemory bounds
    // speculative token capacity; it is not a total-memory limit and may copy more.
    ParsePreset preset = ParsePreset::Balanced;
};
