import re, os
from conans.tools import load
from conans import ConanFile, CMake, tools


class DiceHashConan(ConanFile):
    name = "dice-hash"
    license = "AGPL"
    author = "DICE Group <info@dice-research.org>"
    homepage = "https://github.com/dice-group/dice-hash"
    url = homepage
    topics = ("hash", "C++", "C++20")
    settings = "build_type", "compiler", "os", "arch"
    generators = "cmake", "cmake_find_package", "cmake_paths"
    exports = "LICENSE"
    exports_sources = "src/*", "include/*", "CMakeLists.txt", "cmake/*"
    no_copy_source = True
    # No settings/options are necessary, this is header only

    def set_version(self):
        if not hasattr(self, 'version') or self.version is None:
            cmake_file = load(os.path.join(self.recipe_folder, "CMakeLists.txt"))
            self.version = re.search("project\(dice-hash VERSION (.*)\)", cmake_file).group(1)

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

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        try:
            with open("README.md") as file:
                self.description = file.read()
        except Exception as err:
            self.description = str(err)
