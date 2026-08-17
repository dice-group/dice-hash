#include <catch2/catch_all.hpp>

#include <random>

#include <cassert>
#include <unistd.h>
#include <sys/wait.h>

#include "TestLtHash_metall_common.hpp"

TEST_CASE("LtHash metall") {
	std::string const path{"/tmp/" + std::to_string(std::random_device{}())};

	auto pid = fork();
	REQUIRE(pid >= 0);

	if (pid == 0) {
		execl("TestLtHash_metall_phase1", "TestLtHash_metall_phase1", path.data(), nullptr);
		// execl returns only on failure
		_exit(1);
	} else {
		int rc;
		REQUIRE(waitpid(pid, &rc, 0) >= 0);
		REQUIRE(WIFEXITED(rc));
		REQUIRE(WEXITSTATUS(rc) == 0);
	}

	pid = fork();
	REQUIRE(pid >= 0);

	if (pid == 0) {
		execl("TestLtHash_metall_phase2", "TestLtHash_metall_phase2", path.data(), nullptr);
		// execl returns only on failure
		_exit(1);
	} else {
		int rc;
		REQUIRE(waitpid(pid, &rc, 0) >= 0);
		REQUIRE(WIFEXITED(rc));
		REQUIRE(WEXITSTATUS(rc) == 0);
	}
}
