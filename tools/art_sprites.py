# -*- coding: utf-8 -*-
"""小さなスプライト（アイテムアイコン・敵・勇者・演出）のドット絵定義。

マップは「塗り」だけを描き、輪郭(黒)は outline() で自動生成する。
"""
import math
from pebble_art import (new_image, put, rect, sprite_from_map, mirror_half,
                        outline, blit_map)

# ============================================================
# アイテムアイコン (16x16)
# ============================================================
SWORD = [
    "................",
    ".............Wg.",
    "............Wgk.",
    "...........Wgk..",
    "..........Wgk...",
    ".........Wgk....",
    "........Wgk.....",
    "...yy..Wgk......",
    "....yyWgk.......",
    ".....yY.........",
    "....ooyy........",
    "...oo..yy.......",
    "..oo............",
    ".YY.............",
    ".YY.............",
    "................",
]

AXE_HALF = [
    "........",
    ".......y",
    "..WW...o",
    ".WWgg.ko",
    ".Wgggkko",
    ".Wgggkko",
    ".Wgggkko",
    ".WWgg.ko",
    "..WW...o",
    ".......o",
    ".......b",
    ".......o",
    ".......b",
    ".......o",
    "......yy",
    "........",
]

ARMOR_HALF = [
    "........",
    "........",
    "...344..",
    "..33114.",
    ".3311111",
    ".3111111",
    ".3111111",
    ".2311111",
    "..311111",
    "..311111",
    "..555555",
    "..311111",
    "..311111",
    "..311111",
    "..222222",
    "........",
]

PLATE_HALF = [
    "........",
    "........",
    ".3344...",
    "33111444",
    "31111111",
    "31111111",
    "2311y111",
    ".2311111",
    "..311111",
    "..311y11",
    "..yyyyyy",
    "..311111",
    "..311111",
    "..311111",
    "..222222",
    "........",
]


def icon_sword(remap):
    return sprite_from_map(SWORD, remap)


def icon_axe():
    rows = mirror_half(AXE_HALF, {'W': 'g', 'g': 'g', 'k': 'k'})
    return sprite_from_map(rows)


def icon_armor(base, remap, chain=False):
    rows = mirror_half(base, {'3': '2'})
    img = sprite_from_map(rows, remap)
    if chain:
        # 鎖かたびらの編み目
        g = remap['1']
        for y in range(16):
            for x in range(16):
                px = img.getpixel((x, y))
                if px[3] and px[:3] == (0xAA, 0xAA, 0xAA) and (x + y * 2) % 3 == 0:
                    put(img, x, y, 'k')
    return img


def icon_coin():
    img = new_image(16, 16)
    cx, cy = 7.5, 7.5
    for y in range(16):
        for x in range(16):
            d = math.hypot(x - cx, y - cy)
            if d <= 6.2:
                c = 'Y'
                if d > 5.0:
                    c = 'y' if (x - cx) + (y - cy) > -1 else 'Y'
                elif (x - cx) + (y - cy) > 3:
                    c = 'y'
                put(img, x, y, c)
    # 四つ葉の刻印
    for (x, y) in [(7, 5), (8, 5), (6, 6), (9, 6), (6, 8), (9, 8), (7, 9), (8, 9)]:
        put(img, x, y, 'G')
    for (x, y) in [(7, 6), (8, 6), (7, 8), (8, 8), (7, 7), (8, 7), (6, 7), (9, 7)]:
        put(img, x, y, 'L')
    put(img, 4, 4, 'W'); put(img, 5, 3, 'W'); put(img, 4, 5, 'l')
    return outline(img)


