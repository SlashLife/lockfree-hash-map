#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/rehash", "") {
	comparable_map hm_orig(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm_orig[i*i] = 2*i;
	}
	comparable_map hm_copy(hm_orig);

	REQUIRE( hm_copy.bucket_count() == hm_orig.bucket_count() );
	REQUIRE( hm_copy.size()         == hm_orig.size() );
	REQUIRE( hm_copy                == hm_orig );
	{ // contain same data order
		auto orig_begin=hm_orig.cbegin(), orig_end=hm_orig.cend();
		auto copy_begin=hm_copy.cbegin(), copy_end=hm_copy.cend();

		while(orig_begin != orig_end) {
			REQUIRE(copy_begin != copy_end);
			REQUIRE(*copy_begin == *orig_begin);
			++orig_begin;
			++copy_begin;
		}
		REQUIRE(copy_begin == copy_end);
	}

	hm_copy.rehash(5); // rehash to same size: noop

	REQUIRE( hm_copy.bucket_count() == hm_orig.bucket_count() );
	REQUIRE( hm_copy.size()         == hm_orig.size() );
	REQUIRE( hm_copy                == hm_orig );
	{ // contain same data order, because rehash to same size is a noop
		auto orig_begin=hm_orig.cbegin(), orig_end=hm_orig.cend();
		auto copy_begin=hm_copy.cbegin(), copy_end=hm_copy.cend();

		while(orig_begin != orig_end) {
			REQUIRE(copy_begin != copy_end);
			REQUIRE(*copy_begin == *orig_begin);
			++orig_begin;
			++copy_begin;
		}
		REQUIRE(copy_begin == copy_end);
	}

	hm_copy.rehash(3);

	REQUIRE( hm_copy.bucket_count() != hm_orig.bucket_count() );
	REQUIRE( hm_copy.size()         == hm_orig.size() );
	REQUIRE( hm_copy                == hm_orig );
}
