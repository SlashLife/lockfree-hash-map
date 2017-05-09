#include <algorithm>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>

namespace {
	unsigned comparable_id=0;
}

template<typename T>
struct comparable: public T {
	comparable()
	: T()
	, id(++comparable_id) {}

	bool operator==(const comparable &other) const {
		return id == other.id;
	}

	bool operator!=(const comparable &other) const {
		return id != other.id;
	}

private:
	const unsigned id;
};

typedef hash_map<
	int, int,
	comparable<std::hash<int>>,
	comparable<std::equal_to<int>>,
	comparable<std::allocator<void*>>
> comparable_map;



template<typename T>
std::string dump_map(const T &hm) {
	std::string retval;

	retval+="[";
	for(auto e=hm.bucket_count(), b=e-e; b<e; ++b) {
		if (0 < b) {
			retval += " |";
		}
		if (0 < hm.bucket_size(b)) {
			std::stringstream ss;

			std::for_each(hm.cbegin(b), hm.cend(b), [&](auto element) {
				ss << ", [" << element.first << ',' << element.second << ']';
			});

			retval += ss.str().substr(1);
		}
	}

	retval+=" ]";

	return retval;
}
#undef INFO_MAP
#define INFO_MAP(map) INFO( #map " = " << dump_map(map) )



struct tracked_mapped_type {
	tracked_mapped_type() {
		++created;
	}

	tracked_mapped_type(const tracked_mapped_type &) {
		++created;
	}

	tracked_mapped_type &operator=(const tracked_mapped_type &) {
		return *this;
	}

	~tracked_mapped_type() {
		++destroyed;
	}

	static unsigned created;
	static unsigned destroyed;
};
unsigned tracked_mapped_type::created=0;
unsigned tracked_mapped_type::destroyed=0;