def icon_ring():
    img = new_image(16, 16)
    cx, cy = 7.5, 9.0
    for y in range(16):
        for x in range(16):
            d = math.hypot((x - cx) * 1.0, (y - cy) * 1.15)
            if 3.2 <= d <= 5.6:
                c = 'Y' if (x - cx) < 0 and (y - cy) < 1 else 'y'
                if (x - cx) > 1 and (y - cy) > 0:
                    c = 'o'
                put(img, x, y, c)
    # 宝石
    gem = [
        "..RR..",
        ".RSRRr",
        "RSWRRr",
        ".RRrr.",
        "..rr..",
    ]
    blit_map(img, 5, 1, gem)
    return outline(img)


def icon_charm():
    img = new_image(16, 16)
    # 鎖
    for i in range(6):
        put(img, 3 + i, 1 + i, 'g' if i % 2 else 'W')
        put(img, 12 - i, 1 + i, 'g' if i % 2 else 'k')
    half = [
        "....yy..",
        "...yuuu.",
        "..yuWuuu",
        "..yuuBBB",
        "..yuBBBB",
        "...yBBBN",
        "....yBNN",
        ".....yNN",
        "......yy",
    ]
    rows = mirror_half(half, {'u': 'B', 'W': 'u', 'B': 'N'})
    blit_map(img, 0, 6, rows)
    return outline(img)


def icon_relic():
    img = new_image(16, 16)
    cx, cy = 7.5, 6.5
    for y in range(16):
        for x in range(16):
            d = math.hypot(x - cx, y - cy)
            if d <= 5.0:
                t = (x - cx) + (y - cy)
                c = 'x' if t < -2 else ('p' if t < 3 else 'P')
                put(img, x, y, c)
    put(img, 5, 4, 'W'); put(img, 6, 4, 'H'); put(img, 5, 5, 'H')
    put(img, 8, 6, 'm'); put(img, 7, 7, 'm'); put(img, 9, 7, 'm'); put(img, 8, 8, 'm')
    base = [
        "..y..........y..",
        "..yy........yy..",
        "...yyyyyyyyyy...",
        "....yYYYYYYo....",
        "...yYYyyyyyoo...",
        "...oooooooooo...",
    ]
    blit_map(img, 0, 9, base)
    return outline(img)


def icon_herb():
    rows = [
        "................",
        ".........LL.....",
        "........LjGL....",
        "...LL...LjGd....",
        "..LjGL..LGGd....",
        "..LjjGd..Gd.....",
        "...GGGdd.d..LL..",
        "....dd.d.d.LjGL.",
        ".......dd.dLjGd.",
        "........d.dGGd..",
        "........ddd.d...",
        ".........d......",
        ".........d......",
        "........dd......",
        "................",
        "................",
    ]
    return sprite_from_map(rows)


def icon_fang():
    rows = [
        "................",
        "................",
        "....ggWWWWWW....",
        "...gWWWWWWWWg...",
        "...gWWWWWWWWg...",
        "....gWWWWWWlg...",
        ".....gWWWWWlg...",
        "......gWWWlg....",
        "......gWWWlg....",
        ".......gWWl.....",
        ".......gWlg.....",
        "........Wlg.....",
        "........Wl......",
        "........l.......",
        "................",
        "................",
    ]
    return sprite_from_map(rows)


def icon_gem():
    rows = [
        "................",
        "......W.........",
        ".....WcC........",
        "....WcCCT.......",
        "....WcCCT..W....",
        "...WccCCTT.cC...",
        "...WcCCCTT.cT...",
        "..WccCCCCTTcT...",
        "..WcCCCCCTT.....",
        "..cCCCCCCTTT....",
        "...cCCCCCTTt....",
        "....cCCCTTt.....",
        ".....cCTTt......",
        "......Ttt.......",
        "................",
        "................",
    ]
    return sprite_from_map(rows)


def icon_scale():
    rows = [
        "................",
        "......SSSS......",
        "....SSRRRRSS....",
        "...SRRRRRRRRr...",
        "..SRRSRRRRRRrr..",
        "..SRSRRRRRRRrr..",
        "..SRRRRrRRRRrr..",
        "..RRRRrRrRRrrr..",
        "...RRRrRRrRrr...",
        "...RRrRRRRrrr...",
        "....RRRRRrrr....",
        ".....RRrrrr.....",
        "......rrrr......",
        ".......rr.......",
        "................",
        "................",
    ]
    return sprite_from_map(rows, {'S': 'S'})


