from conan import ConanFile
from conan.tools.scons import SConsDeps

class NativeRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "SConsDeps"
    default_options = {
        "sdl/*:shared": True,
        "libcurl/*:shared": True,
        # Windows 下用系统原生 schannel 代替 openssl,
        # 避免 openssl 3.6.3 在 gcc16/mingw 下的源码编译兼容问题
        "libcurl/*:with_ssl": "schannel",
    }

    def requirements(self):
        self.requires("sdl/3.4.0")
        self.requires("libcurl/8.19.0")
