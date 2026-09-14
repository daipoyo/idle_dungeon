# -*- coding: utf-8 -*-
"""6面まとめて描かれた背景画から、ダンジョン背景のタイルを切り出す。

背景は横に繰り返し表示され、遠景（上96px）と地面（下16px）が別々の速さで
流れる。そのため左端と右端がつながっている必要がある。
生成画像はつながらないので、次の手順で処理する。

  1. パネルごとに切り出す（左側はラベル文字があるので落とす）
  2. 高さをタイルに合わせて縮小する
  3. 左端と右端がもっとも似ている幅128の窓を総当たりで探す
  4. 残った段差を細い帯で馴染ませる
  5. Pebble の64色に落とし、さらに色数を絞る

使い方:
  python tools/dungeon_art.py 元画像 [--write]
    --write で resources/images/bg_dungeon*.png を置き換える。
"""
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from img_to_pebble import reduce_none, count_colors  # noqa: E402
from shop_art import tone  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TILE_W, FAR_H, GROUND_H = 128, 96, 16
TILE_H = FAR_H + GROUND_H
COLORS = 16          # 16色以下なら Pebble が4ビットで持てる
GAMMA = 1.35         # 1.0 より大きいほど暗くなる（白飛びを抑える）
SATURATION = 1.05
LABEL_MARGIN = 0.26  # パネル左側のラベル文字を避けるために落とす割合

# (ダンジョン番号, 名前, 元画像内での範囲)
PANELS = [
    (0, 'Slime Cave',      (0.004, 0.014, 0.492, 0.304)),
    (1, 'Goblin Woods',    (0.504, 0.014, 0.996, 0.304)),
    (2, 'Ancient Ruins',   (0.004, 0.350, 0.492, 0.638)),
    (3, "Dragon's Lair",   (0.004, 0.674, 0.327, 0.995)),
    (4, 'Sunken Temple',   (0.335, 0.674, 0.659, 0.995)),
    (5, 'Abyssal Depths',  (0.667, 0.674, 0.995, 0.995)),
]


def col_diff(px, h, xa, xb):
    s = 0
    for y in range(h):
        a, b = px[xa, y], px[xb, y]
        s += abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])
    return s


def best_window(img, w):
    """幅 w の窓のうち、左端と右端の列がもっとも似ている位置を返す。"""
    px = img.load()
    best, best_x = None, 0
    for x in range(0, max(1, img.width - w)):
        d = col_diff(px, img.height, x, x + w - 1)
        if best is None or d < best:
            best, best_x = d, x
    return best_x, (best or 0)


def heal_seam(img, band=6):
    """右端の帯に左端の列を混ぜて段差を消す。"""
    px = img.load()
    w, h = img.size
    for i in range(band):
        x = w - band + i
        t = (i + 1) / (band + 1)
        for y in range(h):
            a, b = px[x, y], px[i, y]
            px[x, y] = tuple(int(a[c] * (1 - t) + b[c] * t) for c in range(3))
    return img


def choose_palette(img, n):
    """誤差がもっとも小さくなる n 色を選ぶ。

    出現数の多い順に選ぶと、面積の広い明るい面ばかりが残り、
    中間調が失われて平坦な絵になる（白飛びして見える原因）。
    ここでは「その色を加えたときに全体の誤差がどれだけ減るか」で
    1色ずつ選んでいく。候補は元画像に実際にある色だけなので、
    平均によって元に無い色が作られることもない。
    """
    hist = {}
    for p in img.getdata():
        hist[p] = hist.get(p, 0) + 1
    cands = list(hist.keys())
    if len(cands) <= n:
        return cands

    def d2(a, b):
        return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2

    chosen = [max(hist, key=hist.get)]                 # 最初は最頻色
    best = {c: d2(c, chosen[0]) for c in cands}        # 各色の現在の誤差
    while len(chosen) < n:
        pick, gain = None, -1
        for c in cands:
            if c in chosen:
                continue
            g = sum(hist[o] * (best[o] - min(best[o], d2(o, c))) for o in cands)
            if g > gain:
                pick, gain = c, g
        if pick is None or gain <= 0:
            break
        chosen.append(pick)
        for o in cands:
            best[o] = min(best[o], d2(o, pick))
    return chosen