def make_item_icons():
    """カタログ順(main.cのg_items)に16個のアイコンを返す。"""
    icons = [
        icon_sword({'W': 'g', 'g': 'v', 'k': 'o', 'y': 'o', 'Y': 's', 'o': 'b'}),  # Rusty Sword
        icon_sword({'y': 'g', 'Y': 'W'}),                                         # Iron Sword
        icon_sword({'g': 'C', 'k': 'u', 'o': 'B'}),                                # Steel Blade
        icon_axe(),                                                                # Battle Axe
        icon_armor(ARMOR_HALF, {'1': 's', '2': 'o', '3': 'l', '4': 'l', '5': 'o'}),  # Cloth Armor
        icon_armor(ARMOR_HALF, {'1': 'o', '2': 'b', '3': 's', '4': 's', '5': 'y'}),  # Leather Armor
        icon_armor(ARMOR_HALF, {'1': 'g', '2': 'k', '3': 'W', '4': 'W', '5': 'o'}, chain=True),  # Chain Mail
        icon_armor(PLATE_HALF, {'1': 'I', '2': 'i', '3': 'W', '4': 'W'}),         # Plate Armor
        icon_coin(),    # Lucky Coin
        icon_ring(),    # Power Ring
        icon_charm(),   # Guard Charm
        icon_relic(),   # Ancient Relic
        icon_herb(),    # Healing Herb
        icon_fang(),    # Monster Fang
        icon_gem(),     # Gem Shard
        icon_scale(),   # Dragon Scale
    ]
    return icons


# ============================================================
# 敵スプライト (24x24, 左向き, 2フレーム)
# ============================================================
SLIME_A = [
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "...........GG...........",
    ".........GGGGGG.........",
    "........GLLGGGGGG.......",
    ".......GLLGGGGGGGG......",
    "......GGLGGGGGGGGGG.....",
    ".....GGGGGGGGGGGGGGG....",
    ".....GGGGGGGGGGGGGGGd...",
    "....GGWKGGGWKGGGGGGGd...",
    "....GGKKGGGKKGGGGGGGdd..",
    "...GGGGGGGGGGGGGGGGGGd..",
    "...GGGGRRRGGGGGGGGGGdd..",
    "..GGGGGGGGGGGGGGGGGGdd..",
    "..GGGGGGGGGGGGGGGGGdddd.",
    "..GGGGGGGGGGGGGGGGGdddd.",
    "..dGGGGGGGGGGGGGGGddddd.",
    "...dddddddddddddddddd...",
    "........................",
]

SLIME_B = [
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    "...........GGG..........",
    "........GGGGGGGGG.......",
    "......GLLLGGGGGGGGG.....",
    ".....GGLLGGGGGGGGGGG....",
    "....GGGLGGGGGGGGGGGGGd..",
    "...GGGWKGGGWKGGGGGGGGd..",
    "...GGGKKGGGKKGGGGGGGGdd.",
    "..GGGGGGGGGGGGGGGGGGGGd.",
    "..GGGGGRRRGGGGGGGGGGGdd.",
    ".GGGGGGGGGGGGGGGGGGGGdd.",
    ".GGGGGGGGGGGGGGGGGGGddd.",
    ".GGGGGGGGGGGGGGGGGGdddd.",
    ".dGGGGGGGGGGGGGGGGddddd.",
    "..ddddddddddddddddddd...",
    "........................",
]

