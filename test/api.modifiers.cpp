#include <iterator>
#include <utility>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/modifiers: clear", "") {
	hash_map<int, tracked_mapped_type> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i];
	}

	REQUIRE( !hm.empty() );
	REQUIRE( 0 < hm.size() );
	REQUIRE( hm.size() == tracked_mapped_type::created - tracked_mapped_type::destroyed );
	REQUIRE( hm.begin() != hm.end() );

	hm.clear();

	REQUIRE( hm.empty() );
	REQUIRE( 0 == hm.size() );
	REQUIRE( hm.size() == tracked_mapped_type::created - tracked_mapped_type::destroyed );
	REQUIRE( hm.begin() == hm.end() );

	hm.clear(); // won't change anything

	REQUIRE( hm.empty() );
	REQUIRE( 0 == hm.size() );
	REQUIRE( hm.size() == tracked_mapped_type::created - tracked_mapped_type::destroyed );
	REQUIRE( hm.begin() == hm.end() );
}

TEST_CASE("hash_map/modifiers: insert", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	hash_map<int, int> hm_orig(hm);

	REQUIRE( hm.size() == 10 );
	const int existing_key = 5;
	const int existing_value = hm[existing_key];
	const int overriding_value = existing_value+2;
	REQUIRE( existing_value != overriding_value ); // need to be distinguishable

	{ // element exists already, noop!
		REQUIRE( hm.find(existing_key) != hm.end() );
		REQUIRE( hm[existing_key] == existing_value );
		auto result = hm.insert( std::make_pair(existing_key, overriding_value) );
		REQUIRE( hm == hm_orig );
		REQUIRE_FALSE( result.first );
		REQUIRE( result.second->first == existing_key );
		REQUIRE( result.second->second == existing_value );
		REQUIRE( hm[existing_key] == existing_value );
	}

	{ // element exists already, noop! (w/ hint iterator)
		REQUIRE( hm.find(existing_key) != hm.end() );
		REQUIRE( hm[existing_key] == existing_value );
		auto result = hm.insert( hm.begin(), std::make_pair(existing_key, overriding_value) );
		REQUIRE( hm == hm_orig );
		REQUIRE( result->first == existing_key );
		REQUIRE( result->second == existing_value );
		REQUIRE( hm[existing_key] == existing_value );
	}

	// element does not exist - insert
	{ const int key = 50, value = 80;
		REQUIRE( hm.find(key) == hm.end() );
		auto result = hm.insert( std::make_pair(key, value) );
		REQUIRE( hm != hm_orig );
		REQUIRE( hm.size() == 11 );
		REQUIRE( result.first );
		REQUIRE( result.second->first == key );
		REQUIRE( result.second->second == value );
		REQUIRE( hm[key] == value );
		// we insert at the end of a bucket, so the next iterator should be a bucket end
		REQUIRE( std::next(hash_map<int, int>::local_iterator(result.second)) == hm.end(hm.bucket(key)) );
	}

	// element does not exist - insert (w/ hint iterator)
	{ const int key = 51, value = 82;
		REQUIRE( hm.find(key) == hm.end() );
		auto result = hm.insert( hm.begin(), std::make_pair(key, value) );
		REQUIRE( hm != hm_orig );
		REQUIRE( hm.size() == 12 );
		REQUIRE( result->first == key );
		REQUIRE( result->second == value );
		REQUIRE( hm[key] == value );
		// we insert at the end of a bucket, so the next iterator should be a bucket end
		REQUIRE( std::next(hash_map<int, int>::local_iterator(result)) == hm.end(hm.bucket(key)) );
	}
}

TEST_CASE("hash_map/modifiers: insert_or_assign", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}
	hash_map<int, int> hm_orig(hm);

	REQUIRE( hm.size() == 10 );
	const int existing_key = 5;
	const int existing_value = hm[existing_key];
	const int overriding_value = existing_value+2;

	{ // element exists already, assign
		REQUIRE( hm.find(existing_key) != hm.end() );
		REQUIRE( hm[existing_key] == existing_value );
		auto result = hm.insert_or_assign(existing_key, overriding_value);
		REQUIRE( hm != hm_orig );
		REQUIRE( result->first == existing_key );
		REQUIRE( result->second == overriding_value );
		REQUIRE( hm[existing_key] == overriding_value );
	}

	// element does not exist - insert
	{ const int key = 50, value = 80;
		REQUIRE( hm.find(key) == hm.end() );
		auto result = hm.insert_or_assign(key, value);
		REQUIRE( hm != hm_orig );
		REQUIRE( hm.size() == 11 );
		REQUIRE( result->first == key );
		REQUIRE( result->second == value );
		REQUIRE( hm[key] == value );
		// we insert at the end of a bucket, so the next iterator should be a bucket end
		REQUIRE( std::next(hash_map<int, int>::local_iterator(result)) == hm.end(hm.bucket(key)) );
	}
}

TEST_CASE("hash_map/modifiers: erase", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i] = 2*i;
	}

	// key based, existing element
	{ const int key=5;
		REQUIRE( hm.find(key) != hm.end() );
		REQUIRE( hm.size() == 10 );

		REQUIRE( hm.erase(key) == 1 );

		REQUIRE( hm.find(key) == hm.end() );
		REQUIRE( hm.size() == 9 );
	}

	// key based, non-existing element
	{ const int key=42;
		REQUIRE( hm.find(key) == hm.end() );
		REQUIRE( hm.size() == 9 );

		REQUIRE( hm.erase(key) == 0 );

		REQUIRE( hm.find(key) == hm.end() );
		REQUIRE( hm.size() == 9 );
	}

	// iterator based, existing element
	{ const int key=hm.cbegin()->first;
		auto next = std::next(hm.cbegin());
		REQUIRE( hm.find(key) == hm.cbegin() );
		REQUIRE( hm.find(key) != hm.end() );
		REQUIRE( hm.size() == 9 );

		REQUIRE( hm.erase(hm.cbegin()) == next );

		REQUIRE( hm.find(key) == hm.end() );
		REQUIRE( hm.size() == 8 );
	}

	// iterator based, non-existing element
	// There are no iterators to non-existing elements!
}
