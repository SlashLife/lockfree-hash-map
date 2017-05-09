// This implementation was done in response to an assignment for a job interview.
// Production use is discouraged!

#pragma once

#ifndef HASH_MAP_HPP_INCLUDED
#define HASH_MAP_HPP_INCLUDED

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>

/** \brief A concurrency friendly hash map.
 * \nosubgrouping
 *
 * \tparam Key The type for element keys.
 * \tparam T The type for element values.
 * \tparam Hash The type of the hash function.
 * \tparam KeyEqual The type of the key equality comparator.
 * \tparam Allocator The type of the allocator.
 */
template<
	typename Key,
	typename T,
	typename Hash = std::hash<Key>,
	typename KeyEqual = std::equal_to<Key>,
	typename Allocator = std::allocator< std::pair<const Key, T> >
>
struct hash_map {
private:
/// \name Member Types
///\{
	struct node;
	/// \internal \brief The smart pointer type used to hold nodes.
	typedef typename node::pointer node_pointer;

	struct fixed_size_bucket_list;
	/// \internal \brief A smart pointer to a bucket list.
	typedef std::shared_ptr<fixed_size_bucket_list> bucket_list_pointer;

public:
	/// \brief The type used for element counts and indices.
	typedef std::size_t                 size_type;

	/// \brief The difference type of two iterators.
	typedef std::ptrdiff_t              difference_type;

	/// \brief The storage type stored in the map.
	typedef std::pair<const Key, T>     value_type;

	/// \brief The type for element keys.
	typedef Key                         key_type;

	/// \brief The type for element values.
	typedef T                           mapped_type;

	/// \brief The type of the hash function.
	typedef Hash                        hasher;

	/// \brief The type of the key equality comparator.
	typedef KeyEqual                    key_equal;

	/// \brief The type of the allocator.
	typedef Allocator                   allocator_type;

	/// \brief The return type of the hash function.
	typedef std::result_of_t<Hash(Key)> hash_type;

	static_assert( std::numeric_limits<hash_type>::is_integer,
		"Hash result type must be an unsigned integer type." );
	static_assert( !std::numeric_limits<hash_type>::is_signed,
		"Hash result type must be an unsigned integer type." );

	/// \brief The base class for iterator implementations.
	template<typename ReferredT, bool IsConst, bool IsLocal>
	struct iterator_impl {
		friend class hash_map;

	private:
		/// \internal \brief A pointer to refer to a node.
		typedef node* node_pointer;

		/** \internal \brief Creates an iterator from a node pointer.
		 *
		 * \param pnode A pointer to the node to refer to by the iterator or
		 *     \c nullptr to create an invalid or end pointer.
		 */
		explicit iterator_impl(node_pointer pnode)
		: pnode(pnode) {}

	public:
		/// \brief The non-const, non-local iterator type for this iterator.
		typedef iterator_impl<
			hash_map::value_type, false, false> base_iterator_type;

		/// \brief This iterator is a forward iterator.
		typedef std::forward_iterator_tag iterator_category;

		/// \brief The difference type of two iterators.
		typedef std::ptrdiff_t            difference_type;

		/// \brief The storage type stored in the map.
		typedef ReferredT                 value_type;

		/// \brief A pointer to the storage type stored in the map.
		typedef value_type*               pointer;

		/// \brief A reference to the storage type stored in the map.
		typedef value_type&               reference;

		/// \brief Default constructs an iterator.
		iterator_impl()
		: iterator_impl(nullptr) {}

		/** \brief Copies an iterator.
		 *
		 * \tparam OtherReferredT The type referred to by the other iterator.
		 *     Must be either \c value_type or <tt>const value_type</tt>.
		 * \tparam OtherConst Whether the other iterator is a const iterator.
		 * \tparam OtherLocal Whether the other iterator is a local iterator.
		 *
		 * \param other The base iterator to copy from.
		 *
		 * \note This overload is not available if the iterator copied from is
		 *     less lenient in one dimension, i.e. is const or is local when
		 *     the constructed iterator is not.\n
		 *     To copy from such an iterator, use <tt>other.base()</tt>.
		 *
		 * \note If <tt>IsLocal != OtherLocal</tt> and \c other is an end
		 *     iterator, the result is undefined.
		 */
		template<typename OtherReferredT, bool OtherConst, bool OtherLocal
#ifndef DOXYGEN
			, typename = std::enable_if_t<
				( // has correct type
					std::is_same<
						std::remove_const_t<OtherReferredT>,
						std::remove_const_t<     ReferredT>
					>::value
				) &&
				(OtherConst <= IsConst) && // we're at least as restrictive
				(OtherLocal <= IsLocal)    // we're at least as restrictive
			, void>
#endif
		>
		iterator_impl(
			const iterator_impl<OtherReferredT, OtherConst, OtherLocal> &other
		)
		: iterator_impl(other.pnode) {
			assert( (IsLocal==OtherLocal || (pnode && !pnode->is_sentinel()))
				&& "must not convert between local and non-local end iterator" );
		}

		/** \brief Assigns the iterator to another iterators element.
		 *
		 * \param other The iterator from which to assign.
		 *
		 * \return Returns itself.
		 */
		iterator_impl &operator=(const iterator_impl &other) {
			pnode = other.pnode;
			return *this;
		}

		/** \brief Converts the iterator to a base iterator.
		 *
		 * \return A non-const, non-local iterator to the same element.
		 *
		 * \note If the current iterator is a local end() iterator, the result
		 *     is undefined.
		 */
		base_iterator_type base() const {
			return base_iterator_type(pnode);
		}

		/** \brief Advances the iterator to the next element.
		 *
		 * \pre
		 *     - <tt>*this</tt> is valid,
		 *     - <tt>*this != end()</tt>.
		 *
		 * \return Returns itself after advancing.
		 *
		 * \post
		 *     - <tt>*this</tt> refers to the next element.
		 */
		iterator_impl &operator++() {
			assert( pnode && "cannot increment an end iterator" );
			std::shared_ptr<node> cur = std::atomic_load(&pnode->next);
			if (!IsLocal) {
				while(cur && cur->is_sentinel()) {
					// returns the next bucket or nullptr if this was the last.
					const auto *bucket = cur->next_bucket();
					cur = (bucket)
						? std::atomic_load(&bucket->sentinel->next) // first data
							//  node or the sentinel itself if the bucket is empty
						: nullptr; // no more bucket
				}
			}
			pnode = cur.get();

			return *this;
		}

		/** \brief Advances this iterator and returns the previous value.
		 *
		 * \pre
		 *     - <tt>*this</tt> is valid,
		 *     - <tt>*this != end()</tt>.
		 *
		 * \return An iterator to the element this iterator referred to before
		 *     the call.
		 *
		 * \post
		 *     - <tt>*this</tt> refers to the next element,
		 *     - <tt>++return_value == *this</tt>.
		 */
		iterator_impl operator++(int) {
			iterator_impl copy(*this);
			++*this;
			return copy;
		}

