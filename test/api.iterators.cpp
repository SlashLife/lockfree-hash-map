#include<iterator>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/iterators: non-local (c)begin(), (c)end()", "") {
	hash_map<int, int> hm(5);
	const hash_map<int, int> &hm_c = hm;

	REQUIRE( hm.begin() == hm.end() );
	REQUIRE( hm.cbegin() == hm.cend() );
	REQUIRE( hm_c.begin() == hm_c.end() );
	REQUIRE( hm_c.cbegin() == hm_c.cend() );

	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i] = 2*i;
	}

	REQUIRE( hm.begin() != hm.end() );
	REQUIRE( hm.cbegin() != hm.cend() );
	REQUIRE( hm_c.begin() != hm_c.end() );
	REQUIRE( hm_c.cbegin() != hm_c.cend() );

	REQUIRE( hm_c.size() == hm.size() );
	REQUIRE( hm  .size() == std::distance(hm.begin(), hm.end()) );
	REQUIRE( hm  .size() == std::distance(hm.cbegin(), hm.cend()) );
	REQUIRE( hm_c.size() == std::distance(hm_c.begin(), hm_c.end()) );
	REQUIRE( hm_c.size() == std::distance(hm_c.cbegin(), hm_c.cend()) );
}

TEST_CASE("hash_map/iterators: iterators comparison, iterator conversion, range equivalency of local and non-local iterators", "") {
	hash_map<int, int> hm(5);
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i] = 2*i;
	}

	// iterator and const_iterator; converted and not converted
	hash_map<int, int>::iterator       it            = hm.begin();
	hash_map<int, int>::const_iterator it_const      = hm.cbegin();
	hash_map<int, int>::iterator       it_conv       = it_const.base();
	hash_map<int, int>::const_iterator it_const_conv = it;

	hash_map<int, int>::iterator       it_end            = hm.end();
	hash_map<int, int>::const_iterator it_end_const      = hm.cend();
	hash_map<int, int>::iterator       it_end_conv       = it_end_const.base();
	hash_map<int, int>::const_iterator it_end_const_conv = it_end;



	hash_map<int, int>::size_type current_bucket=0;
	const hash_map<int, int>::size_type num_buckets=hm.bucket_count();
	hash_map<int, int>::size_type empty_buckets=0;

	for(; current_bucket < num_buckets; ++current_bucket) {
		// local_iterator and local_const_iterator; converted and not converted
		hash_map<int, int>::local_iterator       local_it            = hm.begin(current_bucket);
		hash_map<int, int>::const_local_iterator local_it_const      = hm.cbegin(current_bucket);
		hash_map<int, int>::const_local_iterator local_it_const_conv = local_it;

		hash_map<int, int>::local_iterator       local_it_end            = hm.end(current_bucket);
		hash_map<int, int>::const_local_iterator local_it_end_const      = hm.cend(current_bucket);
		hash_map<int, int>::const_local_iterator local_it_end_const_conv = local_it_end;

		// careful with these: (const_)local_iterator::base() is undefined for end iterators!
		hash_map<int, int>::local_iterator local_it_conv =
			(local_it_const == hm.cend(current_bucket)) ? local_it_end : local_it_const.base();
		hash_map<int, int>::local_iterator local_it_end_conv =
			(local_it_end_const == hm.cend(current_bucket)) ? local_it_end : local_it_end_const.base();

		if(local_it == local_it_end) {
			++empty_buckets;
		}

		while(local_it != local_it_end) {
			REQUIRE( it != it_end );
			REQUIRE( it_const != it_end_const );
			REQUIRE( it_conv != it_end_conv );
			REQUIRE( it_const_conv != it_end_const_conv );

			REQUIRE( local_it != local_it_end );
			REQUIRE( local_it_const != local_it_end_const );
			REQUIRE( local_it_conv != local_it_end_conv );
			REQUIRE( local_it_const_conv != local_it_end_const_conv );

			// yup, we're doing the full comparison matrix!
			REQUIRE( it == it );
			REQUIRE( it == it_const );
			REQUIRE( it == it_conv );
			REQUIRE( it == it_const_conv );
			REQUIRE( it == local_it );
			REQUIRE( it == local_it_const );
			REQUIRE( it == local_it_conv );
			REQUIRE( it == local_it_const_conv );

			REQUIRE( it_const == it );
			REQUIRE( it_const == it_const );
			REQUIRE( it_const == it_conv );
			REQUIRE( it_const == it_const_conv );
			REQUIRE( it_const == local_it );
			REQUIRE( it_const == local_it_const );
			REQUIRE( it_const == local_it_conv );
			REQUIRE( it_const == local_it_const_conv );

			REQUIRE( it_conv == it );
			REQUIRE( it_conv == it_const );
			REQUIRE( it_conv == it_conv );
			REQUIRE( it_conv == it_const_conv );
			REQUIRE( it_conv == local_it );
			REQUIRE( it_conv == local_it_const );
			REQUIRE( it_conv == local_it_conv );
			REQUIRE( it_conv == local_it_const_conv );

			REQUIRE( it_const_conv == it );
			REQUIRE( it_const_conv == it_const );
			REQUIRE( it_const_conv == it_conv );
			REQUIRE( it_const_conv == it_const_conv );
			REQUIRE( it_const_conv == local_it );
			REQUIRE( it_const_conv == local_it_const );
			REQUIRE( it_const_conv == local_it_conv );
			REQUIRE( it_const_conv == local_it_const_conv );

			REQUIRE( local_it == it );
			REQUIRE( local_it == it_const );
			REQUIRE( local_it == it_conv );
			REQUIRE( local_it == it_const_conv );
			REQUIRE( local_it == local_it );
			REQUIRE( local_it == local_it_const );
			REQUIRE( local_it == local_it_conv );
			REQUIRE( local_it == local_it_const_conv );

			REQUIRE( local_it_const == it );
			REQUIRE( local_it_const == it_const );
			REQUIRE( local_it_const == it_conv );
			REQUIRE( local_it_const == it_const_conv );
			REQUIRE( local_it_const == local_it );
			REQUIRE( local_it_const == local_it_const );
			REQUIRE( local_it_const == local_it_conv );
			REQUIRE( local_it_const == local_it_const_conv );

			REQUIRE( local_it_conv == it );
			REQUIRE( local_it_conv == it_const );
			REQUIRE( local_it_conv == it_conv );
			REQUIRE( local_it_conv == it_const_conv );
			REQUIRE( local_it_conv == local_it );
			REQUIRE( local_it_conv == local_it_const );
			REQUIRE( local_it_conv == local_it_conv );
			REQUIRE( local_it_conv == local_it_const_conv );

			REQUIRE( local_it_const_conv == it );
			REQUIRE( local_it_const_conv == it_const );
			REQUIRE( local_it_const_conv == it_conv );
			REQUIRE( local_it_const_conv == it_const_conv );
			REQUIRE( local_it_const_conv == local_it );
			REQUIRE( local_it_const_conv == local_it_const );
			REQUIRE( local_it_const_conv == local_it_conv );
			REQUIRE( local_it_const_conv == local_it_const_conv );

			++it;
			++it_const;
			++it_conv;
			++it_const_conv;
			++local_it;
			++local_it_const;
			++local_it_conv;
			++local_it_const_conv;
		}

		REQUIRE( local_it == local_it_end );
		REQUIRE( local_it_const == local_it_end_const );
		REQUIRE( local_it_conv == local_it_end_conv );
		REQUIRE( local_it_const_conv == local_it_end_const_conv );
	}

	SECTION("data sanity check") {
		INFO(
			"We failed to check the behavior for empty buckets, "
			"because there were none."
		);
		REQUIRE( 0 < empty_buckets );
	}

	REQUIRE( it == it_end );
	REQUIRE( it_const == it_end_const );
	REQUIRE( it_conv == it_end_conv );
	REQUIRE( it_const_conv == it_end_const_conv );
}

