import re
import os

from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.files import load, rmdir, copy


class DiceHashConan(ConanFile):
    license = "AGPL"
    author = "DICE Group <info@dice-research.org>"
    homepage = "https://github.com/dice-group/dice-hash"
    url = homepage
    topics = ("hash", "wyhash", "xxh3", "robin-hood-hash", "Blake2b", "Blake2Xb", "LtHash", "C++", "C++20")

    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_test_deps": [True, False],
        "with_sodium": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": False,
        "with_test_deps": False,
        "with_sodium": False
    }
    exports = "LICENSE"
    exports_sources = "include/*", "CMakeLists.txt", "cmake/*", "LICENSE"

    generators = ("CMakeDeps", "CMakeToolchain")

    def requirements(self):
        if self.options.with_sodium:
            self.requires("libsodium/cci.20220430", transitive_headers=True)
            self.requires("highway/1.2.0")

        if self.options.with_test_deps:
            self.test_requires("catch2/3.3.2")
            self.test_requires("metall/0.23.1")

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

    _cmake = None

    def _configure_cmake(self):
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.configure(variables={"WITH_SODIUM": self.options.with_sodium})

        return self._cmake

    def build(self):
        self._configure_cmake().build()

    def package(self):
        self._configure_cmake().install()

        for dir in ("lib", "res", "share"):
            rmdir(self, os.path.join(self.package_folder, dir))

        rmdir(self, os.path.join(self.package_folder, "cmake"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        copy(self, pattern="LICENSE*", dst=os.path.join(self.package_folder, "licenses"), src=self.folders.source_folder)
        copy(self, pattern="*.a", src=os.path.join(self.build_folder, "include/dice/hash/blake/internal/blake3"), dst=os.path.join(self.package_folder, "lib"), keep_path=False)
        copy(self, pattern="*.a", src=self.build_folder, dst=os.path.join(self.package_folder, "lib"), keep_path=False)

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_target_name", "dice-hash::dice-hash")
        self.cpp_info.set_property("cmake_file_name", "dice-hash")

        if self.options.with_sodium:
            self.cpp_info.libs += ["dice-hash", "blake3"]
            self.cpp_info.requires += ["libsodium::libsodium", "highway::highway"]
