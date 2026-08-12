#!/usr/bin/env python3
"""图片转墨水屏 1bpp 位图 C 数组工具

用法:
    python tools/img2bitmap.py <图片路径> [宽度] [高度] [--dither]

示例:
    python tools/img2bitmap.py icon.png 48 48          # 简单阈值(适合图标)
    python tools/img2bitmap.py photo.jpg 200 300 --dither  # Floyd-Steinberg抖动(适合照片)

输出: 与 src/bitmap.h 相同格式的 C 数组 (1bpp, MSB first, 黑=1)
"""
import sys
from PIL import Image


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    src = sys.argv[1]
    w = int(sys.argv[2]) if len(sys.argv) > 2 else 48
    h = int(sys.argv[3]) if len(sys.argv) > 3 else 48
    dither = "--dither" in sys.argv

    img = Image.open(src).convert("L")
    img = img.resize((w, h), Image.LANCZOS)
    px = img.load()

    # 二值化: Floyd-Steinberg 抖动或简单阈值
    if dither:
        arr = [[px[x, y] for x in range(w)] for y in range(h)]
        for y in range(h):
            for x in range(w):
                old = arr[y][x]
                new = 0 if old < 128 else 255
                arr[y][x] = new
                err = old - new
                if x + 1 < w: arr[y][x + 1] += err * 7 // 16
                if y + 1 < h:
                    if x > 0: arr[y + 1][x - 1] += err * 3 // 16
                    arr[y + 1][x] += err * 5 // 16
                    if x + 1 < w: arr[y + 1][x + 1] += err * 1 // 16
    else:
        arr = [[0 if px[x, y] < 128 else 255 for x in range(w)] for y in range(h)]

    name = src.split("\\")[-1].split("/")[-1].split(".")[0].upper()
    print(f"// {src} -> {w}x{h} 1bpp ({w * h // 8} 字节), 黑=1, MSB first")
    print(f"const uint8_t IMG_{name}[{w * h // 8}] PROGMEM = {{")
    for y in range(h):
        row = []
        for bx in range(w // 8):
            byte = 0
            for bit in range(8):
                if arr[y][bx * 8 + bit] < 128:
                    byte |= 1 << (7 - bit)
            row.append(f"0x{byte:02x}")
        suffix = "," if y < h - 1 else ""
        print("    " + ", ".join(row) + suffix)
    print("};")
    return 0


if __name__ == "__main__":
    sys.exit(main())