TEST_CASE("hash_map/iterators: iterator incrementing", "") {
	hash_map<int, int> hm(1); // forces global sequence == bucket(0) sequence
	const hash_map<int, int> &hm_c = hm;
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i] = 2*i;
	}

#undef TEST_ITERATOR_INCREMENT
#define TEST_ITERATOR_INCREMENT( type, init_value ) \
	SECTION(#type) { \
		type it = init_value; \
		type next = std::next(it); \
		type pre_increment_operand = it; \
		type pre_increment_result = ++pre_increment_operand; \
		type post_increment_operand = it; \
		type post_increment_result = post_increment_operand++; \
		\
		REQUIRE( it != next ); \
		REQUIRE( *it != *next ); \
		\
		SECTION("checking the actual data") { \
			INFO("If these fail, the test case needs to be adjusted to the node insertion mechanism!"); \
			REQUIRE( it->first == 1 ); \
			REQUIRE( it->second == 2 ); \
			REQUIRE( next->first == 4 ); \
			REQUIRE( next->second == 4 ); \
		} \
		\
		REQUIRE( pre_increment_operand == next ); \
		REQUIRE( pre_increment_result  == next ); \
		REQUIRE( post_increment_operand == next ); \
		REQUIRE( post_increment_result  == it ); \
	}

	TEST_ITERATOR_INCREMENT( decltype(hm)::iterator, hm.begin() );
	TEST_ITERATOR_INCREMENT( decltype(hm)::const_iterator, hm.cbegin() );
	TEST_ITERATOR_INCREMENT( decltype(hm)::local_iterator, hm.begin(0) );
	TEST_ITERATOR_INCREMENT( decltype(hm)::const_local_iterator, hm.cbegin(0) );
}

