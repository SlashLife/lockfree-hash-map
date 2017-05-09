#include <iterator>
#include <stdexcept>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/lookup: at()", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	const hash_map<int, int> &hm_c = hm;

	REQUIRE( hm.size() == 10 );

	// existing element
	REQUIRE( hm.at(7) == 14 ); // current value
	REQUIRE( 42 == (hm.at(7) = 42) ); // change through ref
	REQUIRE( hm_c.at(7) == 42 ); // reflect change

	// non-existing element
	REQUIRE_THROWS_AS( hm.at(23), std::out_of_range );
	REQUIRE_THROWS_AS( hm_c.at(42), std::out_of_range );

	REQUIRE( hm.size() == 10 ); // unchanged
}

TEST_CASE("hash_map/lookup: operator[]", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	// op[] is non-const only
	// const hash_map<int, int> &hm_c = hm;

	REQUIRE( hm.size() == 10 );

	// existing element
	REQUIRE( hm[7] == 14 ); // current value
	REQUIRE( 42 == (hm[7] = 42) ); // change through ref
	REQUIRE( hm[7] == 42 ); // reflect change

	REQUIRE( hm.size() == 10 ); // unchanged

	// non-existing element
	REQUIRE_NOTHROW( hm[23] );
	REQUIRE( hm[23] == 0 );

	REQUIRE( hm.size() == 11 ); // changed
}

TEST_CASE("hash_map/lookup: count()", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	const hash_map<int, int> &hm_c = hm;

	REQUIRE( hm.count(2) == 1 );
	REQUIRE( hm.count(8) == 1 );

	hm.erase(8);
	REQUIRE( hm.count(2) == 1 );
	REQUIRE( hm.count(8) == 0 );

	REQUIRE( hm.count(11) == 0 );
	REQUIRE( hm.count(12) == 0 );

	hm[12] = 0;
	REQUIRE( hm.count(11) == 0 );
	REQUIRE( hm.count(12) == 1 );
}

TEST_CASE("hash_map/lookup: find()", "") {
	hash_map<int, int> hm(1);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	const hash_map<int, int> &hm_c = hm;

	REQUIRE( hm  .find( 1) == hm  .begin() );
	REQUIRE( hm_c.find( 1) == hm_c.begin() );
	REQUIRE( hm  .find( 1)->second == 2 );
	REQUIRE( hm_c.find( 1)->second == 2 );

	REQUIRE( hm  .find(10) != hm  .end() );
	REQUIRE( hm_c.find(10) != hm_c.end() );
	REQUIRE( hm  .find(10)->second == 20 );
	REQUIRE( hm_c.find(10)->second == 20 );

	REQUIRE( hm  .find(23) == hm  .end() );
	REQUIRE( hm_c.find(23) == hm_c.end() );
}

TEST_CASE("hash_map/lookup: equal_range()", "") {
	hash_map<int, int> hm(1);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	const hash_map<int, int> &hm_c = hm;

	{ auto range = hm.equal_range(1);
		REQUIRE( range.first  == hm.begin(0) );
		REQUIRE( range.second == std::next(hm.begin(0)) );
		
		REQUIRE( range.first ->second == 2 );
		REQUIRE( range.second->second == 4 );
	}
	
	{ auto range = hm_c.equal_range(1);
		REQUIRE( range.first  == hm_c.begin(0) );
		REQUIRE( range.second == std::next(hm_c.begin(0)) );
		
		REQUIRE( range.first ->second == 2 );
		REQUIRE( range.second->second == 4 );
	}
	
	{ auto range = hm.equal_range(10);
		REQUIRE( range.first  != hm.end(0) );
		REQUIRE( range.second == hm.end(0) );
		
		REQUIRE( range.first->second == 20 );
	}
	
	{ auto range = hm_c.equal_range(10);
		REQUIRE( range.first  != hm_c.end(0) );
		REQUIRE( range.second == hm_c.end(0) );
		
		REQUIRE( range.first->second == 20 );
	}
	
	{ auto range = hm.equal_range(23);
		REQUIRE( range.first  == hm.end(0) );
		REQUIRE( range.second == hm.end(0) );
	}
	
	{ auto range = hm_c.equal_range(23);
		REQUIRE( range.first  == hm_c.end(0) );
		REQUIRE( range.second == hm_c.end(0) );
	}
}
