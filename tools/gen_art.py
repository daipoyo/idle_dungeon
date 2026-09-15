# -*- coding: utf-8 -*-
"""Idle Dungeon の画像リソースを生成するスクリプト（ゲームボーイカラー風）。

絵はすべて「ドット1個 = 画面の 2x2 ピクセル」の粗い解像度で描き、
書き出すときに 2倍に拡大する。1つの物に使う色はおおむね4色まで。

使い方:  python tools/gen_art.py [--preview 出力先ディレクトリ]

出力:
  resources/images/*.png   … Pebbleのビットマップリソース
  src/c/art_info.h         … 画像サイズや背景の塗り色などの定数
  src/c/sprite_data.h      … 勇者・武器の文字マップとパレット表（gfx.c 専用）
  src/c/font_data.h        … ドットフォント（gfx.c 専用）
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pebble_art import PAL, new_image, save, upscale, on_bg  # noqa: E402
import gb_sprites  # noqa: E402
import gb_scenes  # noqa: E402
import gb_font  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
IMG_DIR = os.path.join(ROOT, 'resources', 'images')
C_DIR = os.path.join(ROOT, 'src', 'c')

PX = 2  # ドット1個の大きさ（画面ピクセル）


def argb8(c):
    r, g, b = PAL[c]
    return 0xC0 | ((r // 85) << 4) | ((g // 85) << 2) | (b // 85)


def grid_image(grid, scale=PX):
    """記号の2次元リスト（None は透明）を画像にする。"""
    h = len(grid)
    w = len(grid[0])
    img = new_image(w, h)
    for y in range(h):
        for x in range(w):
            c = grid[y][x]
            if c is not None and c != '.':
                img.putpixel((x, y), PAL[c] + (255,))
    return upscale(img, scale) if scale > 1 else img


def sheet(images, cols, cell_w, cell_h):
    rows = (len(images) + cols - 1) // cols
    out = new_image(cols * cell_w, rows * cell_h)
    for i, im in enumerate(images):
        out.alpha_composite(im, ((i % cols) * cell_w, (i // cols) * cell_h))
    return out


def c_rows(lines, name, rows, size_expr):
    lines.append('static const char *const %s[%s] = {' % (name, size_expr))
    for r in rows:
        lines.append('  "%s",' % r)
    lines.append('};')
    lines.append('')


HEAD = [
    '// このファイルは tools/gen_art.py で自動生成されています。直接編集しないでください。',
    '#pragma once',
    '',
]


def write_file(name, lines):
    with open(os.path.join(C_DIR, name), 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))


def write_art_info(fills):
    lines = list(HEAD)
    lines.append('// 1ドットの大きさ（画面ピクセル）')
    lines.append('#define PX %d' % PX)
    lines.append('')
    lines.append('// 背景画像より画面が高いときに上側を塗る色 (argb)')
    for i in range(6):
        lines.append('#define BG_DUNGEON%d_FILL 0x%02X' % (i, argb8(fills['bg_dungeon%d' % i])))
    lines.append('#define BG_TOWN_FILL 0x%02X' % argb8(fills['bg_town']))
    lines.append('')
    lines.append('#define BG_TILE_W %d' % (gb_scenes.TILE_W * PX))
    lines.append('#define BG_FAR_H %d' % (gb_scenes.FAR_H * PX))
    lines.append('#define BG_GROUND_H %d' % (gb_scenes.GROUND_H * PX))
    lines.append('#define BG_TOWN_W %d' % (gb_scenes.TOWN_W * PX))
    lines.append('#define BG_TOWN_H %d' % (gb_scenes.TOWN_H * PX))
    lines.append('')
    lines.append('// 勇者の文字マップの大きさ（ドット数）')
    lines.append('#define HERO_MAP_W %d' % gb_sprites.HERO_W)
    lines.append('#define HERO_MAP_H %d' % gb_sprites.HERO_H)
    lines.append('// 敵・アイテム・宝箱の画像の大きさ（画面ピクセル）')
    lines.append('#define ENEMY_SIZE %d' % (gb_sprites.ENEMY_SIZE * PX))
    lines.append('#define ICON_SIZE %d' % (gb_sprites.ICON_SIZE * PX))
    lines.append('')
    write_file('art_info.h', lines)


def write_sprite_data(frames, weapons):
    lines = list(HEAD)
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
        hx, hy = gb_sprites.hero_hand(rows)
        lines.append('#define HERO_%s_HAND_X %d' % (name, hx))
        lines.append('#define HERO_%s_HAND_Y %d' % (name, hy))
        c_rows(lines, 'HERO_' + name, rows, 'HERO_MAP_H')
    for name, rows in weapons.items():
        ax, ay = gb_sprites.weapon_anchor(rows)
        lines.append('#define WPN_%s_W %d' % (name, len(rows[0])))
        lines.append('#define WPN_%s_H %d' % (name, len(rows)))
        lines.append('#define WPN_%s_AX %d' % (name, ax))
        lines.append('#define WPN_%s_AY %d' % (name, ay))
        c_rows(lines, 'WPN_' + name, rows, str(len(rows)))
    write_file('sprite_data.h', lines)


def write_font_data():
    lines = list(HEAD)
    lines.append('#include <stdint.h>')
    lines.append('')
    lines.append('// ドットフォント（%dドットの高さ）。ASCII %d〜%d。' %
                 (gb_font.GLYPH_H, gb_font.FIRST, gb_font.LAST))
    lines.append('// 1文字 = 幅 + 各行のビット（下位ビットが左端）。小文字は大文字で描く。')
    lines.append('#define FONT_GLYPH_H %d' % gb_font.GLYPH_H)
    lines.append('#define FONT_FIRST %d' % gb_font.FIRST)
    lines.append('#define FONT_LAST %d' % gb_font.LAST)
    lines.append('static const uint8_t FONT_GLYPHS[][%d] = {' % (gb_font.GLYPH_H + 1))
    for code in range(gb_font.FIRST, gb_font.LAST + 1):
        w, bits = gb_font.encode(chr(code))
        label = chr(code).replace('\\', 'backslash')
        lines.append('  { %d, %s },  // %s' % (w, ', '.join('0x%02X' % b for b in bits), label))
    lines.append('};')
    lines.append('')
    write_file('font_data.h', lines)


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
    icons = [grid_image(g) for g in gb_sprites.item_icons()]
    cell = gb_sprites.ICON_SIZE * PX
    items_sheet = sheet(icons, 4, cell, cell)
    out('items.png', items_sheet)

    # --- 敵 (1種につき2フレームを横に並べる) ---
    esz = gb_sprites.ENEMY_SIZE * PX
    enemies = [(grid_image(a), grid_image(b)) for a, b in gb_sprites.enemies()]
    for i, (a, b) in enumerate(enemies):
        out('enemy%d.png' % i, sheet([a, b], 2, esz, esz))

    # --- 宝箱 ---
    chests = [grid_image(g) for g in gb_sprites.chests()]
    chest_sheet = sheet(chests, 2, cell, cell)
    out('chest.png', chest_sheet)

    # --- アプリアイコン (25x25) ---
    icon = new_image(25, 25)
    icon.alpha_composite(grid_image(gb_sprites.app_icon()), (0, 0))
    out('app_icon.png', icon)

    # --- 背景 ---
    fills = {}
    scenes = {}
    for i, grid in enumerate(gb_scenes.dungeons()):
        name = 'bg_dungeon%d' % i
        scenes[name] = grid_image(grid)
        fills[name] = gb_scenes.top_color(grid)
        out(name + '.png', scenes[name])
    town = gb_scenes.town()
    scenes['bg_town'] = grid_image(town)
    fills['bg_town'] = gb_scenes.top_color(town)
    out('bg_town.png', scenes['bg_town'])

    # --- お店（機種ごとに画面と同じ大きさ）と店主の顔 ---
    shop = {}
    for plat, (w, h, dlg_h) in gb_scenes.SHOP_SCREENS.items():
        shop[plat] = grid_image(gb_scenes.shop_scene(w // PX, h // PX, (h - dlg_h) // PX))
        out('shop_scene~%s.png' % plat, shop[plat])
    out('shop_scene.png', shop['basalt'])
    face = grid_image(gb_sprites.keeper_face())
    out('keeper_face.png', face)

    # --- 勇者・武器・フォント（Cヘッダ） ---
    frames = gb_sprites.hero_frames()
    weapons = gb_sprites.weapon_maps()
    write_art_info(fills)
    write_sprite_data(frames, weapons)
    write_font_data()

    for name, size, n in report:
        print('%-22s %3dx%-3d colors=%d' % (name, size[0], size[1], n))

    if preview_dir:
        write_previews(preview_dir, icons, enemies, chests, scenes, shop, face, frames, weapons)


def write_previews(d, icons, enemies, chests, scenes, shop, face, frames, weapons):
    gray = (0x55, 0x55, 0x55)
    upscale(on_bg(sheet(icons, 8, 16, 16), (0, 0, 0)), 4).save(os.path.join(d, 'pv_items.png'))
    flat = []
    for a, b in enemies:
        flat += [a, b]
    upscale(on_bg(sheet(flat + chests, 4, 24, 24), gray), 4).save(os.path.join(d, 'pv_enemies.png'))
    # 勇者: 防具5種 x ポーズ4種（武器は防具の番号に合わせて変える）
    cells = []
    for ai in range(5):
        for name, rows in frames:
            pal = dict(gb_sprites.HERO_BASE)
            pal.update(gb_sprites.ARMOR_PAL[ai])
            wi = min(ai, 3)
            pal.update(gb_sprites.WEAPON_PAL[wi])
            im = new_image(24, 20)
            hero = [[pal.get(ch, ch) if ch != '.' else None for ch in r] for r in rows]
            im.alpha_composite(grid_image(hero, 1), (4, 2))
            wkey = ('AXE_' if wi == 3 else 'SWORD_') + ('SWING' if name == 'ATTACK' else 'REST')
            wrows = weapons[wkey]
            wgrid = [[pal.get(ch, ch) if ch != '.' else None for ch in r] for r in wrows]
            hx, hy = gb_sprites.hero_hand(rows)
            ax, ay = gb_sprites.weapon_anchor(wrows)
            im.alpha_composite(grid_image(wgrid, 1), (4 + hx - ax, 2 + hy - ay))
            cells.append(im)
    upscale(on_bg(sheet(cells, 4, 24, 20), (0x55, 0xAA, 0xFF)), 8).save(os.path.join(d, 'pv_hero.png'))
    for i in range(6):
        img = scenes['bg_dungeon%d' % i]
        upscale(sheet([img, img], 2, img.width, img.height), 2).save(os.path.join(d, 'pv_bg_dungeon%d.png' % i))
    upscale(scenes['bg_town'], 2).save(os.path.join(d, 'pv_bg_town.png'))
    row = new_image(sum(s.width + 8 for s in shop.values()), 260)
    x = 0
    for s in shop.values():
        row.alpha_composite(s, (x, 0))
        x += s.width + 8
    upscale(on_bg(row, gray), 2).save(os.path.join(d, 'pv_shop.png'))
    upscale(on_bg(face, gray), 4).save(os.path.join(d, 'pv_face.png'))
    fimg = gb_font.preview()
    upscale(fimg, 4).save(os.path.join(d, 'pv_font.png'))


if __name__ == '__main__':
    main()