TEST_CASE("hash_map/iterators: iterator_impl::base()", "") {
	hash_map<int, int> hm(1); // forces global sequence == bucket(0) sequence
	for(const auto i : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
		hm[i*i] = 2*i;
	}

#undef TEST_ITERATOR_BASE
#define TEST_ITERATOR_BASE( expected_iterator_type, iterator, base_iterator ) \
	SECTION( #expected_iterator_type ) { \
		REQUIRE( (std::is_same< expected_iterator_type, decltype(iterator) >::value) ); \
		const auto base = base_iterator; \
		const auto it   = iterator.base(); \
		REQUIRE( (std::is_same< decltype(base), decltype(it) >::value) ); \
		REQUIRE( base == it ); \
	}

	TEST_ITERATOR_BASE( decltype(hm)::iterator, hm.begin(), hm.begin() );
	TEST_ITERATOR_BASE( decltype(hm)::const_iterator, hm.cbegin(), hm.begin() );
	TEST_ITERATOR_BASE( decltype(hm)::local_iterator, hm.begin(0), hm.begin() );
	TEST_ITERATOR_BASE( decltype(hm)::const_local_iterator, hm.cbegin(0), hm.begin() );

	TEST_ITERATOR_BASE( decltype(hm)::iterator, hm.end(), hm.end() );
	TEST_ITERATOR_BASE( decltype(hm)::const_iterator, hm.cend(), hm.end() );
	// no test for local iterators: local iterator end iterators may not be
	// converted to non-local iterators (and vice versa) - and would in most
	// cases not be equivalent anyway.
}

