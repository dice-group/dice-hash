import re, os
from conans.tools import load
from conans import ConanFile, CMake, tools
from conans.util.files import rmdir


class DiceHashConan(ConanFile):
    license = "AGPL"
    author = "DICE Group <info@dice-research.org>"
    homepage = "https://github.com/dice-group/dice-hash"
    url = homepage
    topics = ("hash", "wyhash", "xxh3", "robin-hood-hash", "C++", "C++20")
    settings = "build_type", "compiler", "os", "arch"
    generators = "cmake", "cmake_find_package", "cmake_paths"
    exports = "LICENSE"
    exports_sources = "include/*", "CMakeLists.txt", "cmake/*", "LICENSE"
    no_copy_source = True

    # No settings/options are necessary, this is header only

    def set_name(self):
        if not hasattr(self, 'name') or self.version is None:
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.name = re.search(r"project\(\s*([a-z\-]+)\s+VERSION", cmake_file).group(1)

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+.\d+)[^)]*\)", cmake_file).group(1)
        if not hasattr(self, 'description') or self.description is None:
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.description = re.search(r"project\([^)]*DESCRIPTION\s+\"([^\"]+)\"[^)]*\)", cmake_file).group(1)

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+.\d+)[^)]*\)", cmake_file).group(1)

    def package_id(self):
        self.info.header_only()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()
        for dir in ("lib", "res", "share"):
            rmdir(os.path.join(self.package_folder, dir))
        self.copy(pattern="LICENSE*", dst="licenses", src=self.folders.source_folder)