		/** \brief Compares two iterators.
		 *
		 * \tparam ORefT The \c ReferredT of the other iterator.
		 * \tparam OConst The \c IsConst of the other iterator.
		 * \tparam OLocal The \c IsLocal of the other iterator.
		 *
		 * \param other Another iterator.
		 *
		 * \return
		 *     - \c true if this iterator equals \c other,
		 *     - \c false otherwise.
		 */
		template<typename ORefT, bool OConst, bool OLocal>
		bool operator==(const iterator_impl<ORefT, OConst, OLocal> &other) const {
			return pnode == other.pnode;
		}

		/** \brief Compares two iterators.
		 *
		 * \tparam ORefT The \c ReferredT of the other iterator.
		 * \tparam OConst The \c IsConst of the other iterator.
		 * \tparam OLocal The \c IsLocal of the other iterator.
		 *
		 * \param other Another iterator.
		 *
		 * \return
		 *     - \c true if this iterator differs from \c other,
		 *     - \c false otherwise.
		 */
		template<typename ORefT, bool OConst, bool OLocal>
		bool operator!=(const iterator_impl<ORefT, OConst, OLocal> &other) const {
			return pnode != other.pnode;
		}

		/** \brief Compares two iterators.
		 *
		 * \tparam ORefT The \c ReferredT of the other iterator.
		 * \tparam OConst The \c IsConst of the other iterator.
		 * \tparam OLocal The \c IsLocal of the other iterator.
		 *
		 * \param other Another iterator.
		 *
		 * \return
		 *     - \c true if this iterator comes before \c other,
		 *     - \c false otherwise.
		 */
		template<typename ORefT, bool OConst, bool OLocal>
		bool before(const iterator_impl<ORefT, OConst, OLocal> &other) const {
			return std::less<node_pointer>()(pnode, other.pnode);
		}

		/** \brief Dereferences the iterator.
		 *
		 * \return A reference to the iterators referred element.
		 */
		reference operator*() const {
			assert( pnode && "cannot dereference invalid iterator" );
			return pnode->data();
		}

		/** \brief Dereferences the iterator.
		 *
		 * \return A pointer to the iterators referred element.
		 */
		pointer operator->() const {
			assert( pnode && "cannot dereference invalid iterator" );
			return &pnode->data();
		}

	private:
		friend class std::less<iterator_impl>;
		/// \internal \brief operator< provided for std::less
		bool operator<(const iterator_impl &other) const {
			return before(other);
		}

		/// \internal \brief A pointer to the node the iterator refers to.
		node_pointer pnode;
	};

	/// \brief The iterator type for the hash_map.
	typedef iterator_impl<      value_type, false, false> iterator;

	/// \brief The constant iterator type for the hash_map.
	typedef iterator_impl<const value_type, true , false> const_iterator;

	/// \brief The local iterator type for the hash_map.
	typedef iterator_impl<      value_type, false, true > local_iterator;

	/// \brief The constant local iterator type for the hash_map.
	typedef iterator_impl<const value_type, true , true > const_local_iterator;
///\}



/// \name Member Functions
///\{
	/** \brief Creates an empty hash_map.
	 *
	 * \param bucket_count The number of buckets used initially.
	 * \param hash The hash function to use.
	 * \param keycomp The key comparison function to use.
	 * \param allocator The allocator to use.
	 *
	 * \pre
	 *     - <tt>0 < bucket_count</tt>
	 */
	explicit hash_map(
		const size_type bucket_count,
		const hasher &hash = hasher{},
		const key_equal &keycomp = key_equal{},
		const allocator_type &allocator = allocator_type{}
	)
	: current_buckets(fixed_size_bucket_list::create(
		bucket_count, hash, keycomp, allocator
	)) {
		assert( 0 < bucket_count
			&& "can not have a hash_map without buckets" );
	}

	/** \brief Creates a copy of a hash_map.
	 *
	 * \post
	 *     - <tt>*this == other</tt>
	 */
	hash_map(const hash_map &other)
	: current_buckets(fixed_size_bucket_list::create(
		other.current_buckets->bucket_count,
		other.current_buckets->hash,
		other.current_buckets->keycomp,
		other.current_buckets->allocator
	)) {
		// copy node bucket by bucket
		const size_type bucket_count = current_buckets->bucket_count;
		for(size_type b_id=0; b_id < bucket_count; ++b_id) {
			const node_pointer sentinel
				= current_buckets->buckets[b_id].sentinel;

			const_local_iterator begin = other.cbegin(b_id);
			const const_local_iterator end = other.cend(b_id);

			node_pointer prev = sentinel;
			while(begin != end) {
				node_pointer new_node =
					node::create_with_data(current_buckets->allocator, *begin);
				prev->next = new_node;
				prev = new_node;
				// buckets list ends in nullptr, but buckets destructor can
				// cope with that, should an exception be thrown.

				++begin;
			}
			prev->next = sentinel; // close the circle
		}
		current_buckets->node_count = other.size();
	}

	/** \brief Destructs the hash_map.
	 *
	 * \post
	 *     - All iterators are invalidated.
	 */
	~hash_map() = default;

	/** \brief Assigns all elements from another hash_map to this one.
	 *
	 * \return A reference to this hash_map.
	 *
	 * \post
	 *     - <tt>*this == other</tt>
	 */
	hash_map &operator=(const hash_map &other) {
		hash_map temp(other);
		swap(temp);
		return *this;
	}

	/** \brief Swaps contents with another hash_map.
	 *
	 * All meta data (bucket count, allocator, hash function, key comparator)
	 * will also be swapped.
	 *
	 * \param other The hash_map to swap contents with.
	 *
	 * \post
	 *     - <tt>*this == other_before_swap</tt>
	 *     - <tt>other == this_before_swap</tt>
	 *     - The allocator, hasher, key comparator and all elements from \c lhs
	 *         are now in \c rhs and vice versa.
	 *     - No swap or assignment of the allocators, hashers, key comparators
	 *         any of the elements of \c lhs or \c rhs has occurred.
	 *     - All iterators to any elements of \c lhs or \c rhs are still valid
	 *         (but are now associated with the opposite hash_map).
	 */
	void swap(hash_map &other) {
		// note: This is SOMEWHAT* thread safe (as opposed to relying on
		//       std::swap), but not atomic!
		// *) functions working on an element level will work reliably on
		//    either of the hash_maps (which one exactly is unspecified),
		//    but a badly timed concurrent call to clear() may be negated
		//    by this. Observers may also find both maps to be the same.
		//
		//    For all of these reasons this function is not *actually*
		//    considered thread safe.

		// atomic:
		//     temp = other.current_buckets;
		//     other.current_buckets = current_buckets;
		bucket_list_pointer temp = std::atomic_exchange(
			&other.current_buckets,
			current_buckets
		);

		// atomic:
		//     current_buckets = temp;
		std::atomic_store(&current_buckets, temp);
	}

