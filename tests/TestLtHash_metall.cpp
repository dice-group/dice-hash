#include <catch2/catch_all.hpp>

#include <random>

#include <cassert>
#include <unistd.h>
#include <sys/wait.h>

#include "TestLtHash_metall_common.hpp"

TEST_CASE("LtHash allocator independent checksum equality") {
	std::string const path{"/tmp/" + std::to_string(std::random_device{}())};

	{
		metall::manager manager(metall::create_only, path.c_str());
	}

	{
		metall::manager manager(metall::open_only, path.c_str());

		LtHash_t<std::allocator<std::byte>> lt1;
		lt1.add(obj);

		LtHash_t<allocator_type> lt2{manager.get_allocator()};
		lt2.add(obj);

		print_span(lt1.checksum());
		print_span(lt2.checksum());

		CHECK(std::ranges::equal(lt1.checksum(), lt2.checksum()));
	}

	metall::manager::remove(path.c_str());
}

TEST_CASE("LtHash metall") {
	std::string const path{"/tmp/" + std::to_string(std::random_device{}())};

	auto pid = fork();
	assert(pid >= 0);

	if (pid == 0) {
		int st = execl("TestLtHash_metall_phase1", "TestLtHash_metall_phase1", path.data(), nullptr);
		assert(st == 0);
	} else {
		int rc;
		int st = waitpid(pid, &rc, 0);
		assert(st >= 0);
		assert(WIFEXITED(rc));
		assert(WEXITSTATUS(rc) == 0);
	}

	pid = fork();
	assert(pid >= 0);

	if (pid == 0) {
		int st = execl("TestLtHash_metall_phase2", "TestLtHash_metall_phase2", path.data(), nullptr);
		assert(st == 0);
	} else {
		int rc;
		int st = waitpid(pid, &rc, 0);
		assert(st >= 0);
		assert(WIFEXITED(rc));
		assert(WEXITSTATUS(rc) == 0);
	}
}
