#pragma once
#include <algorithm>
#include <cstddef>

namespace MasketJSONDetail {
// Internal allocation hint. Called only when a token vector is full, with
// capacity <= limit and a nonnegative density estimate. No extra source scan.
inline size_t lowMemoryCapacity(size_t capacity, size_t limit, double estimate) noexcept
{
    const size_t available = limit - capacity;
    const size_t minimum = (std::max)(capacity / 8, size_t{1});
    const size_t lower = capacity + (std::min)(minimum, available);
    const size_t spare = (std::max)(capacity, size_t{256});
    const size_t upper = capacity + (std::min)(spare, available);
    return estimate >= static_cast<double>(upper) ? upper
        : (std::min)(upper, (std::max)(lower, static_cast<size_t>(estimate)));
}
}