GOBLIN_A = [
    "........................",
    "........................",
    "........................",
    "........................",
    ".........EEEEE..........",
    "..E.....EEEEEEE.....E...",
    "..EE...EjjEEEEEE...EE...",
    "...EEEEjEEEEEEEEEEEE....",
    "....EERYEEERYEEEEEd.....",
    "....EEEEEEEEEEEEEEd.....",
    ".....EEWdWdWEEEEdd......",
    "......EEEEEEEEEdd.......",
    "........oooooo..........",
    "......oooooooooo........",
    "..oo.Eoooooooooood......",
    "..ooEE.ooooyoooo.dd.....",
    "..ooE..oooooooooo.dd....",
    "..oo...oooooooooo.......",
    "..ko....bbb..bbb........",
    "..ko....EE....EE........",
    "..kk....EE....EE........",
    ".......EEE....EEE.......",
    "......bbbb...bbbb.......",
    "........................",
]

GOBLIN_B = [
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    ".........EEEEE..........",
    "..E.....EEEEEEE.....E...",
    "..EE...EjjEEEEEE...EE...",
    "...EEEEjEEEEEEEEEEEE....",
    "....EERYEEERYEEEEEd.....",
    "....EEEEEEEEEEEEEEd.....",
    ".....EEWdWdWEEEEdd......",
    "..oo..EEEEEEEEEdd.......",
    "..oo....oooooo..........",
    "..ooE.oooooooooo........",
    "..koEEoooooooooood......",
    "..ko...ooooyoooo.dd.....",
    "..kk...oooooooooo.dd....",
    ".......oooooooooo.......",
    "........bbb..bbb........",
    ".......EE......EE.......",
    "......EEE......EEE......",
    ".....bbbb......bbbb.....",
    "........................",
]

SKELETON_A = [
    "........................",
    "........................",
    ".........WWWWW..........",
    "........WWWWWWWg........",
    ".......WWWWWWWWWg.......",
    ".......WKKWWKKWWg.......",
    ".......WKKWWKKWWg.......",
    "........WWWKWWWg........",
    ".........WgWgWg.........",
    "..........WWWW..........",
    ".........g.WW.g.........",
    "..g.....gWWWWWWg........",
    "..g....W.WgWWgW.W.......",
    "..g...W..WWWWWW..W......",
    "..g..W....WgWg....W.....",
    ".yyyW.....WWWW.....g....",
    "..o.......gggg..........",
    "..........W..W..........",
    ".........W....W.........",
    ".........W....W.........",
    ".........W....W.........",
    "........WW....WW........",
    ".......ggg....ggg.......",
    "........................",
]

SKELETON_B = [
    "........................",
    "........................",
    "........................",
    ".........WWWWW..........",
    "........WWWWWWWg........",
    ".......WWWWWWWWWg.......",
    ".......WKKWWKKWWg.......",
    ".......WKKWWKKWWg.......",
    "........WWWKWWWg........",
    ".........WgWgWg.........",
    "..........WWWW..........",
    ".........g.WW.g.........",
    "gggg....gWWWWWWg........",
    "...yW..W.WgWWgW.W.......",
    "...yoW...WWWWWW..W......",
    "..........WgWg....W.....",
    "..........WWWW.....g....",
    "..........gggg..........",
    ".........W....W.........",
    "........W......W........",
    "........W......W........",
    ".......WW......WW.......",
    "......ggg......ggg......",
    "........................",
]

DRAGON_A = [
    "........................",
    "........................",
    "..............bb........",
    ".............brrb.......",
    "....ll.......brrrb......",
    ".....l......brrrrrb.....",
    "....RRRl....brrrrrrb....",
    "...RRRRRR...brrrrrrrb...",
    "..RYKRRRRR..brrrrrrrb...",
    ".RRRRRRRRRR..brrrrrb....",
    "RRRRRRRRRRRR..bbrrb.....",
    "WRWRRRRRRRRRRRRRbb......",
    ".RRRRRssRRRRRRRRRR......",
    "..RRRsssRRRRRRRRRRRr....",
    "......ssssRRRRRRRRRrr...",
    ".......sssssRRRRRRRr.r..",
    ".......sssssRRRRRRRr..r.",
    "........ssssRRRRRRr...r.",
    ".........RRR...RRR...rr.",
    ".........RR....RR..rrr..",
    "........yRR...yRR.......",
    "........................",
    "........................",
    "........................",
]

