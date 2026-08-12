# -*- coding: utf-8 -*-
"""对比新旧 BDF 渲染 '周一' 的效果"""
import re
import numpy as np
from PIL import Image

def load_bdf_chars(path):
    data = open(path, encoding='ascii').read()
    blocks = re.findall(
        r'STARTCHAR ([^\n]*)\nENCODING (\d+)\n(?:SWIDTH[^\n]*\n)?(?:DWIDTH ([^\n]*)\n)?BBX (\d+) (\d+) (-?\d+) (-?\d+)\nBITMAP\n(.*?)\nENDCHAR',
        data, re.S)
    out = {}
    for name, enc, dw, w, h, x, y, bm in blocks:
        rows = []
        for line in bm.strip().split('\n'):
            row = []
            for ch in line:
                v = int(ch, 16)
                row += [(v >> (7 - b)) & 1 for b in range(8)]
            rows.append(np.array(row[:int(w)], dtype=np.uint8))
        out[int(enc)] = (int(w), int(h), int(x), int(y), np.array(rows))
    return out

old = load_bdf_chars(r'doc/simhei24.bdf')
new = load_bdf_chars(r'doc/simhei24_v2.bdf')
print('old chars:', len(old), 'new chars:', len(new))

def draw_word(chars, text, canvas_w=160, canvas_h=40):
    img = np.full((canvas_h, canvas_w), 255, dtype=np.uint8)
    cx = 0
    for ch in text:
        w, h, x, y, rows = chars[ord(ch)]
        top = 21 - (y + h - 1)   # BDF y offset -> 画布行
        for i in range(h):
            for j in range(w):
                if rows[i][j]:
                    yy, xx = top + i, cx + x + j
                    if 0 <= yy < canvas_h and 0 <= xx < canvas_w:
                        img[yy][xx] = 0
        cx += 24
    return Image.fromarray(img)

for name, d in [('old', old), ('new', new)]:
    draw_word(d, '周一').save(r'doc/preview_%s_zhouyi.png' % name)

old_img = np.array(draw_word(old, '周一'))
new_img = np.array(draw_word(new, '周一'))
gap = np.full((40, 12), 200, dtype=np.uint8)
combo = np.hstack([old_img, gap, new_img])
Image.fromarray(combo).resize((combo.shape[1]*3, combo.shape[0]*3), Image.NEAREST).save(r'doc/preview_compare.png')
print('compare saved', combo.shape)