	/** \brief Compares the values in the hash_map.
	 *
	 * \param other Another hash_map to compare against.
	 *
	 * \pre
	 *     - \c value_type is <tt>==</tt> comparable.
	 *
	 * \return
	 *     - \c true if the contents of the containers are equal,
	 *     - \c false otherwise.
	 */
	bool operator==(const hash_map &other) const {
		if (
			this == &other ||
			(empty() && other.empty())
		) {
			// always compare equal with self or
			// when both are empty
			return true;
		}

		if (size() != other.size()) {
			// can't be equal
			return false;
		}

		// could introduce a mapped_equal type for this as well, but not even
		// std::unordered_map does it, and it would be an arbitrary decision
		// which hash_maps comparators to use if they differ ...
		// ... so operator==(value_type, value_type) will have to suffice.

		if (is_bucket_comparable_to(other)) {
			// elements are in the equivalent buckets in both hash_maps
			// this can significantly reduce the complexity of the comparisons

			const size_type bucket_count = current_buckets->bucket_count;
			for(size_type b_id=0; b_id < bucket_count; ++b_id) {
				// bucket_size(b_id) is O(N) ... would like to avoid doing this
				// manually and leave it to std::is_permutation, however it is
				// only guaranteed to do a distance check for random access
				// iterators - ours are forward iterators.
				if (
					bucket_size(b_id) != other.bucket_size(b_id) ||
					!std::is_permutation(
						this->cbegin(b_id), this->cend(b_id),
						other.cbegin(b_id), other.cend(b_id)
					)
				) {
					return false;
				}
			}
			return true;
		}
		else {
			// need to do the full thing, unfortunately
			return std::is_permutation(
				this->cbegin(), this->cend(),
				other.cbegin(), other.cend()
			);
		}
	}

	/** \brief Compares the values in the hash_map.
	 *
	 * \param other Another hash_map to compare against.
	 *
	 * \pre
	 *     - \c value_type is <tt>==</tt> comparable.
	 *
	 * \return
	 *     - \c true if the contents of the containers differ,
	 *     - \c false otherwise.
	 */
	bool operator!=(const hash_map &other) const {
		return !operator==(other);
	}

	/** \brief Changes the bucket count and rehashes the elements.
	 *
	 * \param new_bucket_count The new number of buckets after rehashing.
	 *
	 * \pre
	 *     - <tt>0 < new_bucket_count</tt>
	 *
	 * \post
	 *     - <tt>bucket_count() == new_bucket_count</tt>
	 *     - <tt>after_rehash == before_rehash</tt>
	 *
	 * \note If any allocations fail in the process, the value of the hash_map
	 *     will be unchanged.
	 *
	 * \note If the hash function throws during this operation, the behavior is
	 *     undefined.
	 */
	void rehash(size_type new_bucket_count) {
		assert( 0 < new_bucket_count
			&& "can not rehash without buckets" );

		if (new_bucket_count == current_buckets->bucket_count) {
			// nothing to do
			return;
		}

		bucket_list_pointer new_buckets
			= fixed_size_bucket_list::create(
				new_bucket_count,
				current_buckets->hash,
				current_buckets->keycomp,
				current_buckets->allocator
			);

		// if we have reached this point, nothing bad will be happening.
		// all memory required is allocated already, the rest is pointer
		// manipulation and hash calculation / key comparison.

		auto begin = current_buckets->buckets;
		const auto end = begin + current_buckets->bucket_count;
		while (begin != end) {
			node_pointer cur = begin->sentinel->next;

			// unlink the list from the old bucket.
			begin->sentinel->next = begin->sentinel;

			// for all data nodes ...
			while(!cur->is_sentinel()) {
				node_pointer next = cur->next;

				// find target bucket and insert node.
				// rehashing likely changes the node sorting anyway, so we
				// won't put extra effort into preserving the node order, but
				// just insert in the beginning.
				const auto &target_bucket = new_buckets
					->bucket_for_key(cur->data().first);

				cur->next = target_bucket.sentinel->next;
				target_bucket.sentinel->next = cur;
				++new_buckets->node_count;

				cur = next;
				assert( cur
					&& "must not encounter null node during rehashing!");
			}

			++begin;
		}

		new decltype(current_buckets)(current_buckets); // leak
		current_buckets = new_buckets;
	}
///\}



/// \name Observers
///\{
	/** \brief Returns the allocator.
	 *
	 * \return The allocator used by this hash_map.
	 */
	allocator_type get_allocator() const {
		return current_buckets->allocator;
	}

	/** \brief Returns the hash function.
	 *
	 * \return The hash function used by this hash_map.
	 */
	hasher hash_function() const {
		return current_buckets->hash;
	}

	/** \brief Returns the key comparison function.
	 *
	 * \return The key comparison function used by this hash_map.
	 */
	key_equal key_eq() const {
		return current_buckets->keycomp;
	}
///\}



/// \name Iterators
///\{
	/** \brief Returns a begin iterator.
	 *
	 * \return An iterator to the first element.
	 */
	iterator begin() {
		typedef const typename fixed_size_bucket_list::bucket *bucket_pointer;
		bucket_pointer
			current_bucket = current_buckets->buckets;
		const bucket_pointer
			end_bucket = current_bucket + current_buckets->bucket_count;
		while(current_bucket != end_bucket) {
			node_pointer first = std::atomic_load(&current_bucket->sentinel->next);
			if (!first->is_sentinel()) {
				return iterator(first.get());
			}
			++current_bucket;
		}
		return iterator(nullptr);
	}

	/** \brief Returns a begin iterator.
	 *
	 * \return An iterator to the first element.
	 */
	const_iterator begin() const {
		return cbegin();
	}

	/** \brief Returns a begin iterator.
	 *
	 * \return An iterator to the first element.
	 */
	const_iterator cbegin() const {
		return const_iterator(const_cast<hash_map&>(*this).begin());
	}

	/** \brief Returns an end iterator.
	 *
	 * \return An iterator past the last element.
	 */
	iterator end() {
		return iterator(nullptr);
	}

	/** \brief Returns an end iterator.
	 *
	 * \return An iterator past the last element.
	 */
	const_iterator end() const {
		return cend();
	}

	/** \brief Returns an end iterator.
	 *
	 * \return An iterator past the last element.
	 */
	const_iterator cend() const {
		return const_iterator(nullptr);
	}
///\}



/// \name Capacity
///\{
/// \note These functions are thread safe.
	/** \brief Checks whether the container is empty.
	 *
	 * \return
	 *     - \c true if the container is empty,
	 *     - \c false otherwise.
	 */
	bool empty() const {
		return 0 == size();
	}