def map_to(img, palette):
    out = img.copy()
    px = out.load()
    cache = {}
    for y in range(out.height):
        for x in range(out.width):
            p = px[x, y]
            if p not in cache:
                cache[p] = min(palette, key=lambda c: sum((c[i] - p[i]) ** 2 for i in range(3)))
            px[x, y] = cache[p]
    return out


def make_tile(src, box):
    w, h = src.size
    panel = src.crop((int(box[0] * w), int(box[1] * h), int(box[2] * w), int(box[3] * h)))
    # ラベル文字のある左側を落とす
    panel = panel.crop((int(panel.width * LABEL_MARGIN), 0, panel.width, panel.height))
    scale = TILE_H / panel.height
    panel = panel.resize((max(TILE_W + 8, int(panel.width * scale)), TILE_H), Image.LANCZOS)
    x, diff = best_window(panel, TILE_W)
    tile = heal_seam(panel.crop((x, 0, x + TILE_W, TILE_H)))
    # 明るさを落としてから減色する。Pebble は各色4段階しかないので、
    # そのまま丸めると中間の明るさが上の段へ寄って白っぽくなる。
    tile = reduce_none(tone(tile, gamma=GAMMA, sat=SATURATION))
    return map_to(tile, choose_palette(tile, COLORS)), diff


def argb_of(color):
    """GColor8 の 1バイト表現（art_info.h の FILL 定数と同じ形式）。"""
    r, g, b = color
    return 0xC0 | ((r // 85) << 4) | ((g // 85) << 2) | (b // 85)


def main():
    src = Image.open(sys.argv[1]).convert('RGB')
    write = '--write' in sys.argv
    out_dir = os.path.join(ROOT, 'resources', 'images') if write \
        else os.path.join(ROOT, 'build', 'dungeon_art')
    os.makedirs(out_dir, exist_ok=True)
    print('元画像: %dx%d  出力先: %s' % (src.width, src.height, out_dir))

    tiles = []
    for idx, name, box in PANELS:
        tile, diff = make_tile(src, box)
        tile.save(os.path.join(out_dir, 'bg_dungeon%d.png' % idx))
        # 画面が背景より高いときに上を塗る色＝タイル最上段で一番多い色
        top_row = [tile.getpixel((x, 0)) for x in range(TILE_W)]
        fill = max(set(top_row), key=top_row.count)
        tiles.append((idx, name, tile, fill))
        print('  %d %-16s 色数=%-3d 端の差=%-7d 上端の色=0x%02X' %
              (idx, name, count_colors(tile), diff, argb_of(fill)))

    # 確認用：各タイルを2回並べて継ぎ目を見る
    gap = 4
    cw = TILE_W * 2
    sheet = Image.new('RGB', (3 * (cw + gap) + gap, 2 * (TILE_H + gap) + gap), (0x2A, 0x22, 0x28))
    for i, (_, _, t, _) in enumerate(tiles):
        far, ground = t.crop((0, 0, TILE_W, FAR_H)), t.crop((0, FAR_H, TILE_W, TILE_H))
        cell = Image.new('RGB', (cw, TILE_H))
        for k in range(2):
            cell.paste(far, (TILE_W * k, 0))
            cell.paste(ground, (TILE_W * k, FAR_H))
        sheet.paste(cell, (gap + (i % 3) * (cw + gap), gap + (i // 3) * (TILE_H + gap)))
    prev = os.path.join(ROOT, 'build', 'dungeon_art')
    os.makedirs(prev, exist_ok=True)
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(
        os.path.join(prev, 'dungeons_check.png'))

    print('\nart_info.h 用の塗り色:')
    for idx, _, _, fill in tiles:
        print('  #define BG_DUNGEON%d_FILL 0x%02X' % (idx, argb_of(fill)))


if __name__ == '__main__':
    main()
