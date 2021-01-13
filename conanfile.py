import re, os
from conans.tools import load
from conans import ConanFile, CMake, tools


class DiceHashConan(ConanFile):
    name = "dice-hash"
    license = "AGPL"
    author = "DICE Group <info@dice-research.org>"
    homepage = "https://github.com/dice-group/dice-hash"
    url = homepage
    description = "<Description of DiceHash here>"
    topics = ("hash", "C++", "C++20")
    settings = "build_type", "compiler", "os", "arch"
    generators = "cmake", "cmake_find_package", "cmake_paths"
    exports = "LICENSE"
    exports_sources = "include/*", "CMakeLists.txt", "cmake/*"
    no_copy_source = True
    # No settings/options are necessary, this is header only

    def set_version(self):
        if not hasattr(self, 'version'):
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search("project\(dice_hash VERSION (.*)\)", cmake_file).group(1)

    def package_id(self):
        self.info.header_only()

    def package(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()
