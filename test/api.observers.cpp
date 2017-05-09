#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/observers", "") {
	comparable_map::hasher hash;
	comparable_map::key_equal key_eq;
	comparable_map::allocator_type alloc;

	comparable_map hm(5, hash, key_eq, alloc);

	REQUIRE( hm.hash_function() == hash );
	REQUIRE( hm.key_eq() == key_eq );
	REQUIRE( hm.get_allocator() == alloc );
}