	/** \brief Returns the number of elements.
	 *
	 * \return The number of elements in the container.
	 */
	size_type size() const {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		return buckets->node_count;
	}

	/** \brief Returns the maximum possible number of elements.
	 *
	 * \return The maximum number of possible elements in the container.
	 */
	size_type max_size() const {
		return std::numeric_limits<size_type>::max();
	}
///\}



/// \name Modifiers
///\{
/// \note These functions are thread safe.
	/** \brief Clears the contents.
	 *
	 * \post
	 *     - <tt>empty() == true</tt>
	 *     - <tt>size() == 0</tt>
	 *     - All iterators to this hash_map are invalidated.
	 */
	void clear() {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		bucket_list_pointer new_buckets = fixed_size_bucket_list::create(
			buckets->bucket_count,
			buckets->hash,
			buckets->keycomp,
			buckets->allocator
		);

		// If the following operation fails, the hash_map has been resized or
		// concurrently cleared. In either case there is no need to try again:
		// A concurrent clear will already have perfomed the desired operation,
		// a resize() is not thread safe, so the result of this operation is
		// undefined - retaining the resized version is a sane option for
		// implementing this UB.
		std::atomic_compare_exchange_strong(
			&current_buckets, &buckets, new_buckets
		);
	}

	/** \brief Inserts an element into the map.
	 *
	 * \param value The value to insert into the map.
	 *
	 * \return A pair \c pair of a boolean as follows:
	 *     - <tt>pair.first == true</tt>, if \c value was inserted
	 *         successfully. <tt>pair.second</tt> will be an iterator to the
	 *         newly inserted element.
	 *     - <tt>pair.first == false</tt>, if an item with the given key exists
	 *         already. The state of the \c hash_map was not modified by this
	 *         operation. <tt>pair.second</tt> will be an iterator to the
	 *         element that blocked the insertion.
	 *
	 * \post
	 *     - If no element was inserted, the state of the \c hash_map did not
	 *         change. An instance of \c value_type may have been temporarily
	 *         constructed.
	 *     - If an element was inserted, the \c hash_map contains one more
	 *         element. An instance of \c value_type was constructed and
	 *         <tt>size() := size() + 1</tt>.
	 */
	std::pair<bool, iterator> insert(const value_type &value) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		node_pointer new_node;
		node_pointer prev, cur;
		while(true) {
			if (buckets->find(value.first, prev, cur)) {
				return std::make_pair(false, iterator(cur.get()));
			}
			else {
				assert( cur->is_sentinel()
					&& "will only append to the end of a list!" );

				if (!new_node) {
					new_node = node::create_with_data(
						buckets->allocator,
						value
					);
				}

				// configure the node for insertion at this place
				new_node->next = cur;

				// current situataion:
				//
				// ... --> prev --(expected)--> cur (= end of list)
				//                               ^
				//                 new_node -----+
				//
				// now attempt to relink prev->next to new_node, but ONLY
				// if it is still pointing to cur; otherwise someone else
				// beat us to it and we have to retry!
				if (std::atomic_compare_exchange_weak(
					// this invalidates cur, but we have no use for it after
					// this call anyway; either we're done and don't need it,
					// or we need to start the search again and don't need it.
					&prev->next, &cur, new_node
				)) {
					++buckets->node_count;
					return std::make_pair(true, iterator(new_node.get()));
				}

				// someone beat us to it - tough luck; reset and try again ...
				// ... in a moment
				std::this_thread::yield();
			}
		}
	}

	/** \brief Inserts an element into the map.
	 *
	 * This function is equivalent to calling <tt>insert(value)</tt>. This
	 * overload is provided mainly for compatibility with
	 * <tt>std::insert_iterator</tt> and algorithms.
	 *
	 * \param hint Ignored.
	 * \param value The value to insert into the map.
	 *
	 * \return An iterator to the newly inserted element or to the existing
	 *     element with they same key as \c value that blocked the insertion.
	 *
	 * \post
	 *     - If no element was inserted, the state of the \c hash_map did not
	 *         change. An instance of \c value_type may have been temporarily
	 *         constructed.
	 *     - If an element was inserted, the \c hash_map contains one more
	 *         element. An instance of \c value_type was constructed and
	 *         <tt>size() := size() + 1</tt>.
	 */
	iterator insert(const_iterator hint, const value_type &value) {
		((void)hint); // unused, suppress warning
		return insert(value).second;
	}

	/** \brief Inserts an element into the map or modifies an existing one.
	 *
	 * \param key The key of the element in the map.
	 * \param mapped The value to insert or assign.
	 *
	 * \return An iterator to the element with key \c key.
	 *
	 * \post
	 *     - <tt>(*this)[key] == mapped</tt>
	 *     - If an element was inserted, the \c hash_map contains one more
	 *         element. An instance of \c value_type was constructed and
	 *         <tt>size() := size() + 1</tt>.
	 */
	iterator insert_or_assign(const key_type &key, const mapped_type &mapped) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		node_pointer new_node;
		node_pointer prev, cur;
		while(true) {
			if (buckets->find(key, prev, cur)) {
				cur->data().second = mapped;
				return iterator(cur.get());
			}
			else {
				assert( cur->is_sentinel()
					&& "will only append to the end of a list!" );

				if (!new_node) {
					new_node = node::create_with_data(
						buckets->allocator,
						std::make_pair(key, mapped)
					);
				}

				// configure the node for insertion at this place
				new_node->next = cur;

				// current situataion:
				//
				// ... --> prev --(expected)--> cur (= end of list)
				//                               ^
				//                 new_node -----+
				//
				// now attempt to relink prev->next to new_node, but ONLY
				// if it is still pointing to cur; otherwise someone else
				// beat us to it and we have to retry!
				if (std::atomic_compare_exchange_weak(
					// this invalidates cur, but we have no use for it after
					// this call anyway; either we're done and don't need it,
					// or we need to start the search again and don't need it.
					&prev->next, &cur, new_node
				)) {
					++buckets->node_count;
					return iterator(new_node.get());
				}

				// someone beat us to it - tough luck; reset and try again ...
				// ... in a moment
				std::this_thread::yield();
			}
		}
	}

	/** \brief Removes an element from the hash_map by its key.
	 *
	 * \param key The key of the element in the hash_map.
	 *
	 * \return The number of elements erased from the hash_map (0 or 1).
	 *
	 * \post
	 *     - <tt>find(key) == end()</tt>
	 *     - If an element was erased, the \c hash_map contains one less
	 *         element and <tt>size() := size() - 1</tt>.
	 *     - All iterators to the erased node are invalidated.
	 *
	 * \note Other concurrent operations may still refer to the node, so the
	 *     erased element is not necessarily destructed by the time this
	 *     function returns.
	 */
	size_type erase(const key_type &key) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		node_pointer prev, cur;
		while(true) {
			if (!buckets->find(key, prev, cur)) {
				return 0;
			}
			else {
				node_pointer next = std::atomic_exchange(&cur->next, node_pointer{});
				if (!next) {
					// someone else is trying to delete this node at the same
					// time - we will leave them be, but we must not return yet
					// because at this point a subsequent find() may still find
					// the node and return it, which would be in violation with
					// our post-condition.
					// instead we'll check their progress ... in a moment
					std::this_thread::yield();
					continue;
				}

				// At this point, this node is marked for deletion.
				// All traversions made through this by thread-safe operations
				// have either completed successfully or will result in retrys
				// until the node is completely unlinked.
				// A potential insertion after this node will fail, because

				// current situataion:
				//
				// ... --> prev               next --> ...
				//            |               ^
				// (expected) +----> cur --//-+ (severed)

				// now attempt to relink prev->next to next, but ONLY
				// if it is still pointing to cur; otherwise someone else
				// has concurrently deleted prev or cur (only deletion is
				// possible, because insertion only happens at the end, i.e.
				// after this node) while we were preparing for deletion and
				// we need to retry.

				// we will need cur if the exchange fails, so we pass a copy:
				node_pointer expected_value = cur;
				if (std::atomic_compare_exchange_strong(
					&prev->next, &expected_value, next
				)) {
					--buckets->node_count;
					return 1;
				}

				// ruled out concurrent insertion
				//     (only happens at end of list)
				// ruled out concurrent erase of same node
				//     (subsequent operations retry on cur->next==nullptr),
				// prev->next can only change by erase(prev) - which means that
				// prev->next should be a nullptr. So we roll back.
				// But just as we roll back, so could erase(prev), if it
				// collided with an erase(prev->prev); which would then restore
				// prev->next. But then prev->next would be cur again, so the
				// only two possible values for expected_value are nullptr and
				// cur - but we atomically checked against cur, so only nullptr
				// is left as an option.
				// This needed to be documented, but is far too long for an
				// assertion message.

				assert( !expected_value && "failed to exchange prev->next, "
					"but prev->next is not a nullptr, either" );

				// someone beat us to it - tough luck; relink next to cur,
				std::atomic_store(&cur->next, next);
				// then reset and try again ... in a moment
				std::this_thread::yield();
			}
		}
	}

	/** \brief Removes an element from the hash_map by its iterator.
	 *
	 * \param pos An iterator to the element in the hash_map.
	 *
	 * \pre
	 *     - \c pos is a valid iterator to an element in the hash_map.
	 *     - no concurrent operation will invalidate \c pos during the
	 *         execution of this operation.
	 *
	 * \return An iterator to the element after the deleted one.
	 *
	 * \post
	 *     - <tt>find(key) == end()</tt>
	 *     - If an element was erased, the \c hash_map contains one less
	 *         element and <tt>size() := size() - 1</tt>.
	 *     - All iterators to the erased node are invalidated.
	 *
	 * \note Other concurrent operations may still refer to the node, so the
	 *     erased element is not necessarily destructed by the time this
	 *     function returns.
	 */
	iterator erase(const_iterator pos) {
		const iterator next = std::next(pos).base();
		const size_type num_erased = erase(pos->first); // remove by key
		((void)num_erased); // unused in non-debug build; suppress warning

		// Whether deleted by us or by someone else: The post-conditions are
		// met either way, so we don't particularly care about the result of
		// *our* operation.
		//
		// HOWEVER: If our operation was not the one that deleted the node, it
		// indicates that another thread concurrently deleted the node, thereby
		// invalidating the iterator passed to us, violating the pre-condition
		// in the process.
		//
		// As this invalidation happens independently of this function, it
		// indicates the use of possibly invalidated iterators by the caller,
		// which is undefined behavior and needs to be reported:
		assert( num_erased != 0
			&& "undefined behavior detected: pos was invalidated concurrently"
				"during call to hash_map::erase(iterator)!" );

		return next;
	}
