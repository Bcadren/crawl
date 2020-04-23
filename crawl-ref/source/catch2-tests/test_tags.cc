#include "catch.hpp"

#include "AppHdr.h"

#include "tags.h"

FixedVector<spell_type, MAX_KNOWN_SPELLS> unmarshall_player_spells(reader &th);

TEST_CASE( "Player spells can be decoded", "[single-file]" ) {

    SECTION ("spells stored as shorts can be read") {
        vector<unsigned char> input = {
            0x02, // count
            0x00, 0x03, // SPELL_MAGIC_DART
            0x00, 0x04, // SPELL_FIREBALL
        };
        auto r = reader(input);
        r.setMinorVersion(TAG_MINOR_SHORT_SPELL_TYPE);

        FixedVector<spell_type, MAX_KNOWN_SPELLS> expected_spells(SPELL_NO_SPELL);
        expected_spells[0] = SPELL_MAGIC_DART;
        expected_spells[1] = SPELL_FIREBALL;

        const auto result = unmarshall_player_spells(r);

        for (size_t i = 0; i < result.size(); i++)
            REQUIRE(result[i] == expected_spells[i]);
        REQUIRE(r.valid() == false);
    }
}
