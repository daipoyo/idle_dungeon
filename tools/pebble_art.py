# -*- coding: utf-8 -*-
"""Pebble向けドット絵生成の共通ヘルパー。

Pebbleのカラー機は 64色 (RGB各2bit: 0x00/0x55/0xAA/0xFF) なので、
ここで扱う色はすべてその範囲に収める。
"""
from PIL import Image

# ------------------------------------------------------------
# Pebble 64色パレット（よく使う色に1文字の記号を割り当てる）
# ------------------------------------------------------------
PAL = {
    '.': None,             # 透明
    'K': (0x00, 0x00, 0x00),  # Black
    'k': (0x55, 0x55, 0x55),  # DarkGray
    'g': (0xAA, 0xAA, 0xAA),  # LightGray
    'W': (0xFF, 0xFF, 0xFF),  # White
    'y': (0xFF, 0xAA, 0x00),  # ChromeYellow
    'Y': (0xFF, 0xFF, 0x55),  # Icterine
    'l': (0xFF, 0xFF, 0xAA),  # PastelYellow
    'o': (0xAA, 0x55, 0x00),  # WindsorTan
    'b': (0x55, 0x00, 0x00),  # BulgarianRose
    'O': (0xFF, 0x55, 0x00),  # Orange
    'r': (0xAA, 0x00, 0x00),  # DarkCandyAppleRed
    'R': (0xFF, 0x00, 0x00),  # Red
    'e': (0xFF, 0xAA, 0xAA),  # Melon
    's': (0xFF, 0xAA, 0x55),  # Rajah
    'v': (0xAA, 0x55, 0x55),  # RoseVale
    'B': (0x00, 0x55, 0xAA),  # CobaltBlue
    'u': (0x55, 0xAA, 0xFF),  # PictonBlue
    'U': (0x00, 0xAA, 0xFF),  # VividCerulean
    'N': (0x00, 0x00, 0x55),  # OxfordBlue
    'n': (0x00, 0x00, 0xAA),  # DukeBlue
    'i': (0x55, 0x55, 0xAA),  # Liberty
    'I': (0xAA, 0xAA, 0xFF),  # BabyBlueEyes
    'C': (0xAA, 0xFF, 0xFF),  # Celeste
    'c': (0x55, 0xFF, 0xFF),  # ElectricBlue
    'G': (0x00, 0xAA, 0x00),  # IslamicGreen
    'd': (0x00, 0x55, 0x00),  # DarkGreen
    'L': (0x55, 0xFF, 0x55),  # ScreaminGreen
    'j': (0xAA, 0xFF, 0x55),  # Inchworm
    'a': (0x55, 0x55, 0x00),  # ArmyGreen
    'A': (0xAA, 0xAA, 0x55),  # Brass
    'E': (0x55, 0xAA, 0x00),  # KellyGreen
    'p': (0xAA, 0x00, 0xAA),  # Purple
    'P': (0x55, 0x00, 0x55),  # ImperialPurple
    'm': (0xFF, 0x55, 0xFF),  # ShockingPink
    'M': (0xFF, 0x00, 0xAA),  # FashionMagenta
    'f': (0xFF, 0x00, 0x55),  # Folly
    'J': (0xAA, 0x00, 0x55),  # JazzberryJam
    'h': (0xFF, 0x55, 0xAA),  # BrilliantRose
    'H': (0xFF, 0xAA, 0xFF),  # RichBrilliantLavender
    'x': (0xAA, 0x55, 0xFF),  # LavenderIndigo
    'X': (0x55, 0x00, 0xAA),  # Indigo
    'q': (0xAA, 0x55, 0xAA),  # Purpureus
    't': (0x00, 0x55, 0x55),  # MidnightGreen
    'T': (0x00, 0xAA, 0xAA),  # TiffanyBlue
    'Q': (0x55, 0xAA, 0xAA),  # CadetBlue
    'w': (0x55, 0x55, 0xFF),  # VeryLightBlue
    'S': (0xFF, 0x55, 0x55),  # SunsetOrange
}