DRAGON_B = [
    "........................",
    "........................",
    "........................",
    "........................",
    "....ll..................",
    ".....l..................",
    "....RRRl.....bbbbbb.....",
    "...RRRRRR...brrrrrrbb...",
    "..RYKRRRRR.brrrrrrrrrb..",
    ".RRRRRRRRRRbrrrrrrrrrrb.",
    "RRRRRRRRRRRRbbbrrrrbbb..",
    "WRWRRRRRRRRRRRRRbbb.....",
    ".RRRRRssRRRRRRRRRR......",
    "..RRRsssRRRRRRRRRRRr....",
    "......ssssRRRRRRRRRrr...",
    ".......sssssRRRRRRRr.r..",
    ".......sssssRRRRRRRr..r.",
    "........ssssRRRRRRr...r.",
    ".........RRR...RRR...rr.",
    ".........RR....RR..rrr..",
    "........yRR...yRR.......",
    "........................",
    "........................",
    "........................",
]

JELLY_A = [
    "........................",
    "........................",
    "........................",
    ".........IIIIII.........",
    ".......IIWWIIIIII.......",
    "......IIWWIIIIIIIu......",
    ".....IIWIIIIIIIIIIu.....",
    ".....IIIIIIIIIIIIIu.....",
    "....IIINNIIIINNIIIuu....",
    "....IIINNIIIINNIIIuu....",
    "....IIIIIIIIIIIIIIuu....",
    "....IIIIIImmIIIIIIuu....",
    "....uuuuuuuuuuuuuuuu....",
    "....x.xx..xx..xx..x.....",
    ".....x.x..x..x.x..x.....",
    ".....x..x.x..x..x.x.....",
    "....x...x..x.x..x..x....",
    "....x..x...x..x..x.x....",
    ".....x.x..x...x..x..x...",
    ".....x..x.x..x...x..x...",
    "......x.x..x.x..x..x....",
    "........................",
    "........................",
    "........................",
]

JELLY_B = [
    "........................",
    "........................",
    "........................",
    "........................",
    "........................",
    ".........IIIIII.........",
    ".......IIWWIIIIII.......",
    "......IIWWIIIIIIIu......",
    ".....IIWIIIIIIIIIIu.....",
    "....IIIIIIIIIIIIIIIu....",
    "...IIIINNIIIINNIIIIuu...",
    "...IIIINNIIIINNIIIIuu...",
    "...IIIIIIIImmIIIIIIuu...",
    "...uuuuuuuuuuuuuuuuuu...",
    "....x..xx..xx..xx..x....",
    "....x..x..x..x..x..x....",
    ".....x.x..x..x..x.x.....",
    ".....x..x..x.x.x..x.....",
    "....x...x..x..x...x.....",
    "....x..x..x...x..x......",
    "........................",
    "........................",
    "........................",
    "........................",
]

EYE_A = [
    "........................",
    "........................",
    "..P..................P..",
    "..PP................PP..",
    "..PpP....pppppp....PpP..",
    "...PpP.ppppppppppPpP....",
    "...PppppWWWWWWWWppppP...",
    "....PppWWWWWWWWWWWppP...",
    ".....pWWWWRRRRWWWWWp....",
    ".....pWWWRRRRRRWWWWp....",
    "....ppWWRRRKKRRRWWWpp...",
    "....ppWWRRKKKKRRWWWpp...",
    "....ppWWRRKKKKRRWWWpp...",
    "....ppWWWRRKKRRWWWWpp...",
    ".....pWWWWRRRRWWWWWp....",
    ".....ppWWWWWWWWWWWpp....",
    "......ppWWWWWWWWWpp.....",
    ".......pppppppppppP.....",
    "........P.P.PP.P.P......",
    ".......P..P.P..P..P.....",
    "......P..P...P..P..P....",
    ".......P..P.P..P..P.....",
    "........................",
    "........................",
]