TEST_CASE("hash_map/iterators: check order: iterator_impl::before(), std::less<iterator_impl>()", "") {
	hash_map<int, int> hm(1); // forces global sequence == bucket(0) sequence
	for(const auto i : {1, 2}) {
		hm[i*i] = 2*i;
	}

	auto A=hm.begin(), a=hm.find(1), b=hm.find(4), e=hm.end();

	SECTION("validate data") {
		// first check the iterators equality relations are the way we expect
		REQUIRE(A == A); REQUIRE(A == a); REQUIRE(A != b); REQUIRE(A != e);
		REQUIRE(a == A); REQUIRE(a == a); REQUIRE(a != b); REQUIRE(a != e);
		REQUIRE(b != A); REQUIRE(b != a); REQUIRE(b == b); REQUIRE(b != e);
		REQUIRE(e != A); REQUIRE(e != a); REQUIRE(e != b); REQUIRE(e == e);

		REQUIRE( A->first == 1 );
		REQUIRE( A->second == 2 );
		REQUIRE( a->first == 1 );
		REQUIRE( a->second == 2 );
		REQUIRE( b->first == 4 );
		REQUIRE( b->second == 4 );
	}

	SECTION("before and std::less must yield the same results") {
		REQUIRE( A.before(A) == std::less<decltype(hm)::iterator>()(A, A) );
		REQUIRE( A.before(a) == std::less<decltype(hm)::iterator>()(A, a) );
		REQUIRE( A.before(b) == std::less<decltype(hm)::iterator>()(A, b) );
		REQUIRE( A.before(e) == std::less<decltype(hm)::iterator>()(A, e) );

		REQUIRE( a.before(A) == std::less<decltype(hm)::iterator>()(a, A) );
		REQUIRE( a.before(a) == std::less<decltype(hm)::iterator>()(a, a) );
		REQUIRE( a.before(b) == std::less<decltype(hm)::iterator>()(a, b) );
		REQUIRE( a.before(e) == std::less<decltype(hm)::iterator>()(a, e) );

		REQUIRE( b.before(A) == std::less<decltype(hm)::iterator>()(b, A) );
		REQUIRE( b.before(a) == std::less<decltype(hm)::iterator>()(b, a) );
		REQUIRE( b.before(b) == std::less<decltype(hm)::iterator>()(b, b) );
		REQUIRE( b.before(e) == std::less<decltype(hm)::iterator>()(b, e) );

		REQUIRE( e.before(A) == std::less<decltype(hm)::iterator>()(e, A) );
		REQUIRE( e.before(a) == std::less<decltype(hm)::iterator>()(e, a) );
		REQUIRE( e.before(b) == std::less<decltype(hm)::iterator>()(e, b) );
		REQUIRE( e.before(e) == std::less<decltype(hm)::iterator>()(e, e) );
	}

	// std::less was shown to return the same results as .before(),
	// so it suffices to test just one of them.

	SECTION("equal iterators must not compare less either way") {
		REQUIRE_FALSE( A.before(A) );
		REQUIRE_FALSE( A.before(a) );
		REQUIRE_FALSE( a.before(A) );
		REQUIRE_FALSE( a.before(a) );
		REQUIRE_FALSE( b.before(b) );
		REQUIRE_FALSE( e.before(e) );
	}

	SECTION("unequal iterators must compare less in exactly one direction") {
		REQUIRE( A.before(b) != b.before(A) );
		REQUIRE( A.before(e) != e.before(A) );

		REQUIRE( a.before(b) != b.before(a) );
		REQUIRE( a.before(e) != e.before(a) );

		REQUIRE( b.before(A) != A.before(b) );
		REQUIRE( b.before(a) != a.before(b) );
		REQUIRE( b.before(e) != e.before(b) );

		REQUIRE( e.before(A) != A.before(e) );
		REQUIRE( e.before(a) != a.before(e) );
		REQUIRE( e.before(b) != b.before(e) );
	}

	SECTION("check transitivity: a < b && b < c => a < c") {
		if (a.before(b)) {
			if (e.before(a)) {
				// e < a == A < b
				REQUIRE( e.before(a) );
				REQUIRE( e.before(A) );
				REQUIRE( e.before(b) );
				REQUIRE( a.before(b) );
				REQUIRE( A.before(b) );
				// no need to test the reverse; already done above
			}
			else if (e.before(b)) {
				// a == A < e < b
				REQUIRE( a.before(e) );
				REQUIRE( a.before(b) );
				REQUIRE( A.before(e) );
				REQUIRE( A.before(b) );
				REQUIRE( e.before(b) );
				// no need to test the reverse; already done above
			}
			else {
				// a == A < b < e
				REQUIRE( a.before(b) );
				REQUIRE( a.before(e) );
				REQUIRE( A.before(b) );
				REQUIRE( A.before(e) );
				REQUIRE( b.before(e) );
				// no need to test the reverse; already done above
			}
		}
		else {
			if (e.before(b)) {
				// e < b < a == A
				REQUIRE( e.before(b) );
				REQUIRE( e.before(a) );
				REQUIRE( e.before(A) );
				REQUIRE( b.before(a) );
				REQUIRE( b.before(A) );
				// no need to test the reverse; already done above
			}
			else if (e.before(a)) {
				// b < e < a == A
				REQUIRE( b.before(e) );
				REQUIRE( b.before(a) );
				REQUIRE( b.before(A) );
				REQUIRE( e.before(a) );
				REQUIRE( e.before(A) );
				// no need to test the reverse; already done above
			}
			else {
				// b < a == A < e
				REQUIRE( b.before(a) );
				REQUIRE( b.before(A) );
				REQUIRE( b.before(e) );
				REQUIRE( a.before(e) );
				REQUIRE( A.before(e) );
				// no need to test the reverse; already done above
			}
		}
	}
}

TEST_CASE("hash_map/iterators: dereferencing: op*, op->", "") {
	hash_map<int, int> hm(1);

	hm[1] = 20;
	REQUIRE( hm[1] == 20 );

	++(*hm.begin()).second;
	REQUIRE( hm[1] == 21 );

	hm.begin()->second *= 2;
	REQUIRE( hm[1] == 42 );
}