def rgb(c):
    return PAL[c] if isinstance(c, str) else c


def check_pebble_color(c):
    for v in c[:3]:
        assert v in (0x00, 0x55, 0xAA, 0xFF), c


def new_image(w, h, fill=None):
    if fill is None:
        return Image.new('RGBA', (w, h), (0, 0, 0, 0))
    r, g, b = rgb(fill)
    return Image.new('RGBA', (w, h), (r, g, b, 255))


def put(img, x, y, c):
    if c is None or c == '.':
        return
    if 0 <= x < img.width and 0 <= y < img.height:
        r, g, b = rgb(c)
        img.putpixel((x, y), (r, g, b, 255))


def get(img, x, y):
    if 0 <= x < img.width and 0 <= y < img.height:
        return img.getpixel((x, y))
    return (0, 0, 0, 0)


def rect(img, x, y, w, h, c):
    for yy in range(y, y + h):
        for xx in range(x, x + w):
            put(img, xx, yy, c)


def parse_map(rows, remap=None):
    """文字マップ(行のリスト)を2次元リストに変換。remapで記号の置換が可能。"""
    remap = remap or {}
    out = []
    for r in rows:
        out.append([remap.get(ch, ch) for ch in r])
    return out


def mirror_half(rows, swap=None):
    """左半分だけ描いたマップを左右対称に展開する。swapは右側で置き換える記号。"""
    swap = swap or {}
    out = []
    for r in rows:
        right = ''.join(swap.get(ch, ch) for ch in reversed(r))
        out.append(r + right)
    return out


def blit_map(img, ox, oy, rows, remap=None):
    for y, row in enumerate(parse_map(rows, remap)):
        for x, ch in enumerate(row):
            if ch != '.':
                put(img, ox + x, oy + y, ch)


def outline(img, color='K', diagonal=False):
    """不透明ピクセルの外側1pxを輪郭色で縁取る。"""
    w, h = img.size
    src = img.copy()
    for y in range(h):
        for x in range(w):
            if src.getpixel((x, y))[3] != 0:
                continue
            nb = [(1, 0), (-1, 0), (0, 1), (0, -1)]
            if diagonal:
                nb += [(1, 1), (-1, -1), (1, -1), (-1, 1)]
            for dx, dy in nb:
                if get(src, x + dx, y + dy)[3] != 0:
                    put(img, x, y, color)
                    break
    return img


def sprite_from_map(rows, remap=None, auto_outline=True, outline_color='K'):
    w = max(len(r) for r in rows)
    h = len(rows)
    img = new_image(w, h)
    blit_map(img, 0, 0, rows, remap)
    if auto_outline:
        outline(img, outline_color)
    return img


def validate(img):
    colors = set()
    for px in img.getdata():
        if px[3] == 0:
            colors.add('transparent')
            continue
        assert px[3] == 255, px
        check_pebble_color(px)
        colors.add(px[:3])
    return colors


def save(img, path):
    colors = validate(img)
    img.save(path)
    return len(colors)


def upscale(img, s):
    return img.resize((img.width * s, img.height * s), Image.NEAREST)


def on_bg(img, bg=(0xFF, 0x00, 0xFF)):
    """プレビュー用に透明部分を背景色で塗る。"""
    base = Image.new('RGBA', img.size, bg + (255,))
    base.alpha_composite(img)
    return base


# ------------------------------------------------------------
# 周期的なハッシュノイズ（背景をシームレスにタイル化するため）
# ------------------------------------------------------------
def hash2(x, y, seed=0):
    n = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0


BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]


def dither(x, y, t):
    """t(0..1)の割合でTrueを返す4x4ベイヤーディザ。"""
    return (BAYER4[y % 4][x % 4] + 0.5) / 16.0 < t