EYE_B = [
    "........................",
    "........................",
    "........................",
    "........................",
    ".........pppppp.........",
    ".P.....ppppppppppp....P.",
    ".PP...ppWWWWWWWWpp...PP.",
    "..PPPppWWWWWWWWWWWpPPP..",
    "....ppWWWWRRRRWWWWWpp...",
    ".....pWWWRRRRRRWWWWp....",
    "....ppWWRRRKKRRRWWWpp...",
    "....ppWWRRKKKKRRWWWpp...",
    "....ppWWRRKKKKRRWWWpp...",
    "....ppWWWRRKKRRWWWWpp...",
    ".....pWWWWRRRRWWWWWp....",
    ".....ppWWWWWWWWWWWpp....",
    "......ppWWWWWWWWWpp.....",
    ".......pppppppppppP.....",
    ".......P..P.PP.P..P.....",
    "........P..P..P..P......",
    ".......P..P....P..P.....",
    "........P..P..P..P......",
    "........................",
    "........................",
]

ENEMY_MAPS = [
    (SLIME_A, SLIME_B),
    (GOBLIN_A, GOBLIN_B),
    (SKELETON_A, SKELETON_B),
    (DRAGON_A, DRAGON_B),
    (JELLY_A, JELLY_B),
    (EYE_A, EYE_B),
]


def make_enemies():
    out = []
    for a, b in ENEMY_MAPS:
        out.append((sprite_from_map(a), sprite_from_map(b)))
    return out


# ============================================================
# 宝箱など演出用 (16x16)
# ============================================================
CHEST_CLOSED = [
    "................",
    "................",
    "................",
    "................",
    "...oooooooooo...",
    "..osssssssssso..",
    "..oyyyyyyyyyyo..",
    "..osssssssssso..",
    "..bbbbbyybbbbb..",
    "..oooooYYooooo..",
    "..osssssysssso..",
    "..osssssssssso..",
    "..oyyyyyyyyyyo..",
    "..bbbbbbbbbbbb..",
    "................",
    "................",
]

CHEST_OPEN = [
    "................",
    "...oooooooooo...",
    "..osssssssssso..",
    "..oyyyyyyyyyyo..",
    "..obbbbbbbbbbo..",
    "..bKKKKKKKKKKb..",
    "..bKYYlYYyYKKb..",
    "..bYYYYYlYYYyb..",
    "..bbbbbyybbbbb..",
    "..oooooYYooooo..",
    "..osssssysssso..",
    "..osssssssssso..",
    "..oyyyyyyyyyyo..",
    "..bbbbbbbbbbbb..",
    "................",
    "................",
]


def make_chests():
    fix = lambda rows: [r.replace(' ', 's') for r in rows]
    return sprite_from_map(fix(CHEST_CLOSED)), sprite_from_map(fix(CHEST_OPEN))


# ============================================================
# 勇者 (16x24, 右向き) : C側でパレット差し替えして描くので文字マップのまま出力
#   1/2/3 : 鎧(主/影/ハイライト)  … 装備している防具で色が変わる
#   c/C   : マント
#   h/H   : 髪(主/影)
#   e/v   : 肌(主/影)
#   p     : ズボン   B: ブーツ   y: ベルトの金具
# ============================================================
HERO_HEAD = [
    "................",
    ".....hhhhh......",
    "...hhhhhhhhh....",
    "..hhhhhhhhhhh...",
    "..hhhhhhhhhhhh..",
    ".hhhrrrrrrrrr...",
    ".hHhrreeeeeee...",
    ".hHhheeeeeKee...",
    ".hHhheeeeeKee...",
    "..hhheeeeeeeev..",
    "...hhveeeeeve...",
    "....hhvveeee....",
]

