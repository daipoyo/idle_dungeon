# -*- coding: utf-8 -*-
"""背景（ダンジョン6種・町）のドット絵。単位はドット（画面では2倍）。

ダンジョンの背景は横に繰り返し表示され、遠景（上64ドット）と地面（下8ドット）が
別々の速さで流れる。そのため左端と右端がつながるように描く（座標は横方向に折り返す）。
"""
from pebble_art import hash2

TILE_W, FAR_H, GROUND_H = 64, 64, 8
TOWN_W, TOWN_H = 130, 56


class Grid:
    def __init__(self, w, h, fill, wrap=True):
        self.w, self.h, self.wrap = w, h, wrap
        self.g = [[fill] * w for _ in range(h)]

    def px(self, x, y, c):
        if c is None or c == '.':
            return
        if self.wrap:
            x %= self.w
        if 0 <= x < self.w and 0 <= y < self.h:
            self.g[y][x] = c

    def get(self, x, y):
        if self.wrap:
            x %= self.w
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.g[y][x]
        return None

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

    def blit(self, ox, oy, rows, pal=None):
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch == '.':
                    continue
                self.px(ox + x, oy + y, pal[int(ch)] if pal else ch)


def top_color(grid):
    row = grid[0]
    return max(set(row), key=row.count)


def rand(x, y, seed):
    return hash2(x, y, seed)


