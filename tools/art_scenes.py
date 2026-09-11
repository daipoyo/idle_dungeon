# -*- coding: utf-8 -*-
"""背景画像（ダンジョン6種・町・店内）の生成。

ダンジョン背景は横スクロール用に左右がシームレスにつながる 128px 幅のタイル。
  0〜95行目  : 遠景（スクロール速度 1/2）
  96〜111行目: 地面（スクロール速度 1）
各画像は4bitパレット(16色以内)に収まるようにしている（Pebbleのメモリ節約のため）。
"""
import math
from pebble_art import new_image, put, get, hash2, dither, rgb

TILE_W = 128
FAR_H = 96
GROUND_H = 16
DUNGEON_H = FAR_H + GROUND_H


# ------------------------------------------------------------
# ノイズ・描画ヘルパー
# ------------------------------------------------------------
def ramp(x, y, v, colors):
    """v(0..1)を色の列にマップ。隣り合う色の間はベイヤーディザで補間。"""
    v = min(max(v, 0.0), 0.9999)
    n = len(colors) - 1
    p = v * n
    i = int(p)
    f = p - i
    return colors[i + 1] if dither(x, y, f) else colors[i]


def vnoise1(x, period, cells, seed):
    fx = (x % period) * cells / period
    i0 = int(math.floor(fx))
    t = fx - i0
    a = hash2(i0 % cells, 0, seed)
    b = hash2((i0 + 1) % cells, 0, seed)
    t = t * t * (3 - 2 * t)
    return a + (b - a) * t


def fbm1(x, period, seed, cells=(4, 8, 16), weights=(0.6, 0.3, 0.1)):
    return sum(w * vnoise1(x, period, c, seed + i * 31) for i, (c, w) in enumerate(zip(cells, weights)))


def vnoise2(x, y, period, cells, seed):
    cell = period / cells
    fx = (x % period) / cell
    fy = y / cell
    ix, iy = int(math.floor(fx)), int(math.floor(fy))
    tx, ty = fx - ix, fy - iy
    tx = tx * tx * (3 - 2 * tx)
    ty = ty * ty * (3 - 2 * ty)

    def h(i, j):
        return hash2(i % cells, j, seed)
    a, b = h(ix, iy), h(ix + 1, iy)
    c, d = h(ix, iy + 1), h(ix + 1, iy + 1)
    return (a + (b - a) * tx) * (1 - ty) + (c + (d - c) * tx) * ty


def fbm2(x, y, period, seed, cells=8, octaves=3):
    total, amp, norm = 0.0, 0.5, 0.0
    for o in range(octaves):
        total += amp * vnoise2(x, y, period, cells, seed + o * 17)
        norm += amp
        amp *= 0.5
        cells *= 2
    return total / norm


