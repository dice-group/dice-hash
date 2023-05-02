#include "TestLtHash_metall_common.hpp"

int main(int argc, char **argv) {
	assert(argc >= 2);
	char const *path = argv[1];

	{ // create segment
		metall::manager manager(metall::create_only, path);
	}

	metall::manager manager(metall::open_only, path);

	auto lthash_ptr = manager.construct<LtHash_t<allocator_type>>(lthash_name)(manager.get_allocator());
	lthash_ptr->add(obj);
}