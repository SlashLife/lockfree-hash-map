#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/capacity", "") {
	hash_map<int, int> hm(5);

	REQUIRE( 1'000'000'000ULL < hm.max_size() ); // arbitrary check for "big enough"
	const hash_map<int, int>::size_type max_size = hm.max_size();

	REQUIRE( hm.empty() );
	REQUIRE( hm.size() == 0 );
	REQUIRE( hm.max_size() == max_size );

	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i % 10] = 2*i;

		REQUIRE_FALSE( hm.empty() );
		REQUIRE( hm.size() == i );
		REQUIRE( hm.max_size() == max_size );
	}

	hm[11] = 0;

	for(const auto i : {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}) {
		hm.erase( (i+5) % 10 );

		REQUIRE_FALSE( hm.empty() );
		REQUIRE( hm.size() == i );
		REQUIRE( hm.max_size() == max_size );
	}

	hm.erase(11);

	REQUIRE( hm.empty() );
	REQUIRE( hm.size() == 0 );
	REQUIRE( hm.max_size() == max_size );
}
