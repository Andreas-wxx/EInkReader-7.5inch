Import("env")
import os
import shutil

# 编译完成后把 SDL3.dll / libcurl-x64.dll 复制到可执行文件目录,
# 保证模拟器可以直接运行 (预编译库全部位于项目 third_party/, 不依赖 conan)
def copy_dlls(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    os.makedirs(build_dir, exist_ok=True)
    project = env.subst("$PROJECT_DIR")
    dlls = [
        os.path.join(project, "third_party", "sdl3", "SDL3-3.4.0",
                     "x86_64-w64-mingw32", "bin", "SDL3.dll"),
        os.path.join(project, "third_party", "curl", "curl-8.19.0_8-win64-mingw",
                     "bin", "libcurl-x64.dll"),
    ]
    for dll in dlls:
        if os.path.exists(dll):
            shutil.copy2(dll, os.path.join(build_dir, os.path.basename(dll)))
            print("[emulator] copied %s -> %s" % (os.path.basename(dll), build_dir))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.exe", copy_dlls)
