#include "TestLtHash_metall_common.hpp"

int main(int argc, char **argv) {
	assert(argc >= 2);
	auto const *path = argv[1];

	{ // reopen and read the segment
		metall::manager manager(metall::open_only, path);
		auto lthash_ptr = std::get<0>(manager.find<LtHash_t<allocator_type>>(lthash_name));

		LtHash_t<allocator_type> other_lthash1{manager.get_allocator()};
		other_lthash1.add(obj);

		LtHash_t<std::allocator<std::byte>> other_lthash2;
		other_lthash2.add(obj);

		print_span(lthash_ptr->checksum());
		print_span(other_lthash1.checksum());
		print_span(other_lthash2.checksum());

		assert((std::ranges::equal(lthash_ptr->checksum(), other_lthash1.checksum())));
		assert((std::ranges::equal(lthash_ptr->checksum(), other_lthash2.checksum())));
	}

	metall::manager::remove(path);
}
