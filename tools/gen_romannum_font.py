# -*- coding: utf-8 -*-
"""生成 200px 罗马数字字体"""
import re
from PIL import Image, ImageDraw, ImageFont
import numpy as np

SIZE = 200
OUT = r'doc/romannum200.bdf'
font = ImageFont.truetype(r'C:\Windows\Fonts\arialbd.ttf', SIZE)
ascent, descent = font.getmetrics()
codes = list(range(0x30, 0x3A)) + [0x3A]

def render_char(code):
    ch = chr(code)
    img = Image.new('L', (SIZE + 80, SIZE + 100), 255)
    ImageDraw.Draw(img).text((0, 0), ch, font=font, fill=0)
    a = np.array(img); bw = a < 128
    nz = np.where(bw)
    if len(nz[0]) == 0: return None
    x0, y0 = int(nz[1].min()), int(nz[0].min())
    x1, y1 = int(nz[1].max()) + 1, int(nz[0].max()) + 1
    w, h = x1-x0, y1-y0
    return x0, ascent - y0 - h + 1, w, h, bw[y0:y1, x0:x1]

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
for code in codes:
    r = render_char(code)
    if r is None: continue
    xoff, yoff, w, h, crop = r
    lines = [pack_row(crop[row], w) for row in range(h)]
    entries.append((code, int(font.getlength(chr(code))), xoff, yoff, w, h, lines))
    print('U+%04X' % code, 'BBX %d %d %d %d' % (w, h, xoff, yoff))

max_w = max(e[4] for e in entries)
head = ('STARTFONT 2.1\nFONT -FreeType-Arial-Bold-R-Normal--%d-240-72-72-P-%d-ISO10646-1\nSIZE %d 72 72\n'
        'FONTBOUNDINGBOX %d %d 0 -%d\nSTARTPROPERTIES 3\nFONT_ASCENT %d\nFONT_DESCENT %d\nENDPROPERTIES\nCHARS %d\n'
        % (SIZE, int(max_w/2), SIZE, max_w, ascent+descent, descent, ascent, descent, len(entries)))
with open(OUT, 'w', encoding='ascii', newline='\n') as f:
    f.write(head)
    for code, dw, xoff, yoff, w, h, lines in entries:
        f.write('STARTCHAR U+%04X\nENCODING %d\nSWIDTH 500 0\nDWIDTH %d 0\nBBX %d %d %d %d\nBITMAP\n'
                % (code, code, dw, w, h, xoff, yoff))
        for ln in lines: f.write(ln + '\n')
        f.write('ENDCHAR\n')
    f.write('ENDFONT\n')
print('OK ->', OUT)