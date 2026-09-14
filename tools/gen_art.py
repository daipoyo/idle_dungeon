# -*- coding: utf-8 -*-
"""Idle Dungeon の画像リソースを生成するスクリプト。

使い方:  python tools/gen_art.py [--preview 出力先ディレクトリ]

出力:
  resources/images/*.png   … Pebbleのビットマップリソース
  src/c/art_info.h         … 画像サイズや背景の塗り色などの定数
  src/c/sprite_data.h      … 勇者・武器の文字マップとパレット表（gfx.c 専用）
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pebble_art import PAL, new_image, save, upscale, on_bg  # noqa: E402
import art_sprites  # noqa: E402
import art_scenes  # noqa: E402
import art_shopkeeper  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG_DIR = os.path.join(ROOT, 'resources', 'images')
C_DIR = os.path.join(ROOT, 'src', 'c')

# 勇者プレビュー用のパレット（C側 gfx.c の hero パレットと揃える）
HERO_BASE = {
    'K': 'K', 'h': 'o', 'H': 'b', 'r': 'r', 'e': 'e', 'v': 'v',
    'c': 'r', 'C': 'b', 'p': 'N', 'B': 'b', 'y': 'y',
}
ARMOR_PAL = [
    {'1': 'B', '2': 'N', '3': 'u'},   # 装備なし（青い服）
    {'1': 's', '2': 'o', '3': 'l'},   # Cloth
    {'1': 'o', '2': 'b', '3': 's'},   # Leather
    {'1': 'g', '2': 'k', '3': 'W'},   # Chain
    {'1': 'I', '2': 'i', '3': 'W'},   # Plate
]
WEAPON_PAL = [
    {'4': 'v', '5': 'g', '6': 'b'},   # Rusty
    {'4': 'g', '5': 'W', '6': 'o'},   # Iron
    {'4': 'C', '5': 'W', '6': 'B'},   # Steel
    {'4': 'g', '5': 'W', '6': 'o'},   # Axe
]

# 背景画像の上に空きができる大きい画面用の塗り色。
# ダンジョンの背景は tools/dungeon_art.py が作った画像を使っているので、
# ここの値もその画像の上端の色に合わせてある（同スクリプトが出力する）。
SCENE_FILL = {
    'bg_dungeon0': 'K', 'bg_dungeon1': 'K', 'bg_dungeon2': 'a',
    'bg_dungeon3': 'K', 'bg_dungeon4': 'K', 'bg_dungeon5': 'K',
    'bg_town': 'u', 'bg_shop': 'K',
}

# これらは外部で用意した画像に置き換えたので、このスクリプトでは書き出さない
EXTERNAL_ART = ('bg_dungeon', 'bg_shop')


def argb8(c):
    r, g, b = PAL[c]
    return 0xC0 | ((r // 85) << 4) | ((g // 85) << 2) | (b // 85)


def render_charmap(rows, pal):
    h = len(rows)
    w = max(len(r) for r in rows)
    img = new_image(w, h)
    for y, r in enumerate(rows):
        for x, ch in enumerate(r):
            if ch == '.':
                continue
            c = pal.get(ch, ch)
            img.putpixel((x, y), PAL[c] + (255,))
    return img


def hero_hand(rows):
    """胴体部分(12〜17行目)で一番右下にある手(e)の座標。武器の取り付け位置に使う。"""
    best = None
    for y in range(12, 18):
        for x, ch in enumerate(rows[y]):
            if ch == 'e' and (best is None or (x, y) > best):
                best = (x, y)
    return best


def weapon_anchor(rows):
    """武器マップ内の握り手(最初の e)の座標。"""
    for y, r in enumerate(rows):
        x = r.find('e')
        if x >= 0:
            return (x, y)
    raise ValueError('weapon map has no hand')


def write_headers(frames, weapons):
    head = [
        '// このファイルは tools/gen_art.py で自動生成されています。直接編集しないでください。',
        '#pragma once',
        '',
    ]
    # ---- art_info.h : 定数のみ（どこからでもインクルード可） ----
    lines = list(head)
    lines.append('// 背景画像より画面が高いときに上側を塗る色 (argb)')
    for i in range(6):
        lines.append('#define BG_DUNGEON%d_FILL 0x%02X' % (i, argb8(SCENE_FILL['bg_dungeon%d' % i])))
    lines.append('#define BG_TOWN_FILL 0x%02X' % argb8(SCENE_FILL['bg_town']))
    lines.append('#define BG_SHOP_FILL 0x%02X' % argb8(SCENE_FILL['bg_shop']))
    lines.append('')
    lines.append('#define BG_TILE_W %d' % art_scenes.TILE_W)
    lines.append('#define BG_FAR_H %d' % art_scenes.FAR_H)
    lines.append('#define BG_GROUND_H %d' % art_scenes.GROUND_H)
    lines.append('#define BG_TOWN_W %d' % art_scenes.TOWN_W)
    lines.append('#define BG_TOWN_H %d' % art_scenes.TOWN_H)
    lines.append('#define SHOP_WALL_H %d' % art_scenes.SHOP_WALL_H)
    lines.append('#define SHOP_COUNTER_H %d' % art_scenes.SHOP_COUNTER_H)
    lines.append('#define KEEPER_W %d' % art_shopkeeper.W)
    lines.append('#define KEEPER_H %d' % art_shopkeeper.H)
    lines.append('')
    lines.append('#define HERO_W 16')
    lines.append('#define HERO_H 24')
    lines.append('')
    with open(os.path.join(C_DIR, 'art_info.h'), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))

    # ---- sprite_data.h : 配列データ（gfx.c だけがインクルードする） ----
    lines = list(head)
    lines.append('#include <stdint.h>')
    lines.append('')
    lines.append('// 文字マップの記号 -> Pebble GColor8 (argb)。0 は透明。')
    lines.append('static const uint8_t PAL_LUT[128] = {')
    lut = [0] * 128
    for ch, c in PAL.items():
        if c is not None:
            lut[ord(ch)] = argb8(ch)
    for i in range(0, 128, 16):
        lines.append('  ' + ', '.join('0x%02X' % v for v in lut[i:i + 16]) + ',')
    lines.append('};')
    lines.append('')
    for name, rows in frames:
        hx, hy = hero_hand(rows)
        lines.append('#define HERO_%s_HAND_X %d' % (name, hx))
        lines.append('#define HERO_%s_HAND_Y %d' % (name, hy))
        lines.append('static const char *const HERO_%s[HERO_H] = {' % name)
        for r in rows:
            lines.append('  "%s",' % r)
        lines.append('};')
        lines.append('')
    for name, rows in weapons.items():
        w = len(rows[0])
        ax, ay = weapon_anchor(rows)
        lines.append('#define WPN_%s_W %d' % (name, w))
        lines.append('#define WPN_%s_H %d' % (name, len(rows)))
        lines.append('#define WPN_%s_AX %d' % (name, ax))
        lines.append('#define WPN_%s_AY %d' % (name, ay))
        lines.append('static const char *const WPN_%s[%d] = {' % (name, len(rows)))
        for r in rows:
            lines.append('  "%s",' % r)
        lines.append('};')
        lines.append('')
    path = os.path.join(C_DIR, 'sprite_data.h')
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))


def sheet(images, cols, cell_w, cell_h):
    rows = (len(images) + cols - 1) // cols
    out = new_image(cols * cell_w, rows * cell_h)
    for i, im in enumerate(images):
        out.alpha_composite(im, ((i % cols) * cell_w, (i // cols) * cell_h))
    return out


def main():
    preview_dir = None
    if '--preview' in sys.argv:
        preview_dir = sys.argv[sys.argv.index('--preview') + 1]
        os.makedirs(preview_dir, exist_ok=True)
    os.makedirs(IMG_DIR, exist_ok=True)

    report = []

    def out(name, img):
        report.append((name, img.size, save(img, os.path.join(IMG_DIR, name))))

    # --- アイテムアイコン (4x4 のシート) ---
    icons = art_sprites.make_item_icons()
    items_sheet = sheet(icons, 4, 16, 16)
    out('items.png', items_sheet)

    # --- 敵 (1種につき2フレームを横に並べた 48x24) ---
    enemies = art_sprites.make_enemies()
    for i, (a, b) in enumerate(enemies):
        out('enemy%d.png' % i, sheet([a, b], 2, 24, 24))

    # --- 宝箱 ---
    c0, c1 = art_sprites.make_chests()
    chest_sheet = sheet([c0, c1], 2, 16, 16)
    out('chest.png', chest_sheet)

    # --- アプリアイコン (25x25) ---
    icon = new_image(25, 25)
    icon.alpha_composite(icons[2], (5, 4))
    out('app_icon.png', icon)

    # --- 勇者（Cヘッダ） ---
    frames = art_sprites.hero_frames()
    weapons = art_sprites.weapon_maps()
    write_headers(frames, weapons)

    # --- 背景 ---
    scenes = art_scenes.make_all()
    for name, img in scenes.items():
        if name.startswith(EXTERNAL_ART):
            continue
        out(name + '.png', img)

    # --- 店主 ---
    # 店の絵は外部で用意した一枚絵（tools/shop_art.py）に置き換えたので、
    # ここでは書き出さない。プレビューの確認用にだけ作る。
    keeper = art_shopkeeper.make()

    for name, size, n in report:
        print('%-18s %3dx%-3d colors=%d%s' % (name, size[0], size[1], n, '' if n <= 16 else '  (8bit)'))

    if preview_dir:
        upscale(on_bg(items_sheet, (0xFF, 0xFF, 0xFF)), 6).save(os.path.join(preview_dir, 'pv_items.png'))
        flat = []
        for a, b in enemies:
            flat += [a, b]
        upscale(on_bg(sheet(flat, 2, 24, 24), (0x55, 0x55, 0x55)), 5).save(os.path.join(preview_dir, 'pv_enemies.png'))
        upscale(on_bg(chest_sheet, (0x55, 0x55, 0x55)), 6).save(os.path.join(preview_dir, 'pv_chest.png'))
        # 勇者: 防具5種 x フレーム4種 + 武器
        cells = []
        for ai, apal in enumerate(ARMOR_PAL):
            for name, rows in frames:
                pal = dict(HERO_BASE)
                pal.update(apal)
                im = new_image(34, 30)
                im.alpha_composite(render_charmap(rows, pal), (6, 3))
                wkey = 'SWORD_SWING' if name == 'ATTACK' else 'SWORD_REST'
                if ai == 4:
                    wkey = wkey.replace('SWORD', 'AXE')
                wp = dict(WEAPON_PAL[min(ai, 3)])
                wp.update({'e': 'e', 'y': 'y', 'K': 'K'})
                wimg = render_charmap(weapons[wkey], wp)
                hx, hy = hero_hand(rows)
                ax, ay = weapon_anchor(weapons[wkey])
                im.alpha_composite(wimg, (6 + hx - ax, 3 + hy - ay))
                cells.append(im)
        hs = sheet(cells, 4, 34, 30)
        upscale(on_bg(hs, (0x55, 0xAA, 0x55)), 5).save(os.path.join(preview_dir, 'pv_hero.png'))
        for name, img in scenes.items():
            if name != 'bg_town':
                # タイルのつなぎ目確認用に2枚並べる
                img = sheet([img, img], 2, img.width, img.height)
            upscale(img, 3).save(os.path.join(preview_dir, 'pv_%s.png' % name))
        upscale(on_bg(keeper, (0x55, 0x55, 0x55)), 4).save(os.path.join(preview_dir, 'pv_shopkeeper.png'))
        # 店の画面のモック（basalt 144x168）
        shop = scenes['bg_shop']
        mock = new_image(144, 168, 'K')
        wall = shop.crop((0, 0, 128, 120))
        counter = shop.crop((0, 120, 128, 144))
        for x in (0, 128):
            mock.alpha_composite(wall, (x - 8, -12))
        mock.alpha_composite(keeper, ((144 - keeper.width) // 2, 4))
        for x in (0, 128):
            mock.alpha_composite(counter, (x - 8, 96))
        upscale(mock, 3).save(os.path.join(preview_dir, 'pv_mock_shop.png'))


if __name__ == '__main__':
    main()
