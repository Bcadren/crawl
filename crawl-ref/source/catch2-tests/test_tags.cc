#include "catch.hpp"

#include "AppHdr.h"

#include "tags.h"

FixedVector<spell_type, MAX_KNOWN_SPELLS> unmarshall_player_spells(reader &th);
void remove_removed_library_spells(FixedBitVector<NUM_SPELLS>& lib);

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

    SECTION ("removed spells are filtered out") {
        vector<unsigned char> input = {
            0x02, // count
            0x00, 0x07, // SPELL_STRIKING
            0x00, 0x04, // SPELL_FIREBALL
        };
        auto r = reader(input);
        r.setMinorVersion(TAG_MINOR_SHORT_SPELL_TYPE);

        FixedVector<spell_type, MAX_KNOWN_SPELLS> expected_spells(SPELL_NO_SPELL);
        expected_spells[0] = SPELL_NO_SPELL;
        expected_spells[1] = SPELL_FIREBALL;

        const auto result = unmarshall_player_spells(r);

        for (size_t i = 0; i < result.size(); i++)
            REQUIRE(result[i] == expected_spells[i]);
        REQUIRE(r.valid() == false);
    }
}

TEST_CASE( "Removed spells remover can filter spell library", "[single-file]" ) {

    SECTION ("removed spells are removed from the library") {
        FixedBitVector<NUM_SPELLS> library;
        library.set(SPELL_STRIKING, true);

        remove_removed_library_spells(library);

        REQUIRE(library[SPELL_STRIKING] == false);
    }

    SECTION ("non-removed spells are not removed from the library") {
        FixedBitVector<NUM_SPELLS> library;
        library.set(SPELL_MAGIC_DART, true);

        remove_removed_library_spells(library);

        REQUIRE(library[SPELL_MAGIC_DART] == true);
    }

    SECTION ("non-present spells are not added to the library") {
        FixedBitVector<NUM_SPELLS> library;
        library.set(SPELL_MAGIC_DART, false);

        remove_removed_library_spells(library);

        REQUIRE(library[SPELL_MAGIC_DART] == false);
    }
}
