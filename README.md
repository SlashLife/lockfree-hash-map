Concurrent hash_map
===================

(by Simon Stienen)

**This implementation was done in response to an assignment for a job interview.**
**Production use is discouraged!**

This software package offers a concurrent hash map close in style to
`std::unordered_map`.



Building
========

The hash map itself is header only and only uses features from standard C++14,
so no building is required before using it. However, it comes with a build
system to automatically generate the documentation and run the tests.

- To build both the documentation and run the tests, run `make all`

- To remove both the documentation and the tests, run `make clean`

Tests
-----

- To run all tests, run `make test`

- To run a specific test, for example `api.lookup`, run
  `make test/api.lookup.run`

- To delete all tests, run `make clean-test`

Documentation
-------------

The documentation comes in two flavors: The general documentation for library
users and the internal documentation with additional information on the
internal structures for library implementors.

By default the general documentation is built.

- To build the general documentation, run `make doc`

- To build the internal documentation, run `make doc-internal`

- To delete the documentation, run `make clean-doc`



Concurrency model
=================

This hash map is written in a way to make full use of the thread safety
facilities offered by C++11.

The foundation for most thread safe operations are atomic operations on
`std::shared_ptr`. Using those, an operation will in a safe manner either get
access to a resource or not -- and if it _did_ get access, it can guarantee
that the resource will be available for the entirety of its operation.

Buckets
-------

Buckets contain a circular list of nodes. Each bucket contains a distinct
sentinel node, which will not change during the lifetime of the bucket, that
represents both the beginning and the end of the buckets node list. (This
dependency circle is broken by the buckets destructor.)

Insertions into buckets only take place at the end of the list.

Finding internal nodes
----------------------

At the heart of all thread safe functions dealing with elements lies the
function `hash_map::fixed_size_bucket_list::bucket::find()`, which retrieves a
pair of shared node pointers: One to the node the caller was looking for and
one to its predecessor, which contains the next-pointer to the node found.

If no node is found, the sentinel node (and its predecessor) are returned
instead, offering the correct position to insert a new node, as all insertions
happen at the end of a bucket.

The traversal of nodes starts at the sentinel node of a bucket and stops when
it reaches the sentinel node again, or when the requested node is found,
whichever happens first. If a concurrent operation marks a next-pointer invalid
(`nullptr`), traversal will reset and start from the beginning, until it
succeeds in finding the requested node or reaching the sentinel.

Not every operation requires all the information returned by this function, but
all information is required by one function or another, and comes for free with
the node search, so concentrating on the correctness of this one function makes
the code much simpler.

Thread safe base operations
---------------------------

These fundamental thread safe operations deal with elements directly. All other
thread safe functions either not meddle with the contents of the hash map on an
element level, or do so by applying these functions.

### Correctness and atomicity ###

All of these operations atomically create their own counted reference to the
bucket list they work on, so they will finish on the same bucket list they
started on, and keep that list alive while doing so.

There are four operations which can interfer with this: `clear()`, `rehash()`,
`swap()` and `operator=`, the latter of which uses a `swap()`.

`clear()` will atomically exchange the current bucket list with an empty one;
concurrent thread safe element operations will either finish successfully on
the previous bucket list (having their effects ordered _before_ `clear()`) or
on the new one (having their effects ordered _after_ `clear()`).

`rehash()` is not a thread safe function, so running concurrently with it is
not a concern.

`swap()` (and thus `operator=`) is not a thread safe function. While exhibiting
almost the same behavior towards thread safe element functions as `clear()`,
with regard to working on either the old or the new bucket list, a badly timed
`swap()` can negate a `clear()` and thus clearly is not thread safe and not
marked thread safe, so running concurrently with it is not a concern.

#### `find()` ####

Finding nodes is a straightforward application of finding internal nodes. The
result will always yield an internal node, which can be either a data node that
can be returned or a sentinel node, in which case a failure (end iterator) is
returned.

#### `equal_range()` ####

This also is a pretty straightforward application of finding internal nodes. If
the internal node found is a data node, a pair of local iterators to the node
found and its next node is returned. The next node may be a sentinel node, but
local iterators (as opposed to non-local iterators) can refer to sentinel nodes
(and do so to represent the end of the bucket iterator), so that behavior is
perfectly fine. If the node found is itself the sentinel, then no element with
the specified key exists and an empty range `[ end()..end() )` represents the
result accordingly.

#### `erase()` ####

An erase operation will first find the internal node representing the element
to be erased. If this node cannot be found, the element does not exist and the
erase operation is a noop.

Otherwise the internal node will be marked for deletion by atomically replacing
its next-pointer with a `nullptr` (keeping a copy).

If it turns out that the next-pointer _already_ is a `nullptr`, that means a
concurrent erase operation is running concurrently and may or may not succeed.
This erase operation will restart from the beginning (i.e. trying to find the
node again). At this point no change to the data structure has been made (only
a `nullptr` was replaced with a `nullptr` atomically), so no roll-back is
necessary.

At this stage, the `nullptr` next-pointer means that traversions through the
to-be-erased node will either have gotten through to the next node before the
atomic exchange or will run into our `nullptr` and needs to restart its
traversal.

