from conans import ConanFile, CMake, tools


class MTAPConan(ConanFile):
    name = "mtap"
    version = "0.1"
    license = "MPL-2.0"
    author = "jgcodes2020"
    url = "https://github.com/jgcodes2020/mtap"
    description = "Minimal, but heavily templated argument parser"
    topics = ("header-only", "cli", "argument-parser")
    
    exports_sources = "include/*"
    no_copy_source = True
    
    def validate(self):
        tools.check_min_cppstd(self, "20")
    
    def package(self):
        self.copy("*.hpp", dst="include")
    
    def package_id(self):
        self.info.header_only()
    
    def package_info(self):
        self.cpp_info.includedirs = ["include"]
