#include "TestLtHash_metall_common.hpp"

int main(int argc, char **argv) {
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " <segment-path>\n";
		return 1;
	}
	char const *path = argv[1];

	{ // create segment
		metall::manager manager(metall::create_only, path);
	}

	metall::manager manager(metall::open_only, path);

	auto lthash_ptr = manager.construct<LtHash_t>(lthash_name)();
	lthash_ptr->add(obj);
}