///\}



/// \name Lookup
///\{
/// \note These functions are thread safe.
	/** \brief Accesses an element by its key, with bounds-checking.
	 *
	 * \param key The key of the element to access.
	 *
	 * \throw <tt>std::out_of_range</tt> if no element with the key \c key is
	 *     stored in the hash_map.
	 *
	 * \return A reference to the element requested.
	 */
	mapped_type &at(const key_type &key) {
		iterator it = find(key);
		if (it != end()) {
			return it->second;
		}
		else {
			throw std::out_of_range("element not found in hash_map");
		}
	}

	/** \brief Accesses an element by its key, with bounds-checking.
	 *
	 * \param key The key of the element to access.
	 *
	 * \throw <tt>std::out_of_range</tt> if no element with the key \c key is
	 *     stored in the hash_map.
	 *
	 * \return A constant reference to the element requested.
	 */
	const mapped_type &at(const key_type &key) const {
		const_iterator it = find(key);
		if (it != cend()) {
			return it->second;
		}
		else {
			throw std::out_of_range("element not found in hash_map");
		}
	}

	/** \brief Accesses an element by its key.
	 *
	 * If necessary, default constructs an element first to return it.
	 *
	 * \param key The key of the element to access.
	 *
	 * \return The element for the key requested.
	 *
	 * \note This function is only available if \c mapped_type is default
	 *     constructible. Use \ref insert_or_assign() otherwise.
	 */
#ifndef DOXYGEN
	std::enable_if_t<
		std::is_default_constructible<mapped_type>::value,
		mapped_type&
	>
#else
	mapped_type&
