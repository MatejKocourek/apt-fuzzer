#include <rapidcheck.h>
#include "fuzzer.h"
#include <string_view>
#include <vector>
#include <algorithm>

void mutatorInsertBlocks()
{
    rc::check("Mutator for inserting blocks leaves the rest of the string untouched",
        [](const std::string& original) {
            auto tmp = original;
            mutators::insertBlock(tmp);

            // Size got bigger
            RC_ASSERT(tmp.size() > original.size());

            // Original characters are still in it
            for (const auto& c : original)
                RC_ASSERT(tmp.find(c, 0) != std::string::npos);
        });
}

void flipBitIsStillASCII()
{
    rc::check("Mutator for flipping a bit outputs a still valid ASCII character",
        [](char c) {
            c &= 0b01111111;
            // Must be a valid character to behind with
            if (!isJsonAllowedOrEscapeable(c))
                return;

            std::string tmp = std::string(1, c);
            mutators::flipBitASCII(tmp);

            RC_ASSERT(tmp.size() == 1);

            // Must stay valid
            RC_ASSERT(isJsonAllowedOrEscapeable(tmp.at(0)));
        });
}

void generateInRange()
{
    rc::check("String generator returns chars in range",
        [](uint8_t b1, uint8_t b2) {
            uint8_t start = std::min(b1, b2);
            uint8_t end = std::max(b1, b2);

            auto tmp = generators::generateRandomString(start, end, 100);

            for (const auto& c : tmp)
                RC_ASSERT((uint8_t)c >= start && (uint8_t)c <= end);
        });
}

int main() {
    mutatorInsertBlocks();
    flipBitIsStillASCII();
    generateInRange();
    return 0;
}