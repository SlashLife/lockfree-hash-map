#include <iostream>
#include <functional>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/assign_compare_swap: operator==/operator!=", "") {
	SECTION("filled with same data") {
		hash_map<int, int> hm_orig(5);
		hash_map<int, int> hm_different_size(7);
		hash_map<int, int> hm_opposite_fill_order(5);
		hash_map<int, int> hm_opposite_fill_order_different_size(7);

		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			hm_orig[i*i] = 2*i;
			hm_different_size[i*i] = 2*i;
		}

		for(const auto i : {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}) {
			hm_opposite_fill_order[i*i] = 2*i;
			hm_opposite_fill_order_different_size[i*i] = 2*i;
		}

		hash_map<int, int> hm_straight_copy(hm_orig);

		INFO_MAP(hm_orig);
		INFO_MAP(hm_different_size);
		INFO_MAP(hm_opposite_fill_order);
		INFO_MAP(hm_opposite_fill_order_different_size);
		INFO_MAP(hm_straight_copy);

		REQUIRE( hm_orig                              .size() == 10 );
		REQUIRE( hm_different_size                    .size() == 10 );
		REQUIRE( hm_opposite_fill_order               .size() == 10 );
		REQUIRE( hm_opposite_fill_order_different_size.size() == 10 );
		REQUIRE( hm_straight_copy                     .size() == 10 );

		REQUIRE( hm_orig                               == hm_orig );
		REQUIRE( hm_different_size                     == hm_orig );
		REQUIRE( hm_opposite_fill_order                == hm_orig );
		REQUIRE( hm_opposite_fill_order_different_size == hm_orig );
		REQUIRE( hm_straight_copy                      == hm_orig );

		REQUIRE_FALSE( hm_orig                               != hm_orig );
		REQUIRE_FALSE( hm_different_size                     != hm_orig );
		REQUIRE_FALSE( hm_opposite_fill_order                != hm_orig );
		REQUIRE_FALSE( hm_opposite_fill_order_different_size != hm_orig );
		REQUIRE_FALSE( hm_straight_copy                      != hm_orig );
	}

	SECTION("filled with same data (comparable hash)") {
		// comparable hashes allow bucket-wise comparison optimization

		const auto hasher = [](int i) -> unsigned { return i; };

		hash_map<int, int, unsigned(*)(int)> hm_orig(5, hasher);
		hash_map<int, int, unsigned(*)(int)> hm_different_size(7, hasher);
		hash_map<int, int, unsigned(*)(int)> hm_opposite_fill_order(5, hasher);

		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			hm_orig[i*i] = 2*i;
			hm_different_size[i*i] = 2*i;
		}

		for(const auto i : {10, 9, 8, 7, 6, 5, 4, 3, 2, 1}) {
			hm_opposite_fill_order[i*i] = 2*i;
		}

		INFO_MAP(hm_orig);
		INFO_MAP(hm_different_size);
		INFO_MAP(hm_opposite_fill_order);

		REQUIRE( hm_orig               .size() == 10 );
		REQUIRE( hm_different_size     .size() == 10 );
		REQUIRE( hm_opposite_fill_order.size() == 10 );

		REQUIRE( hm_orig                == hm_orig );
		REQUIRE( hm_different_size      == hm_orig );
		REQUIRE( hm_opposite_fill_order == hm_orig );

		REQUIRE_FALSE( hm_orig               != hm_orig );
		REQUIRE_FALSE( hm_different_size     != hm_orig );
		REQUIRE_FALSE( hm_opposite_fill_order!= hm_orig );
	}

	SECTION("filled with different data") {
		hash_map<int, int> hm_orig(5);
		for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
			hm_orig[i*i] = 2*i;
		}

		hash_map<int, int> hm_different_data(hm_orig);
		REQUIRE( hm_different_data.find(4) != hm_different_data.end() );
		REQUIRE( hm_different_data[4] != 8 );
			hm_different_data[4] = 8;

		hash_map<int, int> hm_different_size(hm_orig);
		REQUIRE( hm_different_size.find(8) == hm_different_size.end() );
			hm_different_size[8] = 4;

		INFO_MAP(hm_orig);
		INFO_MAP(hm_different_data);
		INFO_MAP(hm_different_size);

		REQUIRE( hm_orig          .size() == 10 );
		REQUIRE( hm_different_data.size() == 10 );
		REQUIRE( hm_different_size.size() == 11 );

		REQUIRE( hm_orig == hm_orig );
		REQUIRE_FALSE( hm_different_data == hm_orig );
		REQUIRE_FALSE( hm_different_size == hm_orig );

		REQUIRE_FALSE( hm_orig != hm_orig );
		REQUIRE( hm_different_data != hm_orig );
		REQUIRE( hm_different_size != hm_orig );
	}
}