#endif
	operator[](const key_type &key) {
		auto result = insert(std::make_pair(key, mapped_type{}));
		return result.second->second;
	}

	/** \brief Counts the number of elements with a specific key.
	 *
	 * \param key The key of the element to count.
	 *
	 * \return The number of elements with the key \c key. (0 or 1)
	 */
	size_type count(const key_type &key) const {
		return (find(key) != cend())
			? 1
			: 0;
	}

	/** \brief Finds an element by its key.
	 *
	 * If necessary, default constructs an element first to return it.
	 *
	 * \param key The key of the element to fetch.
	 *
	 * \return An iterator to the element with the key \c key, or
	 *     <tt>end()</tt> is no such element exists.
	 */
	iterator find(const key_type &key) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		node_pointer prev, cur;
		if (buckets->find(key, prev, cur)) {
			return iterator(cur.get());
		}
		else {
			return end();
		}
	}

	/** \brief Finds an element by its key.
	 *
	 * If necessary, default constructs an element first to return it.
	 *
	 * \param key The key of the element to fetch.
	 *
	 * \return An iterator to the element with the key \c key, or
	 *     <tt>end()</tt> is no such element exists.
	 */
	const_iterator find(const key_type &key) const {
		return const_cast<hash_map&>(*this).find(key);
	}

	/** \brief Returns an iterator range for all elements with a specific key.
	 *
	 * \param key The key of the element to fetch.
	 *
	 * \return A pair of local iterators marking a range with all the elements
	 *     with the key \c key.
	 */
	// cannot thread safely find the next non-local iterator,
	// so this needs to return local iterators.
	std::pair<local_iterator, local_iterator> equal_range(const key_type &key) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		while(true) {
			node_pointer prev, cur;
			if (!buckets->find(key, prev, cur)) {
				// cur is the buckets sentinel, i.e. the end of the bucket
				return std::make_pair(
					local_iterator(cur.get()),
					local_iterator(cur.get())
				);
			}
			else if (node_pointer next = std::atomic_load(&cur->next)) {
				return std::make_pair(
					local_iterator(cur.get()),
					local_iterator(next.get())
				);
			}
			else {
				// cur->next was nullptr, which means the node is being erased
				// concurrently. We need to retry and check whether the node is
				// gone (or maybe a new node with the same key awaits us)
			}
		}
	}

	/** \brief Returns an iterator range for all elements with a specific key.
	 *
	 * \param key The key of the element to fetch.
	 *
	 * \return A pair of local iterators marking a range with all the elements
	 *     with the key \c key.
	 */
	// cannot thread safely find the next non-local iterator,
	// so this needs to return local iterators.
	std::pair<const_local_iterator, const_local_iterator> equal_range(const key_type &key) const {
		auto result = const_cast<hash_map&>(*this).equal_range(key);
		return std::pair<const_local_iterator, const_local_iterator>(
			result.first,
			result.second
		);
	}
///\}



/// \name Bucket interface
///\{
/// \note These functions are thread safe.
	/** \brief Returns a bucket local iterator to the beginning of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_iterator to the beginning of the bucket.
	 */
	local_iterator begin(size_type bucket_index) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		const typename fixed_size_bucket_list::bucket &bucket
			= buckets->buckets[bucket_index];
		return local_iterator(std::atomic_load(&bucket.sentinel->next).get());
	}

	/** \brief Returns a bucket local iterator to the beginning of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_const_iterator to the beginning of the bucket.
	 */
	const_local_iterator begin(size_type bucket_index) const {
		return cbegin(bucket_index);
	}

	/** \brief Returns a bucket local iterator to the beginning of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_const_iterator to the beginning of the bucket.
	 */
	const_local_iterator cbegin(size_type bucket_index) const {
		return const_cast<hash_map&>(*this).begin(bucket_index);
	}

	/** \brief Returns a bucket local iterator to the end of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_iterator to the end of the bucket.
	 */
	local_iterator end(size_type bucket_index) {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		const typename fixed_size_bucket_list::bucket &bucket
			= buckets->buckets[bucket_index];
		return local_iterator(bucket.sentinel.get());
	}

	/** \brief Returns a bucket local iterator to the end of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_const_iterator to the end of the bucket.
	 */
	const_local_iterator end(size_type bucket_index) const {
		return cend(bucket_index);
	}

	/** \brief Returns a bucket local iterator to the end of a bucket.
	 *
	 * \param bucket_index The index of the bucket for which to retrieve the
	 *     iterator.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return A \c local_const_iterator to the end of the bucket.
	 */
	const_local_iterator cend(size_type bucket_index) const {
		return const_cast<hash_map&>(*this).end(bucket_index);
	}

	/** \brief Returns the number of buckets.
	 *
	 * \return The number of buckets in this hash_map.
	 */
	size_type bucket_count() const {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		return buckets->bucket_count;
	}

	/** \brief Returns the maximum possible number of buckets.
	 *
	 * \return The maximum possible number of buckets in the container.
	 */
	size_type max_bucket_count() const {
		return std::numeric_limits<size_type>::max();
	}

	/** \brief Returns the number of elements in a specific bucket.
	 *
	 * \pre
	 *     - <tt>bucket_index < bucket_count()</tt>
	 *
	 * \return The number of elements in the given bucket.
	 */
	size_type bucket_size(size_type bucket_index) const {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		size_type count = 0;

		node_pointer cur = buckets->buckets[bucket_index].sentinel;
		while( !(cur = std::atomic_load(&cur->next))->is_sentinel() ) {
			++count;
		}

		return count;
	}

	/** \brief Returns the bucket index for a specific key.
	 *
	 * \param key The key for which to retrieve the according bucket index.
	 *
	 * \return The index of the bucket that would hold an element with the key
	 *     \c key.
	 */
	size_type bucket(const key_type &key) const {
		bucket_list_pointer buckets = std::atomic_load(&current_buckets);
		assert( buckets
			&& "can not work with an empty bucket list!" );

		return &buckets->bucket_for_key(key) - buckets->buckets;
	}
///\}



