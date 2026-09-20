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
  src/c/qr_data.h          … 支援ページのURLのQRコード
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from pebble_art import PAL, new_image, save, upscale, on_bg  # noqa: E402
import gb_sprites  # noqa: E402
import gb_scenes  # noqa: E402
import gb_font  # noqa: E402
import gb_items  # noqa: E402
import gb_qr  # noqa: E402

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
    lines.append('#define BG_TOWN_NIGHT_FILL 0x%02X' % argb8(fills['bg_town_night']))
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
    for name, rows in gb_sprites.gear_overlays().items():
        c_rows(lines, 'GEAR_' + name, rows, 'HERO_MAP_H')
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


# 支援ページ（作者への寄付）。時計にQRを出して、スマホのカメラで読んでもらう
SUPPORT_URL = 'ko-fi.com/daipoyo'


def write_qr_data():
    rows, version = gb_qr.qr_rows(SUPPORT_URL)
    lines = list(HEAD)
    lines.append('// 支援ページ %s の QR（バージョン%d、余白 %dマス込み）' %
                 (SUPPORT_URL, version, gb_qr.QUIET))
    lines.append('#define QR_SUPPORT_URL "%s"' % SUPPORT_URL)
    lines.append('#define QR_SUPPORT_SIZE %d' % len(rows))
    lines.append('')
    c_rows(lines, 'QR_SUPPORT', rows, 'QR_SUPPORT_SIZE')
    write_file('qr_data.h', lines)
    return len(rows), version


def main():
    preview_dir = None
    if '--preview' in sys.argv:
        preview_dir = sys.argv[sys.argv.index('--preview') + 1]
        os.makedirs(preview_dir, exist_ok=True)
    os.makedirs(IMG_DIR, exist_ok=True)

    report = []

    def out(name, img):
        report.append((name, img.size, save(img, os.path.join(IMG_DIR, name))))

    cell = gb_sprites.ICON_SIZE * PX

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
    town_night = gb_scenes.town(night=True)
    scenes['bg_town_night'] = grid_image(town_night)
    fills['bg_town_night'] = gb_scenes.top_color(town_night)
    out('bg_town_night.png', scenes['bg_town_night'])

    # --- 施設の人物の顔（一覧の見出しに出す） ---
    faces = {'keeper': grid_image(gb_sprites.keeper_face())}
    out('keeper_face.png', faces['keeper'])
    for name, grid in gb_sprites.npc_faces().items():
        faces[name] = grid_image(grid)
        out('face_%s.png' % name, faces[name])

    # --- 勇者・武器・フォント（Cヘッダ） ---
    frames = gb_sprites.hero_frames()
    weapons = gb_sprites.weapon_maps()
    write_art_info(fills)
    write_sprite_data(frames, weapons)
    write_font_data()
    qr_size, qr_version = write_qr_data()
    report.append(('qr_data.h  %s' % SUPPORT_URL, (qr_size, qr_version), 2))

    # --- 図鑑アイテムの絵（resources/data/item_icons.bin と src/c/item_art.h） ---
    shapes, specials = gb_items.build(ROOT, PAL)
    report.append(('item_icons.bin', (len(shapes) + len(specials) + len(gb_items.UI_ICONS), 1), 6))

    for name, size, n in report:
        print('%-22s %3dx%-3d colors=%d' % (name, size[0], size[1], n))

    if preview_dir:
        write_previews(preview_dir, enemies, chests, scenes, faces, frames, weapons)
        gb_items.preview(ROOT, PAL, preview_dir, new_image, upscale)


# 見本の装備（武器, 胴, 頭, 左手）。名前は形の ID、先頭の数字は素材の段階。固有装備は名前で書く
HERO_LOADOUTS = [
    ('NO GEAR', None, None, None, None),
    ('BRONZE', (0, 'SHORT_SWORD'), (0, 'TUNIC'), None, None),
    ('IRON', (1, 'BATTLE_AXE'), (1, 'CHAIN_MAIL'), (1, 'COIF'), (1, 'ROUND_SHIELD')),
    ('STEEL', (2, 'HALBERD'), (2, 'FULL_PLATE'), (2, 'GREAT_HELM'), None),
    ('SILK', (2, 'STAFF'), (2, 'ROBE'), (2, 'HOOD'), (2, 'ORB')),
    ('EMBER', (3, 'CLAYMORE'), (3, 'FULL_PLATE'), (3, 'HORNED_HELM'), (3, 'TOWER_SHIELD')),
    ('RIME', (4, 'KATANA'), (4, 'MANTLE'), (4, 'CIRCLET'), None),
    ('VOID', (5, 'WAR_HAMMER'), (5, 'HALF_PLATE'), (5, 'CROWN'), (5, 'BULWARK')),
    ('ROGUE', (5, 'KUKRI'), (5, 'JERKIN'), (5, 'BANDANA'), None),
    ('MONK', (4, 'TALONS'), (4, 'GAMBESON'), None, (4, 'LANTERN')),
    ('UNIQUE', 'Tidecaller', 'Leviathan Scale', 'Coral Barbute', 'Barnacle Bulwark'),
    ('UNIQUE', 'Colossus Maul', 'Gatewarden', 'Doomcrown', None),
]


