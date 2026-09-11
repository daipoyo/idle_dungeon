# -*- coding: utf-8 -*-
"""お店の女主人のバストアップ（96x104、透過PNG）。

金髪ロングのウェーブヘア、切れ長の目、赤い唇、オフショルダーの赤いドレス。
図形でシルエットを作り、目や唇などは1ピクセル単位のマップで描き込む。
影はディザを使わずベタ塗りにして、小さな画面でもくっきり見えるようにしている。
"""
import math
from pebble_art import new_image, put, get, outline

W, H = 96, 104
CX = 48

# 肌・髪・ドレスの色（Pebble 64色）
SKIN = 'e'      # Melon
SKIN_HI = 'l'   # PastelYellow
SKIN_SH = 's'   # Rajah
SKIN_DK = 'v'   # RoseVale
HAIR = 'y'      # ChromeYellow
HAIR_HI = 'Y'   # Icterine
HAIR_SP = 'l'   # PastelYellow
HAIR_SH = 'o'   # WindsorTan
HAIR_DK = 'b'   # BulgarianRose
DRESS = 'r'     # DarkCandyAppleRed
DRESS_HI = 'R'  # Red
DRESS_DK = 'b'  # BulgarianRose


class Pic:
    def __init__(self):
        self.img = new_image(W, H)

    def px(self, x, y, c):
        put(self.img, int(x), int(y), c)

    def is_set(self, x, y):
        return get(self.img, int(x), int(y))[3] != 0

    def blit(self, ox, oy, rows, mirror=False):
        for y, row in enumerate(rows):
            if mirror:
                row = row[::-1]
            for x, ch in enumerate(row):
                if ch != '.':
                    self.px(ox + x, oy + y, ch)


# ------------------------------------------------------------
# 形状
# ------------------------------------------------------------
FACE_TOP = 13
FACE_BOTTOM = 46


def face_hw(y):
    """顔の半幅。卵形で、あごは細め。"""
    if y < FACE_TOP or y > FACE_BOTTOM:
        return -1
    if y <= 30:
        t = (30 - y) / 18.0
        return 12.0 * math.sqrt(max(0.0, 1 - t * t))
    t = (y - 30) / 16.0
    return max(1.0, 12.0 * (1 - t ** 1.8))


def torso_hw(y):
    if y < 82:
        return 19.5
    return max(14.0, 19.5 - (y - 82) * 0.4)


def arm_outer(y):
    return 28.0 - max(0, y - 62) * 0.05


def neckline(dx):
    """ドレスの胸元のライン（ハート型）。dxは中心からの距離。"""
    a = abs(dx)
    if a < 9:
        return 69 + 8 * (1 - a / 9.0) ** 2
    return 69 - (a - 9) * 0.3


def shoulder_top(dx):
    a = abs(dx)
    return 50 + (max(0.0, a - 5) / 24.0) ** 2 * 8


BUST = [(38.5, 78.0), (57.5, 78.0)]
BUST_R = 10.5


def in_bust(x, y):
    for (bx, by) in BUST:
        if math.hypot(x - bx, y - by) <= BUST_R:
            return (bx, by)
    return None


# ------------------------------------------------------------
# レイヤー
# ------------------------------------------------------------
def back_hair(p):
    for y in range(3, H):
        if y < 27:
            r = 22.5
            hw = math.sqrt(max(0.0, r * r - (27 - y) ** 2))
        else:
            hw = 22.5 + (y - 27) * 0.14 + 2.0 * math.sin(y * 0.3)
        for x in range(W):
            dx = x - CX
            if abs(dx) > hw:
                continue
            if y > 90 + 6 * math.sin(dx * 0.45) + (abs(dx) / 38.0) * 6:
                continue
            # 毛束：ゆるくうねる縦の筋
            s = math.sin((dx + 3 * math.sin(y * 0.16)) * 0.8)
            c = HAIR
            if s > 0.8:
                c = HAIR_SH
            elif s < -0.9:
                c = HAIR_HI
            if abs(dx) > hw - 2:
                c = HAIR_SH
            # 首の後ろは奥なので暗く
            if abs(dx) < 15 and y > 38:
                c = HAIR_SH if abs(dx) > 11 else HAIR_DK
            p.px(x, y, c)