TEST_CASE("hash_map/assign_compare_swap: operator=", "") {
	comparable_map hm_orig(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm_orig[i*i] = 2*i;
	}

	comparable_map hm_copy(hm_orig);
	comparable_map hm_assigned(7);
	REQUIRE( hm_assigned.size() == 0 );
	REQUIRE( hm_assigned.bucket_count() != hm_copy.bucket_count() );
	REQUIRE( hm_assigned.hash_function() != hm_copy.hash_function() );
	REQUIRE( hm_assigned.key_eq() != hm_copy.key_eq() );
	REQUIRE( hm_assigned.get_allocator() != hm_copy.get_allocator() );
		hm_assigned.swap(hm_orig);
		//hm_assigned = hm_orig;

	INFO_MAP(hm_copy);
	INFO_MAP(hm_assigned);

	REQUIRE( hm_copy    .size() == 10 );
	REQUIRE( hm_assigned.size() == 10 );

	// same metadata
	REQUIRE( hm_assigned.bucket_count() == hm_copy.bucket_count() );
	REQUIRE( hm_assigned.hash_function() == hm_copy.hash_function() );
	REQUIRE( hm_assigned.key_eq() == hm_copy.key_eq() );
	REQUIRE( hm_assigned.get_allocator() == hm_copy.get_allocator() );

	// same data
	REQUIRE( hm_assigned == hm_copy );
	auto copy_begin = hm_copy.cbegin();
	auto copy_end = hm_copy.cend();
	auto assigned_begin = hm_assigned.cbegin();
	auto assigned_end = hm_assigned.cend();
	while(copy_begin != copy_end) {
		REQUIRE( assigned_begin != assigned_end );
		REQUIRE( *copy_begin == *assigned_begin );
		++copy_begin;
		++assigned_begin;
	}
	REQUIRE( assigned_begin == assigned_end );
}

TEST_CASE("hash_map/assign_compare_swap: swap/std::swap", "") {
	comparable_map hm_orig_1(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm_orig_1[i*i] = 2*i;
	}
	comparable_map hm_swap_1(hm_orig_1);
	comparable_map hm_swap_1_std(hm_orig_1);

	comparable_map hm_orig_2(3);
	for(const auto i : {5, 10, 15, 20, 25}) {
		hm_orig_2[i*i] = 2*i;
	}
	comparable_map hm_swap_2(hm_orig_2);
	comparable_map hm_swap_2_std(hm_orig_2);

	REQUIRE( hm_orig_1 != hm_orig_2 );
	REQUIRE( hm_orig_1.bucket_count() != hm_orig_2.bucket_count() );
	REQUIRE( hm_orig_1.hash_function() != hm_orig_2.hash_function() );
	REQUIRE( hm_orig_1.key_eq() != hm_orig_2.key_eq() );
	REQUIRE( hm_orig_1.get_allocator() != hm_orig_2.get_allocator() );

	hm_swap_1.swap(hm_swap_2);
	std::swap(hm_swap_1_std, hm_swap_2_std);
	REQUIRE( hm_orig_1 == hm_swap_2 );
	REQUIRE( hm_orig_1 == hm_swap_2_std );
	REQUIRE( hm_orig_1.bucket_count() == hm_swap_2.bucket_count() );
	REQUIRE( hm_orig_1.bucket_count() == hm_swap_2_std.bucket_count() );
	REQUIRE( hm_orig_1.hash_function() == hm_swap_2.hash_function() );
	REQUIRE( hm_orig_1.hash_function() == hm_swap_2_std.hash_function() );
	REQUIRE( hm_orig_1.key_eq() == hm_swap_2.key_eq() );
	REQUIRE( hm_orig_1.key_eq() == hm_swap_2_std.key_eq() );
	REQUIRE( hm_orig_1.get_allocator() == hm_swap_2.get_allocator() );
	REQUIRE( hm_orig_1.get_allocator() == hm_swap_2_std.get_allocator() );

	REQUIRE( hm_orig_2 == hm_swap_1 );
	REQUIRE( hm_orig_2 == hm_swap_1_std );
	REQUIRE( hm_orig_2.bucket_count() == hm_swap_1.bucket_count() );
	REQUIRE( hm_orig_2.bucket_count() == hm_swap_1_std.bucket_count() );
	REQUIRE( hm_orig_2.hash_function() == hm_swap_1.hash_function() );
	REQUIRE( hm_orig_2.hash_function() == hm_swap_1_std.hash_function() );
	REQUIRE( hm_orig_2.key_eq() == hm_swap_1.key_eq() );
	REQUIRE( hm_orig_2.key_eq() == hm_swap_1_std.key_eq() );
	REQUIRE( hm_orig_2.get_allocator() == hm_swap_1.get_allocator() );
	REQUIRE( hm_orig_2.get_allocator() == hm_swap_1_std.get_allocator() );
}
