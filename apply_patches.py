from os import path

Import("env")

# find library
LIBRARY_DIR = None
for lib_dir in env.get("LIBSOURCE_DIRS"):
    search_lib_path = path.join(env.subst(lib_dir), "U8g2_for_Adafruit_GFX")
    if path.isdir(search_lib_path):
        LIBRARY_DIR = search_lib_path
        break

if not LIBRARY_DIR:
    print("Can not find U8g2_for_Adafruit_GFX, exit")
    exit(1)

print("Find U8g2_for_Adafruit_GFX: " + LIBRARY_DIR)
patchflag_path = path.join(LIBRARY_DIR, ".patching-done")

# patch file only if we didn't do it before
if not path.isfile(patchflag_path):
    patch_file = path.join("patches", "1-u8g2-add-esp8266-fonts-support.patch")
    env.Execute("python tools/patch.py -d %s %s" % (LIBRARY_DIR, patch_file))

    def _touch(path):
        with open(path, "w") as fp:
            fp.write("")
    env.Execute(lambda *args, **kwargs: _touch(patchflag_path))

# Native SPI: 修复 64 位 Windows 下指针转 unsigned long 丢精度的问题
# (Native SPI 库的 SPI.cpp 用 (unsigned long)cbuf 取指针地址, 在 x64 上截断为 32 位)
for lib_dir in env.get("LIBSOURCE_DIRS"):
    native_spi_dir = path.join(env.subst(lib_dir), "Native SPI")
    if not path.isdir(native_spi_dir):
        continue
    print("Find Native SPI: " + native_spi_dir)
    spi_flag_path = path.join(native_spi_dir, ".patching-done")
    if not path.isfile(spi_flag_path):
        spi_cpp = path.join(native_spi_dir, "src", "SPI.cpp")
        with open(spi_cpp, "r", encoding="utf-8", errors="ignore") as fp:
            content = fp.read()
        new_content = content.replace("(unsigned long)cbuf", "(uintptr_t)cbuf")
        if new_content != content:
            with open(spi_cpp, "w", encoding="utf-8") as fp:
                fp.write(new_content)
            print("Patched Native SPI: (unsigned long)cbuf -> (uintptr_t)cbuf")
        with open(spi_flag_path, "w") as fp:
            fp.write("")
    break