def body(p):
    # 首
    for y in range(38, 58):
        for x in range(CX - 5, CX + 6):
            p.px(x, y, SKIN_SH if x >= CX + 3 else SKIN)
    # あごの下の影
    for y in range(44, 48):
        hw = 5 - (y - 44) * 0.5
        for x in range(int(round(CX - hw)), int(round(CX + hw)) + 1):
            p.px(x, y, SKIN_SH)
    # 肩・腕・上半身
    for y in range(48, H):
        for x in range(W):
            dx = x - CX
            a = abs(dx)
            if y < shoulder_top(dx):
                continue
            ao = arm_outer(y)
            if a > ao:
                continue
            thw = torso_hw(y)
            c = SKIN
            if a > thw:
                # 腕：胴との境目に影、外側に影
                if y > 62 and a < thw + 1.5:
                    c = SKIN_DK
                elif dx > 0 and a > ao - 3:
                    c = SKIN_SH
                elif dx < 0 and a > ao - 1.5:
                    c = SKIN_SH
            elif dx > 12:
                c = SKIN_SH
            p.px(x, y, c)
    # 肩のハイライト
    for dx in range(-26, -8):
        p.px(CX + dx, int(shoulder_top(dx)) + 1, SKIN_HI)
    for dx in range(9, 22):
        p.px(CX + dx, int(shoulder_top(dx)) + 1, SKIN_HI)
    # 鎖骨
    for i in range(7):
        p.px(CX - 4 - i, 56 + i // 3, SKIN_SH)
        p.px(CX + 4 + i, 56 + i // 3, SKIN_SH)
    # 胸の丸み（ネックラインより上の肌）
    for (bx, by) in BUST:
        for ang in range(180, 361, 3):
            t = math.radians(ang)
            x = round(bx + math.cos(t) * BUST_R)
            y = round(by + math.sin(t) * BUST_R)
            inner = (x - CX) * (bx - CX) <= 0 or abs(x - CX) < 5
            if y < neckline(x - CX) - 0.5 and inner:
                p.px(x, y, SKIN_SH)
        # ハイライト
        hx = bx - 2
        hy = by - BUST_R + 3
        for (ox, oy) in [(0, 0), (1, 0), (-1, 1), (0, 1)]:
            if hy + oy < neckline(hx + ox - CX) - 1:
                p.px(hx + ox, hy + oy, SKIN_HI)
    # 谷間
    for y in range(68, 78):
        if y < neckline(0) - 0.5:
            p.px(CX, y, SKIN_DK)
            if y > 72:
                p.px(CX - 1, y, SKIN_SH)
                p.px(CX + 1, y, SKIN_SH)


def dress(p):
    for y in range(58, H):
        for x in range(W):
            dx = x - CX
            thw = torso_hw(y)
            if abs(dx) > thw or y < neckline(dx):
                continue
            b = in_bust(x, y)
            c = DRESS
            if b:
                bx, by = b
                t = (x - bx) + (y - by)
                if t < -BUST_R * 0.45 and (x - bx) < 3:
                    c = DRESS_HI
                elif t > BUST_R * 0.85:
                    c = DRESS_DK
            else:
                if y < 88:
                    c = DRESS_DK          # 胸の下の影
                elif abs(dx) > thw - 2:
                    c = DRESS_DK
                elif abs(dx) in (9, 10) and y > 90:
                    c = DRESS_DK          # コルセットのボーン
                elif dx < -4 and y > 90:
                    c = DRESS_HI if abs(dx) in (6, 7) else DRESS
            p.px(x, y, c)
    # 黒レースの縁取り
    for x in range(W):
        dx = x - CX
        if abs(dx) > torso_hw(70):
            continue
        y = int(round(neckline(dx)))
        p.px(x, y, 'K')
        if x % 2 == 0:
            p.px(x, y + 1, 'k')
    # コルセットの編み上げ
    for y in range(86, H):
        p.px(CX - 2, y, 'K')
        p.px(CX + 2, y, 'K')
        p.px(CX - 1, y, DRESS_DK)
        p.px(CX, y, DRESS_DK)
        p.px(CX + 1, y, DRESS_DK)
        if y % 4 == 2:
            p.px(CX - 1, y, 'y')
            p.px(CX + 1, y, 'y')
            p.px(CX, y + 1, 'Y')
    # 胸元のリボン
    p.blit(CX - 3, 76, [
        "yy.yy",
        "yYyYy",
        ".yYy.",
        ".y.y.",
    ])
    # 腕輪
    for y in (84, 85):
        for x in range(W):
            a = abs(x - CX)
            if torso_hw(y) + 1.5 < a <= arm_outer(y) and p.is_set(x, y):
                p.px(x, y, 'Y' if y == 84 else 'y')


def face(p):
    for y in range(FACE_TOP, FACE_BOTTOM + 1):
        hw = face_hw(y)
        if hw < 0:
            continue
        for x in range(int(math.ceil(CX - hw)), int(math.floor(CX + hw)) + 1):
            dx = x - CX
            c = SKIN
            if dx > hw - 2.5 and y > 26:
                c = SKIN_SH
            p.px(x, y, c)


EYE = [
    "KK.......",
    ".KKKKKKK.",
    "..WdKKdW.",
    "..WGWKGW.",
    "...GGGG..",
    "....vv...",
]

LIPS = [
    "..R.R..",
    ".rRRRr.",
    "rRRRRRr",
    ".rSSRr.",
    "..rrr..",
]


def face_details(p):
    ey = 27
    # 眉（細く弧を描く）
    p.blit(CX - 10, ey - 4, ["..ooo..", ".o...o."])
    p.blit(CX + 3, ey - 4, ["..ooo..", ".o...o."])
    # アイシャドウ
    for x in range(CX - 8, CX - 2):
        p.px(x, ey - 1, 'q')
    for x in range(CX + 3, CX + 9):
        p.px(x, ey - 1, 'q')
    # 目（伏し目がちで長いまつ毛、目尻をはね上げる）
    p.blit(CX - 10, ey, EYE)
    p.blit(CX + 2, ey, EYE, mirror=True)
    # 鼻
    p.px(CX + 1, ey + 8, SKIN_SH)
    p.px(CX + 1, ey + 9, SKIN_SH)
    p.px(CX, ey + 10, SKIN_DK)
    # 唇
    p.blit(CX - 3, ey + 12, LIPS)
    # ほくろ
    p.px(CX + 6, ey + 15, SKIN_DK)


def front_hair(p):
    # 頭頂部と前髪（向かって右で分けて、左へ流す）
    part = 5
    for y in range(3, 28):
        r = 23.0
        hw = math.sqrt(max(0.0, r * r - (27 - y) ** 2))
        for x in range(W):
            dx = x - CX
            if abs(dx) > hw:
                continue
            if dx <= part:
                # 流した前髪の下端：左へ行くほど下がる
                fringe = 16 + (part - dx) * 0.42 + 1.2 * math.sin(dx * 0.8)
            else:
                fringe = 15 + (dx - part) * 0.9
            if y > fringe and abs(dx) < 13:
                continue
            s = math.sin((dx - part) * 0.7 - y * 0.25)
            c = HAIR
            if s > 0.82:
                c = HAIR_SH
            # 天使の輪
            if 8 <= y <= 9 and -15 < dx < 12:
                c = HAIR_HI
            if y == 8 and -11 < dx < -3:
                c = HAIR_SP
            p.px(x, y, c)
    # 前髪の下端に影
    for x in range(CX - 12, CX + part + 1):
        dx = x - CX
        fringe = 16 + (part - dx) * 0.42 + 1.2 * math.sin(dx * 0.8)
        p.px(x, int(fringe), HAIR_SH)
    # 顔の横に垂れる髪（左右）
    for side in (-1, 1):
        for y in range(16, 60):
            if y < 30:
                inner = 10.5
            elif y < FACE_BOTTOM:
                inner = max(7.0, face_hw(y) + 0.5)
            else:
                inner = 7.0 + (y - FACE_BOTTOM) * 0.4
            width = 4 + (y - 16) * 0.05
            wave = 1.3 * math.sin(y * 0.28 + (0 if side < 0 else 2))
            x0 = inner + wave
            for k in range(int(width + 0.5)):
                x = CX + side * (x0 + k)
                c = HAIR
                if k == 0 or k >= width - 1.5:
                    c = HAIR_SH
                if side < 0 and k == 1 and y % 8 < 3:
                    c = HAIR_HI
                p.px(x, y, c)
    # 肩から胸へ流れる一房（向かって左）
    for y in range(50, 94):
        t = (y - 50) / 44.0
        cx = CX - 23 + 7 * math.sin(t * 2.4) + t * 4
        w = 5.0 - t * 2.2
        for k in range(int(-w), int(w) + 1):
            x = int(cx + k)
            c = HAIR
            if k <= -w + 1:
                c = HAIR_SH
            elif k >= w - 1.5:
                c = HAIR_SH
            elif k == -1 and (y // 3) % 2 == 0:
                c = HAIR_HI
            p.px(x, y, c)
    p.blit(CX - 22, 92, ["yyo.", ".yyo", "..oy", "...o"])


def accessories(p):
    # チョーカー
    for x in range(CX - 5, CX + 6):
        p.px(x, 49, 'K')
        p.px(x, 50, 'k' if x % 2 else 'K')
    p.blit(CX - 1, 51, ["yRy", ".R.", ".y."])
    # イヤリング
    for ex in (CX - 13, CX + 13):
        p.blit(ex - 1, 35, [".y.", "yYy", ".R.", ".R.", ".y."])
    # 髪に挿した薔薇
    p.blit(CX + 9, 5, [
        "..RRR..",
        ".RRrRR.",
        "RRrRrRR",
        "RrRRRrR",
        ".RRrRR.",
        "..RRR.G",
        "....GG.",
    ])


def make():
    p = Pic()
    back_hair(p)
    body(p)
    dress(p)
    face(p)
    face_details(p)
    front_hair(p)
    accessories(p)
    outline(p.img, 'K')
    return p.img