def voronoi(x, y, period, cell, seed):
    """横方向に周期的なボロノイ。(最近点までの距離, 2番目の距離, 最近点の座標) を返す。"""
    ncx = period // cell
    cx, cy = int(x // cell), int(y // cell)
    f1, f2, near = 1e9, 1e9, (0, 0)
    for j in range(cy - 1, cy + 2):
        for i in range(cx - 1, cx + 2):
            ii = i % ncx
            px_ = i * cell + 0.15 * cell + hash2(ii, j, seed) * cell * 0.7
            py_ = j * cell + 0.15 * cell + hash2(ii, j, seed + 1) * cell * 0.7
            d = math.hypot(x - px_, y - py_)
            if d < f1:
                f2 = f1
                f1, near = d, (px_, py_)
            elif d < f2:
                f2 = d
    return f1, f2, near


def rock_wall(cv, x0, y0, w, h, cell, seed, colors, crack='K', light_bias=0.0, depth_fn=None):
    """ボロノイで岩を敷き詰め、左上から光を当てたように陰影をつける。"""
    for y in range(y0, y0 + h):
        for x in range(x0, x0 + w):
            f1, f2, (px_, py_) = voronoi(x, y, cv.w, cell, seed)
            if f2 - f1 < 1.3:
                cv.px(x, y, crack)
                continue
            v = 0.5 - ((x - px_) + (y - py_)) / (cell * 1.2) + light_bias
            v -= f1 / (cell * 2.5)
            if depth_fn:
                v = depth_fn(x, y, v)
            cv.px(x, y, ramp(x, y, v, colors))


class Canvas:
    """左右ラップ描画に対応したキャンバス。"""

    def __init__(self, w, h, fill='K', wrap=True):
        self.img = new_image(w, h, fill)
        self.w, self.h, self.wrap = w, h, wrap

    def px(self, x, y, c):
        if c is None or c == '.':
            return
        if self.wrap:
            x %= self.w
        put(self.img, x, y, c)

    def get(self, x, y):
        if self.wrap:
            x %= self.w
        return get(self.img, x, y)[:3]

    def is_(self, x, y, c):
        return self.get(x, y) == rgb(c)

    def rect(self, x, y, w, h, c):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.px(xx, yy, c)

    def hline(self, x0, x1, y, c):
        for x in range(x0, x1 + 1):
            self.px(x, y, c)

    def vline(self, x, y0, y1, c):
        for y in range(y0, y1 + 1):
            self.px(x, y, c)

    def disc(self, cx, cy, r, c, shade=None, hi=None):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                d = math.hypot(x - cx, y - cy)
                if d <= r:
                    col = c
                    if shade and (x - cx) + (y - cy) > r * 0.6:
                        col = shade
                    if hi and (x - cx) + (y - cy) < -r * 0.9:
                        col = hi
                    self.px(x, y, col)

    def blit(self, ox, oy, rows, remap=None):
        remap = remap or {}
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch != '.':
                    self.px(ox + x, oy + y, remap.get(ch, ch))

    def outline_color(self, target, color):
        """target色の領域の周りを color で縁取る（シルエットの輪郭用）。"""
        src = [[self.get(x, y) for x in range(self.w)] for y in range(self.h)]
        t = rgb(target)
        for y in range(self.h):
            for x in range(self.w):
                if src[y][x] == t:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = (x + dx) % self.w, y + dy
                    if 0 <= yy < self.h and src[yy][xx] == t:
                        self.px(x, y, color)
                        break


# ------------------------------------------------------------
# ダンジョン0: スライムの洞窟
# ------------------------------------------------------------
def slime_cave():
    cv = Canvas(TILE_W, DUNGEON_H, 'K')
    # 岩壁（上に行くほど暗い）
    rock_wall(cv, 0, 0, TILE_W, FAR_H, 16, 11, ['K', 't', 't', 'Q', 'T'],
              depth_fn=lambda x, y, v: v * (0.35 + 0.75 * y / FAR_H))
    # 鍾乳石
    for sx, ln, wd in [(6, 30, 7), (22, 18, 5), (37, 40, 8), (55, 14, 4), (70, 34, 7),
                       (88, 22, 6), (101, 44, 9), (118, 16, 5)]:
        for i in range(ln):
            half = max(0, int(wd * (1 - i / ln) / 2 + 0.5))
            for dx in range(-half, half + 1):
                c = 'k' if dx < 0 else 't'
                if dx == -half:
                    c = 'g' if i < ln * 0.7 else 'k'
                if dx == half:
                    c = 'K'
                cv.px(sx + dx, i, c)
        # 先端のしずく
        cv.px(sx, ln + 2, 'L')
        cv.px(sx, ln + 3, 'G')
    # 光る鉱石
    for cx, cy in [(15, 62), (48, 70), (80, 58), (112, 76), (63, 84)]:
        cv.blit(cx - 2, cy - 3, [
            "..c..",
            ".cCc.",
            "cCWCT",
            ".cCT.",
            "..T..",
        ])
    # 奥の岩棚
    for x in range(TILE_W):
        top = 87 + int(fbm1(x, TILE_W, 5) * 8)
        for y in range(top, FAR_H):
            cv.px(x, y, 'K' if y > top + 1 else 'Q')
    # 地面（苔むした岩）
    for y in range(FAR_H, DUNGEON_H):
        for x in range(TILE_W):
            gy = y - FAR_H
            n = fbm2(x, gy * 2, TILE_W, 21, cells=16)
            if gy == 0:
                c = 'L' if hash2(x, 1, 3) > 0.3 else 'G'
            elif gy == 1:
                c = 'G'
            elif gy == 2:
                c = 'G' if hash2(x, 2, 3) > 0.5 else 'd'
            else:
                c = ramp(x, y, n * 0.9 + (gy / GROUND_H) * 0.2, ['t', 'd', 'K'])
            cv.px(x, y, c)
    # 地面の石
    for sx in range(4, TILE_W, 23):
        sy = FAR_H + 6 + int(hash2(sx, 0, 9) * 6)
        cv.blit(sx, sy, [".kk.", "kggk", ".kk."], {'g': 'Q'})
    return cv.img


# ------------------------------------------------------------
# ダンジョン1: ゴブリンの森
# ------------------------------------------------------------
def goblin_woods():
    cv = Canvas(TILE_W, DUNGEON_H, 'N')
    for y in range(FAR_H):
        for x in range(TILE_W):
            cv.px(x, y, ramp(x, y, y / 70.0, ['N', 'B', 'u', 'u']))
    # 月
    cv.disc(50, 26, 6, 'l', shade='s')
    # 遠くの針葉樹（シルエット）
    for layer, (base, col, step, hgt, seed) in enumerate([(62, 'B', 9, 26, 3), (74, 'd', 12, 34, 7)]):
        for tx in range(0, TILE_W, step):
            th = int(hgt * (0.6 + 0.4 * hash2(tx, layer, seed)))
            top = base - th
            for y in range(top, FAR_H):
                half = int((y - top) * 0.42) + 1
                if y > base:
                    half = 99
                for dx in range(-half, half + 1):
                    if half == 99 and abs(dx) > step:
                        continue
                    cv.px(tx + dx, y, col)
    # 手前の太い木
    for tx in (18, 82):
        for y in range(10, FAR_H):
            for dx in range(-4, 5):
                c = 'o' if dx < 0 else 'b'
                if dx == -4:
                    c = 's' if hash2(y, tx, 1) > 0.5 else 'o'
                if dx == 4:
                    c = 'K'
                if abs(dx) < 3 and hash2(tx + dx, y // 3, 2) > 0.85:
                    c = 'b'
                cv.px(tx + dx, y, c)
        # 葉のかたまり
        for (ox, oy, r) in [(0, 8, 14), (-12, 16, 10), (12, 14, 11), (-6, 0, 9), (8, 2, 9)]:
            for y in range(int(oy - r), int(oy + r + 1)):
                for x in range(int(ox - r), int(ox + r + 1)):
                    d = math.hypot(x - ox, (y - oy) * 1.2)
                    if d <= r + (hash2(tx + x, y, 4) - 0.5) * 3:
                        v = 0.35 + (y - oy) / (r * 2.5) + (x - ox) / (r * 5) + hash2(tx + x, y, 5) * 0.25
                        cv.px(tx + x, y, ramp(tx + x, y, v, ['j', 'E', 'G', 'd']))
    # 茂み
    for x in range(TILE_W):
        top = 84 + int(fbm1(x, TILE_W, 13, cells=(8, 16, 32)) * 10)
        for y in range(top, FAR_H):
            v = (y - top) / 10.0 + hash2(x, y, 6) * 0.3
            cv.px(x, y, ramp(x, y, v, ['E', 'G', 'd']))
    # 地面（草と土の道）
    for y in range(FAR_H, DUNGEON_H):
        gy = y - FAR_H
        for x in range(TILE_W):
            if gy < 3:
                blade = hash2(x, 0, 8)
                c = 'j' if gy == 0 and blade > 0.55 else ('E' if gy < 2 else 'G')
                if gy == 0 and blade < 0.3:
                    c = 'd'
            else:
                n = fbm2(x, gy * 2, TILE_W, 23, cells=16)
                c = ramp(x, y, n * 0.8 + gy / 40.0, ['s', 'o', 'b'])
            cv.px(x, y, c)
    for sx in range(7, TILE_W, 19):
        cv.px(sx, FAR_H + 8 + (sx % 5), 'k')
        cv.px(sx + 1, FAR_H + 8 + (sx % 5), 'k')
    return cv.img


# ------------------------------------------------------------
# ダンジョン2: 古代遺跡
# ------------------------------------------------------------
def ancient_ruins():
    cv = Canvas(TILE_W, DUNGEON_H, 'X')
    for y in range(FAR_H):
        for x in range(TILE_W):
            cv.px(x, y, ramp(x, y, y / 60.0, ['X', 'q', 'S', 's']))
    cv.disc(40, 44, 10, 'l', hi='W')
    # 遠くのピラミッド
    for px_, base, hw in [(40, 62, 26), (100, 62, 18)]:
        for y in range(base - hw, base + 1):
            half = y - (base - hw)
            for dx in range(-half, half + 1):
                cv.px(px_ + dx, y, 'v' if dx < half * 0.2 else 'q')
    for y in range(62, 70):
        for x in range(TILE_W):
            cv.px(x, y, 'v')
    # 崩れた石壁（レンガ）
    for x in range(TILE_W):
        top = 44 + int(fbm1(x, TILE_W, 17, cells=(4, 8, 32), weights=(0.5, 0.3, 0.2)) * 30)
        for y in range(top, FAR_H):
            bx = (x + (4 if (y // 6) % 2 else 0)) % 8
            by = y % 6
            c = 'A'
            if by == 0 or bx == 0:
                c = 'a'
            elif by == 1 or bx == 1:
                c = 'l' if hash2(x // 8, y // 6, 3) > 0.4 else 'A'
            if y - top < 1:
                c = 'l'
            if hash2(x // 8, y // 6, 5) > 0.93 and by and bx:
                c = 'a'
            cv.px(x, y, c)
    # 柱
    for cx in (20, 76, 110):
        top = 18 + int(hash2(cx, 0, 4) * 22)
        for y in range(top, FAR_H):
            for dx in range(-5, 6):
                if abs(dx) == 5 and y > top + 4:
                    continue
                c = 'l' if dx < -2 else ('A' if dx < 3 else 'a')
                if y < top + 4:
                    c = 'l' if y == top else ('A' if dx < 3 else 'a')
                if (dx in (-2, 2)) and y > top + 4:
                    c = 'a' if dx == 2 else 'A'
                cv.px(cx + dx, y, c)
        for y in range(top + 4, top + 7):
            cv.px(cx - 5, y, 'K')
    # ツタ
    for vx in (8, 50, 95):
        ln = 14 + int(hash2(vx, 1, 2) * 20)
        for i in range(ln):
            x = vx + int(math.sin(i * 0.5) * 1.5)
            cv.px(x, 50 + i, 'E')
            if i % 4 == 0:
                cv.px(x + 1, 50 + i, 'E')
    # 地面（砂岩の床）
    for y in range(FAR_H, DUNGEON_H):
        gy = y - FAR_H
        for x in range(TILE_W):
            tx = (x + (8 if (gy // 8) % 2 else 0)) % 16
            if gy == 0:
                c = 'l'
            elif gy % 8 == 0 or tx == 0:
                c = 'o'
            elif gy % 8 == 1 or tx == 1:
                c = 'l' if hash2(x, gy, 1) > 0.3 else 's'
            else:
                c = 's' if hash2(x, gy, 2) > 0.12 else 'o'
            cv.px(x, y, c)
    return cv.img


# ------------------------------------------------------------
# ダンジョン3: ドラゴンの巣
# ------------------------------------------------------------
def dragons_lair():
    cv = Canvas(TILE_W, DUNGEON_H, 'K')
    # 奥の岩壁（下の溶岩に照らされて赤く光る）
    rock_wall(cv, 0, 0, TILE_W, FAR_H, 14, 41, ['K', 'b', 'r', 'R'],
              depth_fn=lambda x, y, v: v * (0.15 + 0.95 * (y / FAR_H) ** 1.5))
    # 溶岩の滝
    for lx in (34, 98):
        for y in range(0, FAR_H):
            wob = int(math.sin(y * 0.35 + lx) * 1.2)
            for dx in range(-3, 4):
                c = 'y' if abs(dx) <= 1 else ('O' if abs(dx) == 2 else 'R')
                if hash2(y, dx, lx) > 0.85:
                    c = 'Y'
                cv.px(lx + dx + wob, y, c)
    # 手前の岩柱（黒いシルエット＋赤い縁取り）
    for cx, top, wd in [(10, 26, 10), (66, 40, 8), (118, 18, 12)]:
        for y in range(top, FAR_H):
            half = wd // 2 + int((y - top) * 0.12) + int(math.sin(y * 0.4 + cx) * 1.2)
            for dx in range(-half, half + 1):
                c = 'K'
                if dx == -half:
                    c = 'r'
                elif dx == half:
                    c = 'R' if y > top + 10 else 'r'
                cv.px(cx + dx, y, c)
        for dx in range(-(wd // 2) + 1, wd // 2):
            cv.px(cx + dx, top - 1, 'b')
    # 財宝の山（ドラゴンの寝床）
    for x in range(TILE_W):
        h = int(7 + math.sin(x * 2 * math.pi / TILE_W * 3) * 4 + hash2(x, 0, 7) * 2)
        for y in range(FAR_H - h, FAR_H):
            c = 'y' if hash2(x, y, 9) > 0.4 else 'o'
            if hash2(x, y, 10) > 0.9:
                c = 'Y'
            if y == FAR_H - h:
                c = 'l' if hash2(x, 3, 3) > 0.5 else 'Y'
            cv.px(x, y, c)
    # 火の粉
    for i in range(24):
        ex = int(hash2(i, 0, 61) * TILE_W)
        ey = int(hash2(i, 1, 61) * 70)
        cv.px(ex, ey, 'y' if i % 3 else 'Y')
    # 地面（黒い岩＋溶岩の亀裂）
    for y in range(FAR_H, DUNGEON_H):
        gy = y - FAR_H
        for x in range(TILE_W):
            n = fbm2(x, gy * 2, TILE_W, 45, cells=16)
            c = ramp(x, y, n * 0.9, ['k', 'K', 'K'])
            if gy == 0:
                c = 'k'
            cv.px(x, y, c)
    for cx in range(0, TILE_W, 32):
        x, y = cx + 3, FAR_H + 4
        for i in range(14):
            cv.px(x, y, 'O' if i % 3 else 'y')
            x += 1
            if hash2(i, cx, 1) > 0.5:
                y += 1 if y < DUNGEON_H - 3 else -1
            cv.px(x, y + 1, 'R')
    return cv.img


# ------------------------------------------------------------
# ダンジョン4: 海底神殿
# ------------------------------------------------------------
def sunken_temple():
    cv = Canvas(TILE_W, DUNGEON_H, 'N')
    for y in range(FAR_H):
        for x in range(TILE_W):
            cv.px(x, y, ramp(x, y, y / FAR_H, ['U', 'B', 'n', 'N']))
    # 差し込む光
    for rx in (10, 58, 92):
        for y in range(0, 70):
            for dx in range(0, 7):
                x = rx + dx + y // 3
                if dither(x, y, 0.35 * (1 - y / 70)):
                    cv.px(x, y, 'u')
    # 神殿（奥のペディメント＋柱）
    for x in range(TILE_W):
        for y in range(30, 36):
            cv.px(x, y, 'Q' if y == 30 else ('T' if y < 33 else 't'))
    for cx in range(8, TILE_W, 32):
        for y in range(36, FAR_H):
            for dx in range(-4, 5):
                c = 'C' if dx == -3 else ('Q' if dx < 0 else ('T' if dx < 3 else 't'))
                if abs(dx) == 4:
                    c = 't'
                if dx in (-1, 2) and y % 4 == 0:
                    c = 't'
                cv.px(cx + dx, y, c)
        for y in range(36, 39):
            for dx in range(-6, 7):
                cv.px(cx + dx, y, 'Q' if y == 36 else 't')
    # 崩れた部分
    for bx, bw in [(30, 12), (90, 8)]:
        for x in range(bx, bx + bw):
            top = 30 + int(hash2(x, 0, 3) * 5)
            for y in range(30, top + 3):
                cv.px(x, y, ramp(x, y, y / FAR_H, ['U', 'B', 'n', 'N']))
    # 海藻
    for sx in range(4, TILE_W, 17):
        ln = 12 + int(hash2(sx, 0, 5) * 18)
        for i in range(ln):
            x = sx + int(math.sin(i * 0.45 + sx) * 2)
            cv.px(x, FAR_H - 1 - i, 'G' if i % 3 else 'd')
            cv.px(x + 1, FAR_H - 1 - i, 'd')
    # 泡
    for i in range(12):
        bx = int(hash2(i, 0, 71) * TILE_W)
        by = int(hash2(i, 1, 71) * 80) + 4
        r = 1 if i % 3 else 2
        if r == 1:
            cv.px(bx, by, 'C')
        else:
            cv.blit(bx - 1, by - 1, [".C.", "CWC", ".C."])
    # 地面（砂）
    for y in range(FAR_H, DUNGEON_H):
        gy = y - FAR_H
        for x in range(TILE_W):
            wave = int(math.sin(x * 2 * math.pi / 32) * 1.5)
            v = (gy + wave) / 14.0 + hash2(x, y, 3) * 0.2
            c = ramp(x, y, v, ['l', 'A', 'A', 'a'])
            if gy + wave <= 0:
                c = 'l'
            cv.px(x, y, c)
    for sx in range(9, TILE_W, 29):
        cv.blit(sx, FAR_H + 6, [".hh.", "hWhh", ".hh."])
    return cv.img


# ------------------------------------------------------------
# ダンジョン5: 深淵
# ------------------------------------------------------------
def abyssal_depths():
    cv = Canvas(TILE_W, DUNGEON_H, 'K')
    for y in range(FAR_H):
        for x in range(TILE_W):
            n = fbm2(x, y, TILE_W, 51, cells=4, octaves=4)
            v = (n - 0.35) * 1.6
            cv.px(x, y, ramp(x, y, v, ['K', 'K', 'N', 'P', 'X']))
    # 星・光の粒
    for i in range(40):
        sx = int(hash2(i, 0, 81) * TILE_W)
        sy = int(hash2(i, 1, 81) * 80)
        cv.px(sx, sy, 'W' if i % 5 == 0 else ('m' if i % 2 else 'x'))
    # 巨大な目（奥）
    cv.blit(52, 14, [
        "....PPPPPPPP....",
        "..PPpppppppPPP..",
        ".PppxxxxxxxxppP.",
        "PppxxmmRRmmxxppP",
        "PppxxmRKKRmxxppP",
        ".PppxxmmmmxxxpP.",
        "..PPpppppppPPP..",
        "....PPPPPPPP....",
    ])
    # 浮遊する岩
    for cx, cy, w in [(18, 40, 18), (96, 52, 22), (60, 70, 12)]:
        for dy in range(-3, 10):
            half = int(w / 2 * (1 - max(0, dy) / 10.0)) if dy >= 0 else int(w / 2 * (1 + dy / 6.0))
            for dx in range(-half, half + 1):
                c = 'p' if dy < 0 else ('P' if dx < half * 0.4 else 'K')
                if dy == -3 or (dy < 0 and abs(dx) == half):
                    c = 'x'
                cv.px(cx + dx, cy + dy, c)
        cv.px(cx - 2, cy - 5, 'm')
        cv.px(cx - 2, cy - 4, 'x')
        cv.px(cx + 3, cy - 4, 'm')
    # 地面（黒曜石＋ルーン）
    for y in range(FAR_H, DUNGEON_H):
        gy = y - FAR_H
        for x in range(TILE_W):
            n = fbm2(x, gy * 2, TILE_W, 55, cells=16)
            c = ramp(x, y, n, ['P', 'K', 'K'])
            if gy == 0:
                c = 'x' if hash2(x, 0, 2) > 0.5 else 'p'
            cv.px(x, y, c)
    for rx in range(6, TILE_W, 21):
        cv.blit(rx, FAR_H + 5, ["m.m", ".m.", "m.m"] if rx % 2 else ["mmm", "m..", "mmm"])
    return cv.img


# ------------------------------------------------------------
# 町 (260x112, 中央にお店)
# ------------------------------------------------------------
TOWN_W = 260
TOWN_H = 112


def house(cv, x, w, h, roof, wall='l', timber='o', floors=1, door=True, chimney=False):
    base = FAR_H
    top = base - h
    # 壁
    for y in range(top, base):
        for xx in range(x, x + w):
            c = wall
            if xx in (x, x + w - 1) or (y - top) % 12 == 0:
                c = timber
            if xx == x + w - 2:
                c = 's' if wall == 'l' else wall
            cv.px(xx, y, c)
    # 屋根
    rh = w // 2 + 2
    for i in range(rh):
        y = top - rh + i
        half = i + 2
        for dx in range(-half, half + 1):
            xx = x + w // 2 + dx
            if xx < x - 3 or xx > x + w + 2:
                continue
            c = roof
            if dx >= half - 1 or (i % 3 == 2 and dx > -half):
                c = {'r': 'b', 'R': 'r', 'o': 'b'}.get(roof, 'b')
            if dx <= -half + 1:
                c = 'R' if roof == 'r' else ('y' if roof == 'o' else 'W')
            cv.px(xx, y, c)
    if chimney:
        cv.rect(x + w - 8, top - rh - 2, 4, rh - 2, 'k')
        cv.rect(x + w - 9, top - rh - 3, 6, 2, 'K')
    # 窓
    for fx in range(x + 4, x + w - 7, 10):
        for fy in range(top + 3, base - 12, 12):
            cv.rect(fx, fy, 5, 6, 'N')
            cv.rect(fx, fy, 5, 1, 'K')
            cv.px(fx + 1, fy + 1, 'u')
            cv.px(fx + 2, fy + 1, 'u')
            cv.px(fx + 1, fy + 2, 'u')
            cv.rect(fx - 1, fy + 6, 7, 1, timber)
    if door:
        dx = x + w // 2 - 3
        cv.rect(dx, base - 11, 7, 11, 'b')
        cv.rect(dx + 1, base - 10, 5, 10, 'o')
        cv.px(dx + 4, base - 5, 'y')


def town():
    cv = Canvas(TOWN_W, TOWN_H, 'u', wrap=False)
    for y in range(FAR_H):
        for x in range(TOWN_W):
            cv.px(x, y, ramp(x, y, y / 80.0, ['u', 'u', 'C', 'W']))
    # 雲
    for cx, cy, s in [(30, 14, 1.0), (120, 8, 1.3), (205, 18, 0.9)]:
        for (ox, oy, r) in [(0, 0, 6), (8, -3, 7), (16, 0, 6), (8, 2, 6)]:
            cv.disc(cx + ox * s, cy + oy * s, r * s, 'W')
        for x in range(int(cx - 6 * s), int(cx + 22 * s)):
            y = int(cy + 5 * s)
            if get(cv.img, x, y)[:3] == rgb('W'):
                cv.px(x, y, 'C')
    # 遠くの丘
    for x in range(TOWN_W):
        top = 58 + int(math.sin(x / 23.0) * 6 + math.sin(x / 9.0) * 2)
        for y in range(top, FAR_H):
            cv.px(x, y, 'E' if dither(x, y, 0.8 - (y - top) / 40.0) else 'G')
    # 木
    for tx in (8, 250, 88, 172):
        cv.rect(tx - 1, 78, 3, 18, 'o')
        cv.disc(tx, 72, 9, 'G', shade='E', hi='E')
        cv.disc(tx - 3, 69, 4, 'E')
    # 家々（中央=お店）
    house(cv, 14, 40, 34, 'r', chimney=True)
    house(cv, 62, 32, 28, 'o')
    house(cv, 166, 34, 30, 'o', chimney=True)
    house(cv, 208, 40, 36, 'r')
    # --- お店（中央） ---
    sx, sw, sh = 104, 52, 42
    house(cv, sx, sw, sh, 'R', wall='s', timber='b', door=False)
    # 日よけ(ストライプ)
    for y in range(FAR_H - 26, FAR_H - 21):
        for x in range(sx - 3, sx + sw + 3):
            cv.px(x, y, 'R' if ((x - sx) // 4) % 2 == 0 else 'W')
    for x in range(sx - 3, sx + sw + 3, 4):
        cv.px(x + 1, FAR_H - 21, 'R' if ((x - sx) // 4) % 2 == 0 else 'W')
    # ショーウィンドウとドア
    cv.rect(sx + 5, FAR_H - 18, 14, 11, 'N')
    cv.rect(sx + 33, FAR_H - 18, 14, 11, 'N')
    for (wx, wy, c) in [(sx + 8, FAR_H - 12, 'R'), (sx + 12, FAR_H - 12, 'y'), (sx + 16, FAR_H - 12, 'E'),
                        (sx + 36, FAR_H - 12, 'y'), (sx + 40, FAR_H - 12, 'R'), (sx + 44, FAR_H - 12, 'W')]:
        cv.rect(wx, wy, 2, 4, c)
        cv.px(wx, wy - 1, 'g')
    cv.rect(sx + 21, FAR_H - 16, 10, 16, 'b')
    cv.rect(sx + 22, FAR_H - 15, 8, 15, 'o')
    cv.px(sx + 28, FAR_H - 8, 'y')
    # 看板（ハートとコイン）
    cv.rect(sx + 14, FAR_H - 40, 24, 11, 'b')
    cv.rect(sx + 15, FAR_H - 39, 22, 9, 'y')
    cv.blit(sx + 18, FAR_H - 38, [
        ".RR.RR..",
        "RRRRRRR.",
        "RRRRRRR.",
        ".RRRRR..",
        "..RRR...",
        "...R....",
    ])
    cv.disc(sx + 31, FAR_H - 35, 3, 'l', shade='o')
    # 地面（石畳）
    for y in range(FAR_H, TOWN_H):
        gy = y - FAR_H
        for x in range(TOWN_W):
            bx = (x + (5 if (gy // 5) % 2 else 0)) % 10
            c = 'g'
            if gy % 5 == 0 or bx == 0:
                c = 'k'
            elif gy % 5 == 1 and bx < 8:
                c = 'W' if hash2(x, gy, 1) > 0.6 else 'g'
            if gy == 0:
                c = 'G'
            cv.px(x, y, c)
    return cv.img


# ------------------------------------------------------------
# 店内（壁 128x120 + カウンター 128x24 を縦に並べた 128x144）
# ------------------------------------------------------------
SHOP_WALL_H = 120
SHOP_COUNTER_H = 24


def shop_interior():
    W = TILE_W
    cv = Canvas(W, SHOP_WALL_H + SHOP_COUNTER_H, 'P')
    # ダマスク柄の紫の壁紙（上に行くほど暗い）
    motif = [
        "....J....",
        "...JqJ...",
        "..J.q.J..",
        ".J.JqJ.J.",
        "J.JqqqJ.J",
        ".J.JqJ.J.",
        "..J.q.J..",
        "...JqJ...",
        "....J....",
    ]
    for y in range(SHOP_WALL_H):
        for x in range(W):
            c = 'P'
            if y < 14 and dither(x, y, 1 - y / 14.0):
                c = 'K'
            cv.px(x, y, c)
    for my in range(4, SHOP_WALL_H, 16):
        for mx in range(0, W, 16):
            ox = mx + (8 if (my // 16) % 2 else 0)
            cv.blit(ox - 4, my, motif)
    # 腰板（木）
    for y in range(SHOP_WALL_H - 22, SHOP_WALL_H):
        for x in range(W):
            c = 'o'
            if x % 16 == 0:
                c = 'b'
            elif x % 16 == 1:
                c = 's'
            cv.px(x, y, c)
    cv.hline(0, W - 1, SHOP_WALL_H - 23, 'y')
    cv.hline(0, W - 1, SHOP_WALL_H - 24, 'b')
    # 棚
    shelves = [24, 56, 88]
    for sy in shelves:
        cv.rect(0, sy, W, 3, 'o')
        cv.hline(0, W - 1, sy, 's')
        cv.rect(0, sy + 3, W, 1, 'K')
        for bx in range(6, W, 32):
            cv.blit(bx, sy + 3, ["bo", ".b"])
    # 棚の上の品物（瓶・壺・本）
    potion = [".gg.", ".WW.", "cccc", "cWcc", "cccc", ".cc."]
    jar = [".KK.", "gggg", "gWgg", "gggg", "Kggk"]
    book = ["rr", "rr", "rr", "rr", "rr", "rr", "rr"]
    jar = [r.replace('k', 'K') for r in jar]
    for si, sy in enumerate(shelves):
        x = 3 + si * 5
        i = 0
        while x < W - 6:
            kind = int(hash2(i, si, 7) * 5)
            if kind in (0, 1, 2):
                col = ['R', 'u', 'L', 'm', 'Y'][int(hash2(i, si, 9) * 5)]
                cv.blit(x, sy - 6, potion, {'c': col})
                x += 6
            elif kind == 3:
                cv.blit(x, sy - 5, jar)
                x += 6
            else:
                for b in range(3):
                    col = ['R', 'u', 'y'][(b + i) % 3]
                    hgt = 6 + (b % 2)
                    cv.blit(x + b * 2, sy - hgt, book[:hgt], {'r': col})
                x += 8
            i += 1
    # 吊り下げランプ
    for lx in (20, 84):
        cv.vline(lx, 0, 6, 'K')
        cv.blit(lx - 3, 7, [
            ".yyyyy.",
            "yYYlYYy",
            ".ylWly.",
            "..yYy..",
        ])
    # カウンター
    cy = SHOP_WALL_H
    for y in range(cy, cy + SHOP_COUNTER_H):
        gy = y - cy
        for x in range(W):
            if gy == 0:
                c = 'l'
            elif gy < 3:
                c = 's'
            elif gy == 3:
                c = 'b'
            elif gy == 4:
                c = 'y'
            else:
                px_ = x % 32
                c = 'o'
                if px_ in (0, 31) or gy in (6, SHOP_COUNTER_H - 2):
                    c = 'b'
                elif px_ == 1 or gy == 7:
                    c = 's'
                if gy == SHOP_COUNTER_H - 1:
                    c = 'K'
            cv.px(x, y, c)
    return cv.img


def make_all():
    return {
        'bg_dungeon0': slime_cave(),
        'bg_dungeon1': goblin_woods(),
        'bg_dungeon2': ancient_ruins(),
        'bg_dungeon3': dragons_lair(),
        'bg_dungeon4': sunken_temple(),
        'bg_dungeon5': abyssal_depths(),
        'bg_town': town(),
        'bg_shop': shop_interior(),
    }
