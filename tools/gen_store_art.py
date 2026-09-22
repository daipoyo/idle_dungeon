# -*- coding: utf-8 -*-
"""ストア掲載用の画像を作る（バナーとアイコン）。

ゲーム内と同じドット絵・同じドットフォントで組み立てるので、
掲載ページと実機の見た目が揃う。

使い方:  python tools/gen_store_art.py

出力:
  docs/store/banner.png       720x320  一覧の上に出る横長の画像
  docs/store/icon_large.png   144x144
  docs/store/icon_small.png   48x48
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from PIL import Image  # noqa: E402

from pebble_art import PAL, new_image, upscale  # noqa: E402
import gb_scenes  # noqa: E402
import gb_sprites  # noqa: E402
import gb_font  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT_DIR = os.path.join(ROOT, 'docs', 'store')


class Canvas:
    """ドット単位の絵。記号は pebble_art.PAL の色。"""

    def __init__(self, w, h, fill=None):
        self.w, self.h = w, h
        self.g = [[fill] * w for _ in range(h)]

    def px(self, x, y, c):
        if c in (None, '.'):
            return
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y][x] = c

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.px(xx, yy, c)

    def blit(self, ox, oy, rows, scale=1):
        """記号の2次元リスト（または文字列の並び）を置く。"""
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch in (None, '.'):
                    continue
                for dy in range(scale):
                    for dx in range(scale):
                        self.px(ox + x * scale + dx, oy + y * scale + dy, ch)

    def text(self, ox, oy, s, color, scale=1, shadow=None):
        """ドットフォントで文字を書く。戻り値は書いた幅（ドット）。"""
        x = ox
        for ch in s:
            w, bits = gb_font.encode(ch)
            for r, v in enumerate(bits):
                for c in range(w):
                    if not (v >> c) & 1:
                        continue
                    for dy in range(scale):
                        for dx in range(scale):
                            if shadow:
                                self.px(x + c * scale + dx + scale, oy + r * scale + dy + scale, shadow)
            for r, v in enumerate(bits):
                for c in range(w):
                    if not (v >> c) & 1:
                        continue
                    for dy in range(scale):
                        for dx in range(scale):
                            self.px(x + c * scale + dx, oy + r * scale + dy, color)
            x += (w + 1) * scale
        return x - ox

    def text_width(self, s, scale=1):
        return sum((gb_font.encode(ch)[0] + 1) * scale for ch in s) - scale

    def image(self, scale):
        img = new_image(self.w, self.h)
        for y in range(self.h):
            for x in range(self.w):
                c = self.g[y][x]
                if c not in (None, '.'):
                    img.putpixel((x, y), PAL[c] + (255,))
        return upscale(img, scale)


# 剣の色（鋼）。4 = 刃、5 = 光、6 = 柄
SWORD_PAL = {'4': 'g', '5': 'W', '6': 'o', 'K': 'K'}


def hero_rows(with_sword=True):
    """勇者の1コマ（防具なしの色）。剣は手の位置に重ねる。"""
    frames = dict(gb_sprites.hero_frames())
    rows = frames['IDLE']
    pal = dict(gb_sprites.HERO_BASE)
    pal.update(gb_sprites.HERO_NO_ARMOR)
    grid = [[None if ch == '.' else pal.get(ch, ch) for ch in row] for row in rows]
    if not with_sword:
        return grid

    # 剣は勇者の右側にはみ出すので、絵を広げてから重ねる
    wpn = gb_sprites.weapon_maps()['SWORD_REST']
    ax, ay = gb_sprites.weapon_anchor(wpn)
    hx, hy = gb_sprites.hero_hand(rows)
    pad = max(0, hx - ax + len(wpn[0]) - len(grid[0]))
    grid = [row + [None] * pad for row in grid]
    for y, row in enumerate(wpn):
        for x, ch in enumerate(row):
            if ch in ('.', 'e'):
                continue
            gy, gx = hy - ay + y, hx - ax + x
            if 0 <= gy < len(grid) and 0 <= gx < len(grid[0]):
                grid[gy][gx] = SWORD_PAL.get(ch, ch)
    return grid


def banner():
    """720x320 (180x80 ドット x4)。ダンジョンの壁を背景に、勇者と魔物、上に題名。"""
    W, H, SCALE = 180, 80, 4
    cv = Canvas(W, H, 'K')
    wall = gb_scenes.dungeons()[0]          # スライムの洞窟（石壁）
    tile_h = gb_scenes.FAR_H
    for ox in range(0, W, gb_scenes.TILE_W):
        for y in range(tile_h):
            for x in range(gb_scenes.TILE_W):
                cv.px(ox + x, y + H - gb_scenes.GROUND_H - tile_h, wall[y][x])
    # 地面
    ground_y = H - gb_scenes.GROUND_H
    for ox in range(0, W, gb_scenes.TILE_W):
        for y in range(gb_scenes.GROUND_H):
            for x in range(gb_scenes.TILE_W):
                cv.px(ox + x, ground_y + y, wall[tile_h + y][x])
    # 上を暗くして題名を読みやすくする（市松模様で黒を重ねる）
    for y in range(0, 30):
        for x in range(W):
            if (x + y) % 2 == 0 or y < 22:
                cv.px(x, y, 'K')

    # 勇者と魔物
    hero = hero_rows()
    cv.blit(24, ground_y - len(hero) * 2, hero, scale=2)
    slime = gb_sprites.enemies()[0][0]
    cv.blit(W - 24 - len(slime[0]) * 2, ground_y - len(slime) * 2, slime, scale=2)

    # 題名と一言
    title = 'IDLE DUNGEON'
    tw = cv.text_width(title, scale=3)
    cv.text((W - tw) // 2, 4, title, 'Y', scale=3, shadow='b')
    sub = 'EVERY STEP TAKES YOU DEEPER'
    sw = cv.text_width(sub, scale=1)
    cv.text((W - sw) // 2, 22, sub, 'W', scale=1, shadow='K')
    return cv.image(SCALE)


def icon(size, dots, scale):
    """石畳を背景に、勇者を中央に置いた正方形のアイコン。"""
    cv = Canvas(dots, dots, 'K')
    wall = gb_scenes.dungeons()[0]
    for y in range(dots):
        for x in range(dots):
            cv.px(x, y, wall[y % gb_scenes.FAR_H][x % gb_scenes.TILE_W])
    # 中央を少し暗くして勇者を目立たせる
    for y in range(dots):
        for x in range(dots):
            if (x + y) % 2 == 0:
                cv.px(x, y, 'K')
    hero = hero_rows(with_sword=False)
    hw, hh = len(hero[0]) * scale, len(hero) * scale
    cv.blit((dots - hw) // 2, (dots - hh) // 2, hero, scale=scale)
    # 枠
    for x in range(dots):
        cv.px(x, 0, 'k')
        cv.px(x, dots - 1, 'k')
    for y in range(dots):
        cv.px(0, y, 'k')
        cv.px(dots - 1, y, 'k')
    img = cv.image(1)
    return img.resize((size, size), Image.NEAREST)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    out = [
        ('banner.png', banner()),
        ('icon_large.png', icon(144, 36, 2)),
        ('icon_small.png', icon(48, 24, 1)),
    ]
    for name, img in out:
        path = os.path.join(OUT_DIR, name)
        img.convert('RGB').save(path)
        print('%-16s %s' % (name, img.size))


if __name__ == '__main__':
    main()
