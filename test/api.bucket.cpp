#include <iterator>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/bucket: iterators, bucket_size", "") {
	// see also hash_map/iterator tests.

	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i] = 2*i;
	}
	const hash_map<int, int> &hm_c = hm;

	hash_map<int, int>::size_type total_nodes = 0;
	for(
		hash_map<int, int>::size_type
			bucket=0,
			end=hm.bucket_count();
		bucket != end;
		++bucket
	) {
		REQUIRE( std::distance( hm  . begin(bucket), hm  . end(bucket) ) == hm  .bucket_size(bucket) );
		REQUIRE( std::distance( hm  .cbegin(bucket), hm  .cend(bucket) ) == hm  .bucket_size(bucket) );
		REQUIRE( std::distance( hm_c. begin(bucket), hm_c. end(bucket) ) == hm_c.bucket_size(bucket) );
		REQUIRE( std::distance( hm_c.cbegin(bucket), hm_c.cend(bucket) ) == hm_c.bucket_size(bucket) );
		total_nodes += hm.bucket_size(bucket);
	}
	REQUIRE( hm.size() == total_nodes );
}

TEST_CASE("hash_map/bucket: bucket_count", "") {
	hash_map<int, int> hm1(5);
	REQUIRE( hm1.bucket_count() == 5 );

	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm1[i*i] = 2*i;
	}
	REQUIRE( hm1.bucket_count() == 5 ); // did not change

	hash_map<int, int> hm2(hm1);
	REQUIRE( hm1.bucket_count() == 5 ); // did not change
	REQUIRE( hm2.bucket_count() == 5 ); // as well

	hm2.rehash(7);
	REQUIRE( hm1.bucket_count() == 5 ); // still did not change
	REQUIRE( hm2.bucket_count() == 7 ); // adjusted by rehashing

	hm1 = hm2;
	REQUIRE( hm1.bucket_count() == 7 ); // now was adjusted by assigning
	REQUIRE( hm2.bucket_count() == 7 ); // did not change
}

TEST_CASE("hash_map/bucket: max_bucket_count", "") {
	hash_map<int, int> hm(5);

	REQUIRE( 1'000'000ULL < hm.max_bucket_count() ); // arbitrary check for "big enough"
}

TEST_CASE("hash_map/bucket: bucket", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		const int key = i*i;
		const hash_map<int, int>::size_type bucket_id = hm.bucket(key);

		hash_map<int, int>::const_local_iterator it
			= hm.insert_or_assign(key, 2*i);

		// check whether the bucket_id is correct by checking against the
		// end iterator of the specified bucket.
		REQUIRE( std::next(it) == hm.end(bucket_id) );
		REQUIRE( hm.size() == i );
	}
}
