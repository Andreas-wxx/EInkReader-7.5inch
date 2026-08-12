# -*- coding: utf-8 -*-
"""
重新生成 SimHei 24px BDF 字体。
修复：原 BDF 把所有字形 BBX y 偏移写成 1（贴底），导致"一"这类矮字沉到底部。
本脚本用 PIL 渲染 simhei.ttf + numpy 先阈值化再取 ink 边界，
得到字形在 24px em 框中的原生位置（汉字自动居中）。
用法: python tools/gen_simhei_font.py
"""
import re
from PIL import Image, ImageDraw, ImageFont
import numpy as np

SRC  = r'doc/simhei24.bdf'        # 原 BDF（提供字符集 + DWIDTH）
OUT  = r'doc/simhei24_v2.bdf'     # 新 BDF
FONT_PATH = r'C:\Windows\Fonts\simhei.ttf'
SIZE = 24
THRESHOLD = 128

font = ImageFont.truetype(FONT_PATH, SIZE)

# ---------- 1. 解析原 BDF：提取每个码点的 SWIDTH/DWIDTH ----------
data = open(SRC, encoding='utf-8').read()
head_end = data.index('CHARS')
head = data[:head_end]
blocks = re.findall(
    r'STARTCHAR ([^\n]*)\nENCODING (\d+)\n(?:SWIDTH ([^\n]*)\n)?(?:DWIDTH ([^\n]*)\n)?(?:BBX ([^\n]*)\n)?BITMAP\n(.*?)\nENDCHAR',
    data, re.S)
chars = {}
for name, enc, sw, dw, bbx, bm in blocks:
    chars[int(enc)] = (sw, dw)
print('parsed chars:', len(chars))

# ---------- 2. 渲染 -> 阈值化 -> 取真实 ink 边界 ----------
def render_char(code):
    ch = chr(code)
    img = Image.new('L', (32, 32), 255)
    ImageDraw.Draw(img).text((0, 0), ch, font=font, fill=0)
    a = np.array(img)
    bw = a < THRESHOLD                      # 先 1-bit 化
    nz = np.where(bw)
    if len(nz[0]) == 0:
        return None
    x0, y0 = int(nz[1].min()), int(nz[0].min())
    x1, y1 = int(nz[1].max()) + 1, int(nz[0].max()) + 1
    w, h = x1 - x0, y1 - y0
    crop = bw[y0:y1, x0:x1]
    xoff = x0
    yoff = 22 - y0 - h                     # BDF y offset（基线 y=0，框顶 y=21）
    return xoff, yoff, w, h, crop

def pack_row(row_mask, width):
    nbytes = (width + 7) // 8
    out = []
    for b in range(nbytes):
        byte = 0
        for bit in range(8):
            i = b * 8 + bit
            if i < width and row_mask[i]:
                byte |= (0x80 >> bit)
        out.append('%02X' % byte)
    return ''.join(out)

entries = []
missing = []
for code in sorted(chars):
    sw, dw = chars[code]
    r = render_char(code)
    if r is None:
        missing.append(code)
        continue
    xoff, yoff, w, h, crop = r
    lines = [pack_row(crop[row], w) for row in range(h)]
    entries.append((code, sw, dw, xoff, yoff, w, h, lines))

print('rendered:', len(entries), 'missing:', len(missing), missing[:20])

# ---------- 3. 写新 BDF ----------
with open(OUT, 'w', encoding='ascii', newline='\n') as f:
    f.write(head)
    f.write('CHARS %d\n' % len(entries))
    for code, sw, dw, xoff, yoff, w, h, lines in entries:
        f.write('STARTCHAR U+%04X\n' % code)
        f.write('ENCODING %d\n' % code)
        f.write('SWIDTH %s\n' % (sw if sw else '500 0'))
        f.write('DWIDTH %s\n' % dw)
        f.write('BBX %d %d %d %d\n' % (w, h, xoff, yoff))
        f.write('BITMAP\n')
        for ln in lines:
            f.write(ln + '\n')
        f.write('ENDCHAR\n')
    f.write('ENDFONT\n')

# ---------- 4. 验证 ----------
def get_bbx(path, code):
    d = open(path, encoding='ascii').read()
    m = re.search(r'ENCODING %d\n(?:SWIDTH[^\n]*\n)?(?:DWIDTH[^\n]*\n)?(BBX [^\n]*)\n' % code, d)
    return m.group(1) if m else 'NOT FOUND'

print('\n--- 新旧 BBX 对比 ---')
for ch in ['一', '二', '周', '天', 'A', '。', '℃']:
    print(repr(ch), 'old:', get_bbx(SRC, ord(ch)), ' new:', get_bbx(OUT, ord(ch)))
print('\nOK ->', OUT)