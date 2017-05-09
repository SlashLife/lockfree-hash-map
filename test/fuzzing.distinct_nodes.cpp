#include <cstdint>

#include <atomic>
#include <limits>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

#include "../include/hash_map.hpp"
#include "test_helper.hpp"

#define CATCH_CONFIG_MAIN
#include "../3rdparty/catch.hpp"

TEST_CASE("hash_map/fuzzing: distinct_nodes", "") {
	// This test will concurrently run several threads on the same
	// hash_map, randomly (creating and) incrementing or deleting
	// nodes. However, each thread has a distinct set of node keys,
	// so each particular node will only be created or deleted by
	// the same thread. However, it may be modified by other threads
	// adding or removing adjacent nodes.
	
	// NOTE: This test runs with high congenstion.
	//       This is INTENTIONAL to get high interference between threads
	//       to test the thread safety of the concurrent operations.
	
	// Operations tested: erase(), find(), insert(),
	//                    insert_or_assign() and equal_range()
	// All other thread safe operations will either not touch the
	// node list at all or do so though one of these operations.
	
	constexpr auto nodes_per_thread = 100U;
	constexpr auto iterations_per_thread = 25'000ULL;
	constexpr auto kill_chance = 10U; // kill node in 1 in n cases
	const auto num_threads = std::thread::hardware_concurrency();

	std::cout <<
		"Running " << num_threads << "\n"
		"   with " << nodes_per_thread << " nodes each\n"
		"   for  " << iterations_per_thread << " iterations each.\n"
		"NOTE: This test runs with high congestion (by design), so\n"
		"      this may take several minutes." << std::endl;
	

	std::random_device rd;
	std::mt19937_64 rand(std::uniform_int_distribution<std::uint_fast64_t>(
		0, ~static_cast<std::uint_fast64_t>(0)
	)(rd));
	
	// nodes will only accessed by one thread, so value_type does
	// not need to be thread safe itself.
	hash_map<unsigned, std::uint_fast32_t> hm(
		std::uniform_int_distribution<>(4,12)(rand) // 1-10 buckets
	);
	
	std::cout << "Hash map has " << hm.bucket_count() << " buckets." << std::endl;

	// need big enough number space
	REQUIRE(
		nodes_per_thread * num_threads <=
		std::numeric_limits<decltype(hm)::key_type>::max()
	);

	std::vector<std::vector<std::uint_fast32_t>> data_tracker(
		num_threads,
		std::vector<std::uint_fast32_t>(nodes_per_thread)
	);

	// set up threads
	// let them all start at once
	std::vector<std::thread> threads;
	std::mutex thread_start_mutex;
	{ std::unique_lock<std::mutex> lock(thread_start_mutex);
		unsigned thread_id = 0;
		while(threads.size() < num_threads) {
			threads.emplace_back([&,thread_id](){
				std::cout << "Thread " << thread_id << " created." << std::endl;
				// might not be thread safe, so we'll grab our own 
				std::mt19937_64 rng(std::uniform_int_distribution<std::uint_fast64_t>(
					0, ~static_cast<std::uint_fast64_t>(0)
				)(rd));
				std::uniform_int_distribution<unsigned> node_dist(
					0,
					nodes_per_thread - 1
				);
				std::uniform_int_distribution<unsigned> kill_dist(
					0,
					kill_chance - 1
				);
				
				std::uint_fast32_t n = iterations_per_thread;
				const unsigned base_node_id = nodes_per_thread * thread_id;
				std::vector<std::uint_fast32_t> &my_tracker = data_tracker[thread_id];
				
				// sync start
				{ std::unique_lock<std::mutex> lock(thread_start_mutex); }
				std::cout << "Thread " << thread_id << " started." << std::endl;
				
				// n node kills/increments
				while(n--) {
					if (n && 0 == n%2500) {
						std::cout << "Thread " << thread_id << ": " << n << " iterations left." << std::endl;
					}
					
					const unsigned node_offset = node_dist(rng);
					const unsigned node_id = base_node_id + node_offset;
					
					if (0 == kill_dist(rng)) {
						// kill node
						// test: hm.erase(), hm.find(), hm.equal_range();
						auto it_pair = hm.equal_range(node_id);
						if (it_pair.first != it_pair.second) {
							// found an element, so find will succeed, too
							hm.erase(hm.find(node_id));
							my_tracker[node_offset]=0;
						}
					}
					else {
						// increment node
						// tests hm.insert() (called by op[]), hm.insert_or_assign()
						hm.insert_or_assign(node_id, hm[node_id]+1);
						++my_tracker[node_offset];
					}
				}
				
				std::cout << "Thread " << thread_id << " stopped." << std::endl;
			});
			++thread_id;
		}
	}
	
	// wait for all threads to be done
	while(!threads.empty()) {
		threads.back().join();
		threads.pop_back();
	}
	std::cout << "All threads finished.\n";

	// check if tracked data matches the contents of the hash_map
	for(auto thread_id = 0U; thread_id < data_tracker.size(); ++thread_id) {
		for(auto node_offset = 0U; node_offset < data_tracker[thread_id].size(); ++node_offset) {
			const auto count = data_tracker[thread_id][node_offset];
			const auto node_id = thread_id * nodes_per_thread + node_offset;
			const auto it = hm.find(node_id);

			if (count) {
				REQUIRE( it != hm.end() );
				REQUIRE( it->first == node_id );
				REQUIRE( it->second == count );
			}
			else {
				REQUIRE( it == hm.end() );
			}
		}
	}
}