def gear_kind(shape_id, slot):
    """C 側の gfx_draw_hero と同じ分け方。"""
    order = [sid for sid, _, _, _ in gb_items.read_catalog(ROOT)[0]]
    i = order.index(shape_id)
    at = order.index
    if slot == 'weapon':
        for first, kind in (('KNUCKLES', 'FIST'), ('STAFF', 'STAFF'), ('SPEAR', 'POLE'),
                            ('WAR_HAMMER', 'HAMMER'), ('CLUB', 'MACE'), ('HATCHET', 'AXE'),
                            ('KNIFE', 'DAGGER')):
            if i >= at(first):
                return kind
        return 'SWORD'
    if slot == 'head':
        if i >= at('COIF'):
            return 'HEAD_HELM'
        if i >= at('CIRCLET'):
            return 'HEAD_CROWN'
        return 'HEAD_CLOTH'
    return 'OFF_FOCUS' if i >= at('ORB') else 'OFF_SHIELD'


def gear_colors(entry):
    """(形 ID, 暗, 中, 明, 固定色1, 固定色2)。entry は (段階, 形 ID) か固有装備の名前。"""
    shapes, specials = gb_items.read_catalog(ROOT)
    if isinstance(entry, tuple):
        tier, sid = entry
        fam = [f for s_id, _, _, f in shapes if s_id == sid][0]
        colors = gb_items.FAMILIES[gb_items.FAMILY_INDEX[fam]][1][tier][1]
        accents = gb_items.SHAPES[sid][0]
        return (sid,) + tuple(colors) + tuple(accents)
    sid = [s_id for name, s_id, _ in specials if name == entry][0]
    return (sid,) + tuple(gb_items.UNIQUE_COLORS[entry])


def hero_gear_preview(d, frames, weapons):
    """勇者に装備を着せた見本（C 側の gfx_draw_hero と同じ重ね方）。"""
    idle = dict(frames)['IDLE']
    overlays = gb_sprites.gear_overlays()
    cells = []
    for label, wpn, body, head, off in HERO_LOADOUTS:
        pal = dict(gb_sprites.HERO_BASE)
        pal.update(gb_sprites.HERO_NO_ARMOR)
        if body:
            _, dark, mid, _, _, _ = gear_colors(body)
            pal.update({'1': mid, '2': dark})
        cw, ch = 30, 30
        im = new_image(cw, ch)
        ox, oy = 8, 10

        def paste(rows, mapping, x, y):
            grid = [[mapping.get(c, c) if c != '.' else None for c in r] for r in rows]
            im.alpha_composite(grid_image(grid, 1), (x, y))

        paste(idle, pal, ox, oy)
        if off:
            sid, dark, mid, light, a1, _ = gear_colors(off)
            paste(overlays[gear_kind(sid, 'off')],
                  dict(pal, **{'7': light, '8': mid, '9': dark, 'j': a1}), ox, oy)
        if head:
            sid, dark, mid, light, a1, _ = gear_colors(head)
            paste(overlays[gear_kind(sid, 'head')], dict(pal, **{'7': light, '8': mid, '9': dark, 'j': a1}), ox, oy)
        if wpn:
            sid, dark, mid, light, a1, _ = gear_colors(wpn)
            kind = gear_kind(sid, 'weapon')
            wrows = weapons[kind + '_REST']
            hx, hy = gb_sprites.hero_hand(idle)
            ax, ay = gb_sprites.weapon_anchor(wrows)
            paste(wrows, dict(pal, **{'4': mid, '5': light, '6': a1}), ox + hx - ax, oy + hy - ay)
        cells.append((label, im))
    cols = 6
    scale = 6
    label_h = 9
    out = new_image(cols * 30 * scale, ((len(cells) + cols - 1) // cols) * (30 * scale + label_h * 2))
    out.paste((0, 0, 0, 255), (0, 0, out.width, out.height))
    for i, (label, im) in enumerate(cells):
        x = (i % cols) * 30 * scale
        y = (i // cols) * (30 * scale + label_h * 2)
        bg = new_image(30, 30)
        bg.paste(PAL['t'] + (255,), (0, 0, 30, 30))
        bg.paste(PAL['d'] + (255,), (0, 27, 30, 30))
        bg.alpha_composite(im, (0, 0))
        out.alpha_composite(upscale(bg, scale), (x, y))
        # ラベル（ドットフォントを2倍で）
        tx = x + 6
        for chx in label:
            w, bits = gb_font.encode(chx)
            for gy, v in enumerate(bits):
                for gx in range(w):
                    if v >> gx & 1:
                        out.paste((255, 255, 255, 255), (tx + gx * 2, y + 30 * scale + 3 + gy * 2,
                                                         tx + gx * 2 + 2, y + 30 * scale + 5 + gy * 2))
            tx += (w + 1) * 2
    out.save(os.path.join(d, 'pv_hero_gear.png'))


def write_previews(d, enemies, chests, scenes, faces, frames, weapons):
    gray = (0x55, 0x55, 0x55)
    flat = []
    for a, b in enemies:
        flat += [a, b]
    upscale(on_bg(sheet(flat + chests, 4, 24, 24), gray), 4).save(os.path.join(d, 'pv_enemies.png'))
    hero_gear_preview(d, frames, weapons)
    for i in range(6):
        img = scenes['bg_dungeon%d' % i]
        upscale(sheet([img, img], 2, img.width, img.height), 2).save(os.path.join(d, 'pv_bg_dungeon%d.png' % i))
    upscale(scenes['bg_town'], 2).save(os.path.join(d, 'pv_bg_town.png'))
    upscale(scenes['bg_town_night'], 2).save(os.path.join(d, 'pv_bg_town_night.png'))
    row = new_image(len(faces) * 52, 44)
    for i, img in enumerate(faces.values()):
        row.alpha_composite(img, (i * 52, 0))
    upscale(on_bg(row, gray), 4).save(os.path.join(d, 'pv_faces.png'))
    fimg = gb_font.preview()
    upscale(fimg, 4).save(os.path.join(d, 'pv_font.png'))


if __name__ == '__main__':
    main()