HERO_BODY_IDLE = [
    "...cc311113e....",
    "..ccc3111112e...",
    "..ccc1111112e...",
    "..cc21111112e...",
    "..cc.yyyyyy.....",
    "..cc.2221222....",
]

HERO_LEGS_IDLE = [
    ".....pp..pp.....",
    ".....pp..pp.....",
    ".....pp..pp.....",
    ".....BB..BB.....",
    "....BBB.BBBB....",
    "................",
]

HERO_LEGS_WALK0 = [
    ".....pp..pp.....",
    "....pp....pp....",
    "...pp......pp...",
    "...BB......BB...",
    "..BBB......BBB..",
    "................",
]

HERO_LEGS_WALK1 = [
    "......pppp......",
    "......pp.pp.....",
    ".....pp..pp.....",
    ".....BB..BB.....",
    "....BBB..BBBB...",
    "................",
]

HERO_BODY_ATTACK = [
    "...cc31111eeee..",
    "..ccc311111.....",
    "..ccc1111112....",
    "..cc21111112....",
    "..cc.yyyyyy.....",
    "..cc.2221222....",
]


def hero_frames():
    """(名前, 行リスト) のリスト。輪郭は自動生成して 'K' を含んだ状態で返す。"""
    frames = {
        'IDLE': HERO_HEAD + HERO_BODY_IDLE + HERO_LEGS_IDLE,
        'WALK0': HERO_HEAD + HERO_BODY_IDLE + HERO_LEGS_WALK0,
        'WALK1': HERO_HEAD + HERO_BODY_IDLE + HERO_LEGS_WALK1,
        'ATTACK': HERO_HEAD + HERO_BODY_ATTACK + HERO_LEGS_WALK0,
    }
    out = []
    for name, rows in frames.items():
        rows = [r.ljust(16, '.') for r in rows]
        out.append((name, add_outline_to_map(rows)))
    return out


# 武器（勇者の手に重ねる）。4=刀身 5=刀身ハイライト 6=柄 e=手
WEAPON_REST = [
    ".y......",
    "ey5.....",
    ".y45....",
    "...45...",
    "....45..",
    ".....45.",
    "......45",
    ".......4",
]

WEAPON_SWING = [
    "..y.........",
    "eeyy55555555",
    "..y444444444",
    "..y.........",
]

AXE_REST = [
    "e6.......",
    ".66......",
    "..66.555.",
    "...66455.",
    "....6445.",
    "...55445.",
    "....5555.",
]

AXE_SWING = [
    "..........555",
    "..........545",
    "ee66666666445",
    "..........545",
    "..........555",
]


def add_outline_to_map(rows):
    h = len(rows)
    w = max(len(r) for r in rows)
    grid = [list(r.ljust(w, '.')) for r in rows]
    out = [row[:] for row in grid]
    for y in range(h):
        for x in range(w):
            if grid[y][x] != '.':
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < w and 0 <= yy < h and grid[yy][xx] != '.':
                    out[y][x] = 'K'
                    break
    return [''.join(r) for r in out]


def pad_map(rows, l=1, r=1, t=1, b=1):
    w = max(len(x) for x in rows)
    rows = [x.ljust(w, '.') for x in rows]
    rows = ['.' * (w + l + r)] * t + ['.' * l + x + '.' * r for x in rows] + ['.' * (w + l + r)] * b
    return rows


def weapon_maps():
    return {
        'SWORD_REST': add_outline_to_map(pad_map(WEAPON_REST)),
        'SWORD_SWING': add_outline_to_map(pad_map(WEAPON_SWING)),
        'AXE_REST': add_outline_to_map(pad_map(AXE_REST)),
        'AXE_SWING': add_outline_to_map(pad_map(AXE_SWING)),
    }
