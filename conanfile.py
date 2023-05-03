import re, os

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import load, rmdir, copy


class DiceHashConan(ConanFile):
    license = "AGPL"
    author = "DICE Group <info@dice-research.org>"
    homepage = "https://github.com/dice-group/dice-hash"
    url = homepage
    topics = ("hash", "wyhash", "xxh3", "robin-hood-hash", "Blake2xb", "LtHash", "C++", "C++20")
    settings = "build_type", "compiler", "os", "arch"
    generators = ("cmake_find_package", "CMakeDeps", "CMakeToolchain")
    exports = "LICENSE"
    exports_sources = "include/*", "CMakeLists.txt", "cmake/*", "LICENSE"
    no_copy_source = True
    options = {"with_test_deps": [True, False]}
    default_options = {"with_test_deps": False}

    # No settings/options are necessary, this is header only

    def requirements(self):
        self.requires("libsodium/cci.20220430")

        if self.options.with_test_deps:
            self.requires("metall/0.21")

        self.requires("boost/1.81.0")  # override because older boost versions don't build with clang-16+

    def set_name(self):
        if not hasattr(self, 'name') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.name = re.search(r"project\(\s*([a-z\-]+)\s+VERSION", cmake_file).group(1)

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+.\d+)[^)]*\)", cmake_file).group(1)
        if not hasattr(self, 'description') or self.description is None:
            cmake_file = load(self, os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.description = re.search(r"project\([^)]*DESCRIPTION\s+\"([^\"]+)\"[^)]*\)", cmake_file).group(1)

    def layout(self):
        cmake_layout(self)

    def package_id(self):
        self.info.header_only()

    def package(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()
        for dir in ("lib", "res", "share"):
            rmdir(self, os.path.join(self.package_folder, dir))
        copy(self, pattern="LICENSE*", dst="licenses", src=self.folders.source_folder)