# ------------------------------------------------------------
# 共通パーツ
# ------------------------------------------------------------
def stone_blocks(cv, y0, y1, bw, bh, pal, seed, stagger=True):
    """レンガ積みの石壁。pal = (目地, 影, 面, 光)。"""
    k, dark, mid, light = pal
    for y in range(y0, y1):
        ry = (y - y0) // bh
        by = (y - y0) % bh
        off = (bw // 2) * (ry % 2) if stagger else 0
        for x in range(cv.w):
            bx = (x + off) % bw
            if by == bh - 1 or bx == bw - 1:
                c = k
            elif by == 0 or bx == 0:
                c = light
            elif by == bh - 2 or bx == bw - 2:
                c = dark
            else:
                c = mid
                if rand((x + off) // bw * 7, ry, seed) > 0.75 and (x + y) % 3 == 0:
                    c = dark
            cv.px(x, y, c)


def ground_tiles(cv, y0, pal, tw=8, seed=1):
    """地面（8ドット）。pal = (線, 影, 面, 光)。"""
    k, dark, mid, light = pal
    for y in range(y0, y0 + GROUND_H):
        yy = y - y0
        for x in range(cv.w):
            if yy == 0:
                c = light
            elif yy == 1:
                c = k
            elif yy == 4 or (x + (0 if yy < 4 else tw // 2)) % tw == 0:
                c = dark
            else:
                c = mid
                if rand(x, yy, seed) > 0.9:
                    c = light
            cv.px(x, y, c)


# ------------------------------------------------------------
# ダンジョン
# ------------------------------------------------------------
def slime_cave():
    far = ('K', 't', 'Q', 'C')
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'K')
    stone_blocks(cv, 6, FAR_H, 12, 7, ('K', 'K', 't', 'Q'), 3)
    # 天井の鍾乳石
    cv.rect(0, 0, TILE_W, 6, 'K')
    for i, x in enumerate(range(2, TILE_W, 9)):
        ln = 4 + (i * 5) % 7
        for d in range(ln):
            half = max(0, (ln - d) // 3)
            cv.hline(x - half, x + half, 6 + d, far[1] if d < ln - 1 else far[2])
        cv.px(x, 5 + ln, far[3])
    # 壁をつたうスライム
    for x, top, ln in ((20, 22, 20), (47, 34, 14)):
        cv.vline(x, top, top + ln, 'G')
        cv.vline(x + 1, top, top + ln - 4, 'L')
        cv.rect(x - 1, top + ln, 3, 2, 'G')
    ground_tiles(cv, FAR_H, ('K', 't', 'Q', 'C'), seed=5)
    return cv.g


def goblin_woods():
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'd')
    # 奥の木の幹
    for x in (4, 26, 44):
        cv.rect(x, 0, 5, FAR_H, 'K')
        cv.rect(x + 1, 0, 3, FAR_H, 'd')
        for y in range(0, FAR_H, 5):
            cv.px(x + 2, y + (x % 3), 'K')
    # 手前の太い幹
    for x in (14, 54):
        cv.rect(x, 8, 7, FAR_H - 8, 'K')
        cv.rect(x + 1, 8, 5, FAR_H - 8, 'o')
        cv.vline(x + 2, 8, FAR_H, 'b')
        cv.vline(x + 5, 10, FAR_H, 'b')
    # 葉っぱ（上側）
    for y in range(0, 16):
        for x in range(TILE_W):
            wave = 10 + (3 if (x // 8) % 2 else 0) + ((x * 3) % 5 == 0)
            if y < wave:
                c = 'G' if (x + y) % 2 == 0 or y < wave - 3 else 'd'
                if y < wave - 2 and rand(x, y, 11) > 0.8:
                    c = 'j'
                cv.px(x, y, c)
            elif y == wave:
                cv.px(x, y, 'K')
    # 草むら（下側）
    for x in range(TILE_W):
        h = 3 + ((x * 7) % 4 == 0) + ((x // 5) % 2)
        for y in range(FAR_H - h, FAR_H):
            cv.px(x, y, 'G' if y > FAR_H - h else 'j')
    ground_tiles(cv, FAR_H, ('K', 'b', 'o', 's'), tw=16, seed=7)
    return cv.g


def ancient_ruins():
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'k')
    stone_blocks(cv, 0, FAR_H, 10, 6, ('K', 'K', 'k', 'g'), 13)
    # 柱
    for x in (6, 38):
        cv.rect(x - 1, 2, 12, 4, 'K')
        cv.rect(x, 3, 10, 2, 'W')
        cv.rect(x + 1, 6, 8, FAR_H - 10, 'K')
        cv.rect(x + 2, 6, 6, FAR_H - 10, 'g')
        cv.vline(x + 3, 6, FAR_H - 5, 'W')
        cv.vline(x + 7, 6, FAR_H - 5, 'k')
        cv.rect(x - 1, FAR_H - 4, 12, 4, 'K')
        cv.rect(x, FAR_H - 3, 10, 2, 'g')
    # 崩れた柱
    cv.rect(24, FAR_H - 9, 8, 9, 'K')
    cv.rect(25, FAR_H - 8, 6, 8, 'g')
    cv.px(26, FAR_H - 9, 'K'); cv.px(29, FAR_H - 9, 'K')
    # つた
    for x, ln in ((13, 18), (46, 12), (58, 22)):
        for y in range(6, 6 + ln):
            cv.px(x + (y // 3) % 2, y, 'E')
            if y % 4 == 0:
                cv.px(x + 2, y, 'L')
    ground_tiles(cv, FAR_H, ('K', 'a', 'A', 'l'), tw=12, seed=9)
    return cv.g


def dragons_lair():
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'b')
    # ごつごつした岩肌
    for y in range(FAR_H):
        for x in range(TILE_W):
            v = rand(x // 6, y // 5, 21) + rand((x + 3) // 6, (y + 2) // 5, 22)
            if v > 1.55:
                cv.px(x, y, 'r')
            elif v < 0.8:
                cv.px(x, y, 'K')
    # 溶岩の滝
    for x in (10, 42):
        for y in range(0, FAR_H):
            cv.px(x - 1, y, 'K')
            cv.px(x + 4, y, 'K')
            for dx in range(4):
                cv.px(x + dx, y, 'O' if (y + dx * 2) % 5 else 'y')
    # 溶岩だまりの光
    for y in range(FAR_H - 6, FAR_H):
        for x in range(TILE_W):
            if (x + y * 2) % 7 == 0:
                cv.px(x, y, 'O')
    ground_tiles(cv, FAR_H, ('K', 'b', 'r', 'O'), tw=10, seed=23)
    for x in range(0, TILE_W, 10):
        cv.px(x + 3, FAR_H + 6, 'y')
    return cv.g


def sunken_temple():
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'N')
    stone_blocks(cv, 0, FAR_H, 16, 8, ('N', 'n', 'B', 'U'), 31)
    # アーチ
    r = 8
    for cx in (16, 48):
        for y in range(34 - r, FAR_H):
            dy = 34 - y
            half = int((r * r - dy * dy) ** 0.5) if dy > 0 else r
            cv.hline(cx - half - 1, cx + half + 1, y, 'U' if dy > 0 else 'K')
            cv.hline(cx - half, cx + half, y, 'N')
    # 泡
    for i, (x, y) in enumerate(((6, 46), (8, 38), (5, 30), (33, 52), (35, 42), (58, 36), (60, 27))):
        cv.px(x, y, 'C')
        cv.px(x + 1, y - 1, 'u')
    # 水面のゆらぎ
    for x in range(TILE_W):
        if (x // 4) % 2 == 0:
            cv.px(x, 2, 'u')
        else:
            cv.px(x, 3, 'u')
    ground_tiles(cv, FAR_H, ('N', 'B', 'U', 'C'), tw=16, seed=33)
    return cv.g


def abyssal_depths():
    cv = Grid(TILE_W, FAR_H + GROUND_H, 'K')
    # 星
    for y in range(FAR_H):
        for x in range(TILE_W):
            r = rand(x, y, 41)
            if r > 0.985:
                cv.px(x, y, 'H')
            elif r > 0.965:
                cv.px(x, y, 'x')
    # 渦（紫の帯）
    for x in range(TILE_W):
        yc = 30 + int(6 * ((x % 32) - 16 if (x // 32) % 2 == 0 else 16 - (x % 32)) / 16)
        cv.px(x, yc, 'X')
        cv.px(x, yc + 1, 'P')
        cv.px(x, yc + 12, 'P')
    # 浮かぶ結晶
    crystal = [
        "..0..",
        ".010.",
        ".0120",
        "01120",
        "01220",
        ".022.",
        "..0..",
    ]
    for x, y in ((8, 14), (40, 40), (54, 10)):
        cv.blit(x, y, crystal, ('K', 'x', 'H'))
    # 下側の岩
    for x in range(TILE_W):
        h = 4 + (x * 5 % 7 == 0) * 2 + ((x // 9) % 2)
        cv.vline(x, FAR_H - h, FAR_H - 1, 'P')
        cv.px(x, FAR_H - h, 'X')
    ground_tiles(cv, FAR_H, ('K', 'X', 'P', 'x'), tw=8, seed=43)
    return cv.g


def dungeons():
    return [slime_cave(), goblin_woods(), ancient_ruins(), dragons_lair(),
            sunken_temple(), abyssal_depths()]


# ------------------------------------------------------------
# 町
# ------------------------------------------------------------
def house(cv, x, w, h, roof, wall, door=True):
    base = TOWN_H - GROUND_H
    top = base - h
    # 壁
    cv.rect(x, top, w, h, 'K')
    cv.rect(x + 1, top + 1, w - 2, h - 1, wall[0])
    cv.vline(x + w - 2, top + 1, base - 1, wall[1])
    # 屋根
    rh = w // 2
    for i in range(rh):
        y = top - rh + i + 1
        cv.hline(x + rh - i - 2, x + w - rh + i + 1, y, 'K')
        if i > 0:
            cv.hline(x + rh - i - 1, x + w - rh + i, y, roof[0] if (y % 2) else roof[1])
    # 窓
    for wx in range(x + 3, x + w - 4, 6):
        cv.rect(wx, top + 3, 3, 3, 'K')
        cv.px(wx + 1, top + 4, 'u')
    if door:
        cx = x + w // 2 - 2
        cv.rect(cx, base - 6, 4, 6, 'K')
        cv.rect(cx + 1, base - 5, 2, 5, 'o')


def light_house(cv, x, w, h):
    """夜: 家の窓と戸口に明かりをともす（house と同じ位置）。"""
    base = TOWN_H - GROUND_H
    top = base - h
    for wx in range(x + 3, x + w - 4, 6):
        cv.px(wx + 1, top + 4, 'Y')
        cv.px(wx + 1, top + 6, 'A')   # 窓の明かりが壁にこぼれる
    cx = x + w // 2 - 2
    cv.rect(cx + 1, base - 5, 2, 5, 'b')
    cv.px(cx + 1, base - 5, 'y')


# 夜の色の置き換え（昼の絵を描いてから、空・丘・壁・屋根・石畳を暗くする）
NIGHT_COLORS = {
    'u': 'N',            # 空
    'G': 'd', 'j': 't',  # 丘
    'l': 'i', 's': 'X',  # 壁（明るい面と影）、橙の屋根の影
    'W': 'I', 'g': 'i',  # 白い壁、石畳
    'r': 'b', 'R': 'r',  # 赤い屋根
    'o': 'P',            # 橙の屋根
    'E': 't',            # 木の葉
    'k': 'N',            # 石畳の影
}


def town(night=False):
    cv = Grid(TOWN_W, TOWN_H, 'u', wrap=False)
    if not night:
        # 雲
        for cx, cy in ((12, 6), (58, 3), (104, 8)):
            cv.rect(cx, cy + 1, 12, 3, 'W')
            cv.rect(cx + 3, cy, 6, 1, 'W')
            cv.hline(cx + 1, cx + 10, cy + 4, 'C')
    # 丘
    for x in range(TOWN_W):
        h = 10 + int(4 * abs(((x % 40) - 20) / 20.0))
        cv.vline(x, TOWN_H - GROUND_H - h, TOWN_H - GROUND_H - 1, 'G')
        cv.px(x, TOWN_H - GROUND_H - h, 'j')
    houses = [
        (2, 20, 16, ('r', 'R'), ('l', 's')),
        (26, 16, 12, ('o', 's'), ('W', 'g')),
        (52, 26, 18, ('R', 'r'), ('l', 's')),   # お店（中央、看板付き）
        (88, 16, 12, ('o', 's'), ('W', 'g')),
        (108, 20, 16, ('r', 'R'), ('l', 's')),
    ]
    for hx, hw, hh, roof, wall in houses:
        house(cv, hx, hw, hh, roof, wall)
    # 木
    for tx in (46, 83, 128):
        base = TOWN_H - GROUND_H
        cv.rect(tx, base - 4, 2, 4, 'b')
        cv.rect(tx - 2, base - 11, 6, 7, 'K')
        cv.rect(tx - 1, base - 10, 4, 5, 'E')
        cv.px(tx, base - 9, 'j')
    # 石畳
    ground_tiles(cv, TOWN_H - GROUND_H, ('K', 'k', 'g', 'W'), tw=6, seed=51)

    if night:
        for y in range(cv.h):
            for x in range(cv.w):
                cv.g[y][x] = NIGHT_COLORS.get(cv.g[y][x], cv.g[y][x])
        # 地平線に近い空は少し明るい（市松模様で混ぜる）
        for y in range(26, 40):
            for x in range(cv.w):
                if cv.g[y][x] != 'N':
                    continue
                if (y >= 32 and (x + y) % 2 == 0) or (y < 32 and x % 4 == (y * 2) % 4 and y % 2 == 0):
                    cv.g[y][x] = 'n'
        # 星（月のまわりはあける）
        mx, my = 112, 4
        for y in range(0, 26):
            for x in range(cv.w):
                v = rand(x, y, 77)
                near_moon = mx - 3 <= x <= mx + 9 and y <= my + 9
                if cv.g[y][x] == 'N' and v > 0.985 and not near_moon:
                    cv.px(x, y, 'W' if v > 0.994 else 'I')
        # 三日月
        moon = [
            '..lll..',
            '.lYY...',
            'lYY....',
            'lYY....',
            'lYY....',
            '.lYY...',
            '..lll..',
        ]
        for yy, row in enumerate(moon):
            for xx, ch in enumerate(row):
                if ch != '.':
                    cv.px(mx + xx, my + yy, ch)
        # 窓に明かりをともして描き直す
        for hx, hw, hh, roof, wall in houses:
            light_house(cv, hx, hw, hh)

    # お店の看板（夜も明るい）
    sx, sy = 57, TOWN_H - GROUND_H - 12
    cv.rect(sx, sy, 16, 4, 'K')
    cv.rect(sx + 1, sy + 1, 14, 2, 'y')
    for i in range(0, 14, 3):
        cv.px(sx + 2 + i, sy + 2, 'r')

    if night:
        # 街灯（お店の左右）と足もとの明かり
        base = TOWN_H - GROUND_H
        for lx in (48 + 2, 82 - 1):
            cv.vline(lx, base - 13, base - 1, 'K')
            cv.rect(lx - 1, base - 16, 3, 3, 'K')
            cv.px(lx, base - 15, 'Y')
            cv.px(lx - 1, base - 14, 'y')
            cv.px(lx + 1, base - 14, 'y')
            for gx in range(lx - 3, lx + 4):
                if cv.get(gx, base) is not None and abs(gx - lx) <= 3:
                    cv.px(gx, base, 'A' if abs(gx - lx) <= 1 else 'i')
    return cv.g