/// \internal \name Internals
///\{ \internal
private:
	/** \internal
	 * \brief Checks whether bucket-wise comparison with another
	 *     hash_map is possible.
	 *
	 * To allow bucket-wise comparison, equal elements must be in equivalent
	 * buckets. We can only guarantee this if both the <tt>bucket_count()</tt>
	 * and the <tt>hash_function()</tt> are equal.
	 *
	 * However, to even be able to assert this, the <tt>hash_functions()</tt>
	 * must be comparable in the first place.
	 *
	 * \return
	 *     - \c true if bucket-wise comparison is possible,
	 *     - \c false otherwise
	 */
	// enable_if is integral to this interface; do not hide from Doxygen here!
	template<typename HashMap>
	std::enable_if_t<sizeof(decltype(
		std::declval<hasher>() ==
		std::declval<typename HashMap::hasher>()
	)), bool> is_bucket_comparable_to(const HashMap &other) const {
		return
			other.current_buckets->bucket_count
				== current_buckets->bucket_count &&
			other.current_buckets->hash
				== current_buckets->hash;
	}

	/** \internal
	 * \brief Disallows bucket-wise comparison.
	 *
	 * This overload is a fallback, in case <tt>hasher</tt> is not comparable.
	 *
	 * \return <tt>false</tt>, because safe bucket-wise comparison cannot be
	 *     guaranteed.
	 */
	template<typename=void>
	bool is_bucket_comparable_to(const hash_map &) const {
		return false;
	}

	/// \internal \brief Represents a data or sentinel node inside a bucket.
	struct node {
		/// \internal \brief The smart pointer type used to hold nodes.
		typedef std::shared_ptr<node> pointer;

		/// \internal \brief The pointer type to point to buckets.
		typedef const typename fixed_size_bucket_list::bucket *bucket_pointer;

		/// \internal \brief A pointer to the next node in the bucket.
		pointer next;

		/** \internal \brief Accesses the data stored.
		 *
		 * \pre
		 *     - \c is_sentinel() returns \c false.
		 *
		 * \return A reference to the \c value_type
		 *      object stored in this node.
		 */
		const value_type &data() const {
			assert( !is_sentinel()
				&& "must not access data of a sentinel node" );
			return *reinterpret_cast<const value_type *>(data_);
		}

		/** \internal \brief Accesses the data stored.
		 *
		 * \pre
		 *     - \c is_sentinel() returns \c false.
		 *
		 * \return A reference to the \c value_type
		 *      object stored in this node.
		 */
		value_type &data() {
			assert( !is_sentinel()
				&& "must not access data of a sentinel node" );
			return *reinterpret_cast<value_type *>(data_);
		}

		/** \internal \brief Determines the next bucket.
		 *
		 * \pre
		 *     - \c is_sentinel() returns \c true.
		 *
		 * \return A pointer to the following bucket.
		 */
		bucket_pointer next_bucket() const {
			assert( is_sentinel()
				&& "can not get next bucket from a data node" );
			return *reinterpret_cast<const bucket_pointer *>(data_);
		}

		/** \internal \brief Checks whether the node is a sentinel node.
		 *
		 * \return
		 *     - \c true if this node is a sentinel node,
		 *     - \c false otherwise.
		 */
		bool is_sentinel() const noexcept {
			return !initialized;
		}

		/** \internal \brief Creates a sentinel node.
		 *
		 * \param alloc The allocator to use to allocate the node.
		 * \param next_bucket A pointer to the next bucket or \c nullptr if no
		 *     bucket follows.
		 *
		 * \return A \c node_pointer to the new node.
		 *
		 * \post
		 *     - \c is_data() returns \c false
		 *     - \c is_sentinel() returns \c true
		 *
		 * \note While this function does not throw itself, it will forward
		 *     exceptions thrown by the allocator called.
		 *
		 * \note While next_bucket needs to point to the location of the next
		 *     bucket, the bucket it not accessed from this function and thus
		 *     does not (yet) need to exist.
		 */
		static pointer create_sentinel(
			const allocator_type &alloc,
			bucket_pointer next_bucket
		) {
			pointer new_node = std::allocate_shared<node>(alloc);
			new (new_node->data_) bucket_pointer(next_bucket);
			return new_node;
		}

		/** \internal
		 * \brief Creates a data node.
		 *
		 * Creates a data node and emplaces a \c value_type object into it.
		 *
		 * \tparam Args Types of arguments passed to the constructor of
		 *     \c value_type.
		 *
		 * \param alloc The allocator to use to allocate the node.
		 * \param args These arguments are forwarded to the constructor of
		 *     \c value_type.
		 *
		 * \return A \c node_pointer to the new node.
		 *
		 * \post
		 *     - \c is_data() returns \c true
		 *     - \c is_sentinel() returns \c false
		 *     - \c data() returns a reference to the created \c value_type
		 *          object.
		 *
		 * \note While this function does not throw itself, it will forward
		 *     exceptions thrown by the allocator or the constructor called.
		 */
		template<typename... Args>
		static pointer create_with_data(
			const allocator_type &alloc,
			Args&&... args
		) {
			pointer new_node = std::allocate_shared<node>(alloc);
			new (new_node->data_) value_type(std::forward<Args>(args)...);
			new_node->initialized = true; // must come after initialization
				// to avoid calling the dtor on an uninitialized object in
				// case the ctor throws!
			return new_node;
		}

		/// \internal \brief Initializes an empty (sentinel) node.
		node() noexcept
		: next()
		, initialized(false) {}

		/** \internal
		 * \brief Destroys a node.
		 *
		 * If this node is a data node, the data object held by it will be
		 * destructed as well.
		 */
		~node() noexcept {
			// assumes no destructor throws - otherwise all hell is loose
			// during destruction of a non-empty hash_map anyway.
			if (is_sentinel()) {
				reinterpret_cast<bucket_pointer*>(data_)->~bucket_pointer();
			}
			else {
				data().~value_type();
			}
		}

	private:
		/** \internal \brief Indicates whether this nodes \c data_ member contains a
		 *     \c value_type object.
		 *
		 * \note Also governs whether the node is seen as a sentinel node or
		 *     a data node: A node is a data node iff initialized is true.
		 */
		bool initialized;

		/** \internal
		 * \brief Aligned storage for \c value_type.
		 *
		 * Doubles as the storage for a pointer to the next bucket
		 * in sentinel nodes to allow for lightweight iterators.
		 */
		alignas(value_type) alignas(bucket_pointer)
			char data_[std::max(sizeof(value_type), sizeof(bucket_pointer))];
	};

	/// \internal \brief Represents a bucket list.
	struct fixed_size_bucket_list {
		/// \internal \brief Stores a list of nodes for a reduced hash.
		struct bucket {
			/** \internal \brief Creates a bucket.
			 *
			 * \param allocator The allocator to use for allocating the
			 *     sentinel nodes.
			 * \param is_last Whether this is the last bucket in the list.
			 */
			bucket(const allocator_type &allocator, bool is_last)
			: sentinel(node::create_sentinel(
				allocator,
				/* next_bucket = */ is_last
					? nullptr
					: this + 1
			)) {
				// introducing a cyclic structure of owning pointers -
				// however on destruction of the bucket, all nodes in it will
				// get unlinked, so this won't leak.
				sentinel->next = sentinel;
			}

			// can't copy or assign buckets
			bucket(const bucket &) = delete;
			bucket &operator=(const bucket &) = delete;

			/** \internal
			 * \brief Destroys the bucket.
			 *
			 * All nodes held by the bucket are unlinked and, if not held by
			 * another entity, destructed in the process.
			 */
			~bucket() {
				node_pointer current = sentinel;
				while(current) {
					node_pointer next = current->next;
					current->next = nullptr;
					current = next;
				}
			}

			/** \internal \brief Finds the node for a key.
			 *
			 * \param key The key to look for.
			 * \param keycomp A comparator for key equality comparison.
			 * \param[out] prev A node_pointer to store a pointer to the
			 *     node before the found one in.
			 * \param[out] cur A node_pointer to store a pointer to the
			 *     found node in.
			 *
			 * \return
			 *     - \c true if \c key was found in the bucket,
			 *     - \c false if \c key was <em>not</em> found in the bucket.
			 *
			 * \post
			 *     - <tt>prev->next == cur</tt>, conceptually. This may change
			 *         if either node is concurrently removed.
			 *     - iff the return value is \c true:
			 *         <tt>cur->is_sentinel() == false</tt> and
			 *         <tt>keycomp(key, cur.data().first) == true</tt>.
			 *     - iff the return value is \c false:
			 *         <tt>cur->is_sentinel() == true</tt> and
			 *         <tt>cur == sentinel</tt>.
			 */
			bool find(
				const key_type &key,
				const key_equal &keycomp,
				node_pointer &prev,
				node_pointer &cur
			) const {
				while(true) {
					prev = sentinel;
					while(cur = std::atomic_load(&prev->next)) {
						if (cur->is_sentinel()) {
							assert(cur == this->sentinel
								&& "encountered alien sentinel node!");
							return false;
						}
						else if (keycomp(key, cur->data().first)) {
							return true;
						}
						else {
							prev = cur;
						}
					}
					// cur is nullptr: we ran into a node in the process of
					//     being unlinked.
					// -> try again from the start ... in a moment
					std::this_thread::yield();
				}
			}

			/// \internal \brief A sentinel node representing the front of the
			///     node list held by the bucket.
			const node_pointer sentinel;
		};

		/// \internal \brief The allocator for buckets.
		typedef typename std::allocator_traits<allocator_type>
			::template rebind_alloc<bucket> bucket_allocator_type;

		/// \internal \brief The allocator traits for buckets.
		typedef typename std::allocator_traits<allocator_type>
			::template rebind_traits<bucket> bucket_allocator_traits;

		/** \internal
		 * \brief Creates a bucket list.
		 *
		 * \param bucket_count The number of buckets in this list.
		 * \param hash The hash function used for keys.
		 * \param keycomp The comparison function used for keys.
		 * \param allocator The allocator to use for allocating the buckets.
		 */
		static bucket_list_pointer create(
			size_type bucket_count,
			const hasher &hash,
			const key_equal &keycomp,
			const allocator_type &allocator
		) {
			return std::allocate_shared<fixed_size_bucket_list, allocator_type>(
				allocator, bucket_count, hash, keycomp, allocator
			);
		}

		/** \internal \brief Creates a bucket list.
		 *
		 * \param bucket_count The number of buckets in this list.
		 * \param hash The hash function used for keys.
		 * \param keycomp The comparison function used for keys.
		 * \param allocator The allocator to use for allocating the buckets.
		 */
		fixed_size_bucket_list(
			size_type bucket_count,
			const hasher &hash,
			const key_equal &keycomp,
			const allocator_type &allocator
		)
		: bucket_count(bucket_count)
		, node_count(0)
		, hash(hash)
		, keycomp(keycomp)
		, allocator(allocator)
		, bucket_allocator(allocator)
		, buckets(bucket_allocator_traits::allocate(bucket_allocator, bucket_count)) {
			size_type n=0;
			try {
				// construct all buckets
				for(; n < bucket_count; ++n) {
					bucket_allocator_traits::construct(
						bucket_allocator,
						buckets + n,
						this->allocator,
						/* is_last = */ (bucket_count - n == 1)
					);
				}
			}
			catch(...) {
				// construction failed w/ exception; destruct all constructed
				// buckets in reverse order, dealloc bucket array and rethrow
				// note: skipping buckets[n] because it wasn't constructed.
				while(n) {
					--n;
					bucket_allocator_traits::destroy(
						bucket_allocator, buckets+n
					);
				}
				bucket_allocator_traits::deallocate(
					bucket_allocator, buckets, bucket_count
				);
				throw;
			}
		}

		fixed_size_bucket_list(const fixed_size_bucket_list &) = delete;
		fixed_size_bucket_list &operator=(const fixed_size_bucket_list &) = delete;

		/// \internal \brief Destroys the bucket list.
		~fixed_size_bucket_list() {
			// destruct all buckets in reverse order
			size_type n=bucket_count;
			while(n) {
				--n;
				bucket_allocator_traits::destroy(
					bucket_allocator, buckets+n
				);
			}
			// deallocate bucket list
			bucket_allocator_traits::deallocate(
				bucket_allocator, buckets, bucket_count
			);
		}

		/** \internal \brief Retrieves the bucket for a specific key value.
		 *
		 * \param key The key for which to find the associated bucket.
		 *
		 * \return The bucket associated with the key passed.
		 */
		const bucket &bucket_for_key(const key_type &key) const {
			return buckets[hash(key) % bucket_count];
		}


		/** \internal \brief Finds the node for a key.
		 *
		 * \param key The key to look for.
		 * \param[out] prev A node_pointer to store a pointer to the
		 *     node before the found one in.
		 * \param[out] cur A node_pointer to store a pointer to the
		 *     found node in.
		 *
		 * \return
		 *     - \c true if \c key was found in the map,
		 *     - \c false if \c key was <em>not</em> found in the map.
		 *
		 * \post
		 *     - <tt>prev->next == cur</tt>, conceptually. This may change
		 *         if either node is concurrently removed.
		 *     - iff the return value is \c true:
		 *         <tt>cur->is_sentinel() == false</tt> and
		 *         <tt>keycomp(key, cur.data().first) == true</tt>.
		 *     - iff the return value is \c false:
		 *         <tt>cur->is_sentinel() == true</tt> and
		 *         <tt>cur == sentinel</tt>.
		 */
		bool find(
			const key_type &key,
			node_pointer &prev,
			node_pointer &cur
		) const {
			return bucket_for_key(key).find(key, keycomp, prev, cur);
		}

		/// \internal \brief The number of buckets in the list.
		const size_type bucket_count;

		/// \internal \brief The current number of nodes in all buckets.
		std::atomic<size_type> node_count;

		/// \internal \brief The number of buckets in the list.
		const hasher hash;

		/// \internal \brief The comparator for element keys.
		const key_equal keycomp;

		/// \internal \brief The allocator used to handle allocations.
		const allocator_type allocator;

	private:
		/// \internal \brief The allocator used to handle bucket allocation.
		bucket_allocator_type bucket_allocator;

	public:
		/// \internal \brief A pointer to the start of the bucket list.
		bucket * const buckets; // pointer to array of bucket_count buckets.
	};

	/// \internal \brief Current bucket list.
	bucket_list_pointer current_buckets;
