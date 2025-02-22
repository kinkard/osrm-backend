#include "engine/polyline_compressor.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdlib>

namespace osrm::engine::detail // anonymous to keep TU local
{

void encode(int number_to_encode, std::string &output)
{
    if (number_to_encode < 0)
    {
        const unsigned binary = std::llabs(number_to_encode);
        const unsigned twos = (~binary) + 1u;
        const unsigned shl = twos << 1u;
        number_to_encode = static_cast<int>(~shl);
    }
    else
    {
        number_to_encode <<= 1u;
    }
    while (number_to_encode >= 0x20)
    {
        const int next_value = (0x20 | (number_to_encode & 0x1f)) + 63;
        output += static_cast<char>(next_value);
        number_to_encode >>= 5;
    }

    number_to_encode += 63;
    output += static_cast<char>(number_to_encode);
}

// https://developers.google.com/maps/documentation/utilities/polylinealgorithm
std::int32_t decode_polyline_integer(std::string::const_iterator &first,
                                     std::string::const_iterator last)
{
    // varint coding parameters
    const std::uint32_t bits_in_chunk = 5;
    const std::uint32_t continuation_bit = 1 << bits_in_chunk;
    const std::uint32_t chunk_mask = (1 << bits_in_chunk) - 1;

    std::uint32_t result = 0;
    for (std::uint32_t value = continuation_bit, shift = 0;
         (value & continuation_bit) && (shift < CHAR_BIT * sizeof(result) - 1) && first != last;
         ++first, shift += bits_in_chunk)
    {
        value = *first - 63; // convert ASCII coding [?..~] to an integer [0..63]
        result |= (value & chunk_mask) << shift;
    }

    // change "zig-zag" sign coding to two's complement
    result = ((result & 1) == 1) ? ~(result >> 1) : (result >> 1);
    return static_cast<std::int32_t>(result);
}
} // namespace osrm::engine::detail