However, if the next-pointer in the to-be-erased node was changed after the
next-pointer in the predecessor, a concurrent insert after the to-be-deleted
node may be lost or skipped over by traversions concurrent to both operations,
resulting either in possible data loss or possible element duplication.

Following up, the integrity of the node list is restored by replacing the
predecessors next-pointer with the backup of the old next-pointer using an
atomic compare and switch, comparing the current value against the
to-be-deleted node. If the compare and switch succeeded, the to-be-deleted node
has successfully been unlinked from the node list and will be deleted when the
last `shared_ptr` to it held by any concurrent operation goes out of scope.

There are three operations that could have changed that pointer, two of which
can be rules out:
- Concurrent `insert()`; but inserts only happen at the end of a node list, and
  while this `erase()` operation may very well cause the predecessor to
  _become_ the end of the list, it is not yet.
- Concurrent `erase()` to the same node; which will not continue once it sees
  `current->next == nullptr`.

The only operation left to cause this is an `erase()` on the predecessor, which
leaves the next-pointer as a `nullptr`. To find the new predecessor, `erase()`
needs to restart, but before it does that, it needs to (atomically) restore the
to-be-deleted nodes next-pointer to its old value to restore the previous state
of the list.

#### `insert()` ####

An insert operation will first find an internal node representing the requested
element. If one is found, the element is already in the list and `insert()` is
a noop and returns.

Otherwise, the internal find retrieved the sentinel (end of bucket) as the node
found and its predecessor (which may also be the sentinel) which contains the
next-pointer to the sentinel.

At this point a new node containing the element is instantiated and configured
to refer to the end of the list with its next-pointer. As its value is constant
for the duration of the function (and keys are constant anyway), its associated
bucket and with it the end-of-node-list node will not change, and due to the
fact that insertions always happen at the end of the bucket, neither will the
new nodes next-pointer. This part of the operation is race free.

Next an atomic compare and swap operation replaces the predecessors
next-pointer with a pointer to the new node, if that next-pointer still refers
to the end-of-list. If this exchange succeeds, the new node has successfully
been added to the bucket list, and an iterator to it can be returned.

There are two possible options for the exchange to fail: A concurrent erase
operation on the predecessor or a successful concurrent insert operation to the
same bucket. In either case the process essentially starts from the beginning,
attempting to find the element or the end of the bucket again.

However, on restarting the process, the instantiated new node can be retained
and does not need to be created again on a subsequent attempt.

#### `insert_or_assign()` ####

This function works essentially the same way as `insert()` and just differs in
little details how existing nodes are handled and how the result is returned.
This could, in theory, be implemented in terms of `insert()`, but `insert()`
returns an iterator (and a bool), access to which is not thread safe.

### Progress ###

All of the aforementioned operations are guaranteed to succeed eventually:

(Note: In this context the term "block" refers to the failure to successfully
       perform an operation in a particular iteration and the need to restart
	   that operation.)

#### General traversal, `find()` and `equal_range()` ####

These will not block the progress of any other operation, and will only be
blocked in the presence of a `nullptr` next-pointer by an unfinished erase
operation. In the worst case they will __succeed__ after all erase operations
have succeeded.

#### `erase()` ####

These can block the traversal to later nodes in the bucket.

They can only be blocked by __successful__ insert operations, if the node to
be erased was the last node in the bucket beforehand. Unsuccessful inserts will
not block an erase operation.

They can not be blocked by erase operations (successful or not) to later
elements.

They can be blocked by concurrent erase operations to the predecessor.

They can be blocked by concurrent erase operations to the same node, however
this block is not mutual: One of the operations will __succeed__, unless
blocked by an operation other than an erase of the same node.

Put together, if _all_ concurrent erase operations on the same node are
blocked, the reason is either a __successful__ insert operation (which is
progress), or the presence of erase operations on the predecessor, which can
only be _all_ blocked by the presence of erase operations to _their_
predecessor, and so on, until the beginning of the bucket, where there are no
more predecessors and one of the erase operation will either be __successful__
or blocked by a __successful__ insert operation.

#### `insert()` and `insert_or_assign()` ####

Note: `insert_or_assign()` can only block other operations if it results in an
      actual insert operation.

_If and only if_ __successful__, these can block concurrent erase operations on
the last node in the bucket and other insert operations on the same bucket.

In turn they can _only_ be blocked (save traversal to the end by unfinished
erase operations) by __successful__ concurrent erase operations on the last
node of the bucket or another __successful__ insert operation.

Limitations
===========

To keep the code somewhat simple, some features did not make the cut:
- __Owning iterators__, that would keep the objects they are pointing to alive.
  While this by itself would be simple enough to implement (let iterators hold
  a `shared_ptr<node>` instead of a `node*`), it may not be the desired thing
  to have in all situations, so instead __iterator policies__ would have been
  introduced.
- A bunch of __overloads for modifiers__, including range based operations for
  example, which could be trivially implemented in a thread safe (though not
  necessarily atomic) manner.
- __Move semantics__
- ...
