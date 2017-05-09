#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/create_destroy: component constructor", "") {
	SECTION("vanilla - use default values") {
		hash_map<int, int> hm(42);
		
		REQUIRE( hm.empty() );

		REQUIRE( hm.size() == 0 );
		REQUIRE( hm.begin() == hm.end() );
		REQUIRE( hm.cbegin() == hm.cend() );

		REQUIRE( hm.bucket_count() == 42 );
		for(decltype(hm)::size_type bucket=0, end=42; bucket<end; ++bucket) {
			INFO( "bucket = " << bucket );
			REQUIRE( hm.bucket_size(bucket) == 0 );
			REQUIRE( hm.begin(bucket) == hm.end(bucket) );
			REQUIRE( hm.cbegin(bucket) == hm.cend(bucket) );
		}
	}

	SECTION("custom - explicitly pass values to constructor") {
		hash_map<
			int, int,
			unsigned short(*)(short),
			bool(*)(long, long),
			std::allocator<void*>
		> hm(
			23,
			[](short) -> unsigned short { return 7; },
			[](long, long) { return true; },
			std::allocator<void*>()
		);
		
		REQUIRE( hm.empty() );

		REQUIRE( hm.size() == 0 );
		REQUIRE( hm.begin() == hm.end() );
		REQUIRE( hm.cbegin() == hm.cend() );

		REQUIRE( hm.bucket_count() == 23 );
		for(decltype(hm)::size_type bucket=0, end=23; bucket<end; ++bucket) {
			INFO( "bucket = " << bucket );
			REQUIRE( hm.bucket_size(bucket) == 0 );
			REQUIRE( hm.begin(bucket) == hm.end(bucket) );
			REQUIRE( hm.cbegin(bucket) == hm.cend(bucket) );
		}
	}
}

TEST_CASE("hash_map/create_destroy: copy constructor", "") {
	SECTION("hash_map metadata") {
		comparable<std::hash<int>> common_hash, uncommon_hash;
		comparable<std::equal_to<int>> common_keyeq, uncommon_keyeq;
		comparable<std::allocator<void*>> common_alloc, uncommon_alloc;
		
		REQUIRE( common_hash == comparable<std::hash<int>>(common_hash) );
		REQUIRE( common_hash != uncommon_hash );
		REQUIRE( common_keyeq == comparable<std::equal_to<int>>(common_keyeq) );
		REQUIRE( common_keyeq != uncommon_keyeq );
		REQUIRE( common_alloc == comparable<std::allocator<void*>>(common_alloc) );
		REQUIRE( common_alloc != uncommon_alloc );

		comparable_map orig(15, common_hash, common_keyeq, common_alloc);
		comparable_map copy(orig);
		comparable_map non_copy(17, uncommon_hash, uncommon_keyeq, uncommon_alloc);
		
		REQUIRE( orig.bucket_count() == copy.bucket_count() );
		REQUIRE( orig.bucket_count() != non_copy.bucket_count() );
		REQUIRE( orig.hash_function() == copy.hash_function() );
		REQUIRE( orig.hash_function() != non_copy.hash_function() );
		REQUIRE( orig.key_eq() == copy.key_eq() );
		REQUIRE( orig.key_eq() != non_copy.key_eq() );
		REQUIRE( orig.get_allocator() == copy.get_allocator() );
		REQUIRE( orig.get_allocator() != non_copy.get_allocator() );
	}
	
	SECTION("hash_map data") {
		hash_map<int, int> orig(3);
		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			orig[i*i] = 2*i;
		}
		
		hash_map<int, int> copy(orig);
		
		REQUIRE( copy.bucket_count() == orig.bucket_count() );
		REQUIRE( copy.bucket_count() == 3 );
		REQUIRE( copy.size() == orig.size() );
		REQUIRE( copy.size() == 10 );
		
		{ // compare data with global iterator
			auto orig_b = orig.cbegin();
			auto orig_e = orig.cend();
			auto copy_b = copy.cbegin();
			auto copy_e = copy.cend();
			
			while(orig_b != orig_e) {
				REQUIRE( copy_b != copy_e );
				REQUIRE( *orig_b == *copy_b );
				++orig_b;
				++copy_b;
			}
			REQUIRE( copy_b == copy_e );
		}

		// bucket-wise data comparison
		for(auto e=copy.bucket_count(), b=0*e; b < e; ++b) {
			auto orig_b = orig.cbegin(b);
			auto orig_e = orig.cend(b);
			auto copy_b = copy.cbegin(b);
			auto copy_e = copy.cend(b);
			
			while(orig_b != orig_e) {
				REQUIRE( copy_b != copy_e );
				REQUIRE( *orig_b == *copy_b );
				++orig_b;
				++copy_b;
			}
			REQUIRE( copy_b == copy_e );
		}
	}

	SECTION("test deep copy") {
		hash_map<int, int> orig(3);
		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			orig[i*i] = 2*i;
		}
		
		hash_map<int, int> copy1(orig);
		hash_map<int, int> copy2(orig);
		orig.erase(orig.begin());
		
		REQUIRE( copy1 == copy2 );
		REQUIRE( copy1 != orig );
		REQUIRE( copy2 != orig );
	}
}

TEST_CASE("hash_map/create_destroy: destructor", "") {
	REQUIRE( tracked_mapped_type::created == tracked_mapped_type::destroyed );
	{ hash_map<int, tracked_mapped_type> hm(3);
		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			hm[i];
		}
		
		REQUIRE( hm.size() == 10 );
		REQUIRE( tracked_mapped_type::created == tracked_mapped_type::destroyed+10 );
	}
	REQUIRE( tracked_mapped_type::created == tracked_mapped_type::destroyed );
}