///\}
};


namespace std {
	/** \brief Swaps two hash_maps
	 *
	 * \tparam Key The \c key_type of the \c hash_map.
	 * \tparam T The \c mapped_type of the \c hash_map.
	 * \tparam Hash The \c hasher of the \c hash_map.
	 * \tparam KeyEqual The \c key_equal of the \c hash_map.
	 * \tparam Allocator The \c allocator of the \c hash_map.
	 *
	 * \param lhs One hash_map.
	 * \param rhs The other hash_map.
	 *
	 * \post
	 *     - The allocator, hasher, key comparator and all elements from \c lhs
	 *         are now in \c rhs and vice versa.
	 *     - No swap or assignment of the allocators, hashers, key comparators
	 *         any of the elements of \c lhs or \c rhs has occurred.
	 *     - All iterators to any elements of \c lhs or \c rhs are still valid
	 *         (but are now associated with the opposite hash_map).
	 */
	template<
		typename Key, typename T, typename Hash,
		typename KeyEqual, typename Allocator
	>
	void swap(
		::hash_map<Key, T, Hash, KeyEqual, Allocator> &lhs,
		::hash_map<Key, T, Hash, KeyEqual, Allocator> &rhs
	) {
		lhs.swap(rhs);
	}
} // namespace std

#endif // HASH_MAP_HPP_INCLUDED
