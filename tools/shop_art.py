# -*- coding: utf-8 -*-
"""店の絵（外部で用意した一枚絵）から、ゲーム用のリソースを作る。

出力は2種類 × 機種ごと:
  shop_scene~<機種>.png  … 店頭画面の全面に貼る絵
  keeper_face~<機種>.png … 購入・売却画面の見出しに出す顔

機種ごとに画面サイズが違うので、同じ縦横比では収まらない。
  basalt 144x168 / chalk 180x180 / emery 200x228 / gabbro 260x260
そこで機種ごとに、必要な縦横比に合わせて切り出す範囲を調整する。

Pebble は 8Bit 画像で 1画素=1バイトなので、全面の絵はそれなりの RAM を食う。
basalt で 144*168 = 約24KB（空きヒープは約44KB）。これが上限に近い。

使い方:
  python tools/shop_art.py 元画像 [--write]
    --write を付けると resources/images/ に書き出す。
    付けない場合は build/shop_art/ に出すだけ（確認用）。
"""
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from img_to_pebble import reduce_none, count_colors  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# 機種名 -> 画面サイズ
SCREENS = {
    'basalt': (144, 168),
    'chalk': (180, 180),
    'emery': (200, 228),
    'gabbro': (260, 260),
}

# 全面の絵に許す色数。16色以下なら Pebble が4ビット形式（1画素0.5バイト）で
# 持ってくれるので、消費メモリが半分になる。RAM に余裕のない機種はこれで抑える。
#   basalt 空きヒープ約45KB / chalk 約45KB / emery・gabbro 約110KB
SCENE_COLORS = {'basalt': 16, 'chalk': 16, 'emery': 0, 'gabbro': 0}   # 0 は制限なし

# 見出しの顔の大きさ。shop_window.c の FACE_W / LIST_HEADER_H と合わせる。
#   FACE_W        = 画面幅200以上なら56、それ以外は46
#   LIST_HEADER_H = 丸型は画面高の34%、幅200以上なら58、それ以外は46（いずれも-2して使う）
FACE_SIZES = {
    'basalt': (46, 44),
    'chalk': (46, 59),
    'emery': (56, 56),
    'gabbro': (56, 56),
}

# 元画像の中で使いたい範囲（正規化座標）
SCENE_BOX = (0.113, 0.190, 0.887, 0.880)   # 人物とカウンター
FACE_BOX = (0.395, 0.250, 0.605, 0.410)    # 顔まわり


def fit_box(box, aspect, size):
    """切り出し範囲を、目標の縦横比に合わせて中心から広げる／狭める。"""
    w, h = size
    x0, y0, x1, y1 = box[0] * w, box[1] * h, box[2] * w, box[3] * h
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    bw, bh = x1 - x0, y1 - y0
    if bw / bh > aspect:
        bh = bw / aspect            # 横が余る → 縦に広げる
    else:
        bw = bh * aspect            # 縦が余る → 横に広げる
    # 画面外にはみ出す場合は中心をずらして収める
    bw, bh = min(bw, w), min(bh, h)
    cx = min(max(cx, bw / 2), w - bw / 2)
    cy = min(max(cy, bh / 2), h - bh / 2)
    return (int(cx - bw / 2), int(cy - bh / 2), int(cx + bw / 2), int(cy + bh / 2))


def tone(img, gamma=1.25, sat=1.05):
    px = img.load()
    out = img.copy()
    dst = out.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = [255 * (v / 255) ** gamma for v in px[x, y]]
            m = (r + g + b) / 3
            r, g, b = [m + (v - m) * sat for v in (r, g, b)]
            dst[x, y] = tuple(int(max(0, min(255, v))) for v in (r, g, b))
    return out


def limit_colors(img, n):
    """色数を n 色以下に落とす。メディアンカットで選んだ代表色を、
    Pebble の64色格子に丸め直してから割り当てる。"""
    p = img.quantize(colors=n, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    pal = p.getpalette()[:n * 3]
    snapped = [(v // 85 + (1 if v % 85 > 42 else 0)) * 85 for v in pal]
    p.putpalette(snapped + [0] * (768 - len(snapped)))
    return p.convert('RGB')


def render(src, box, size, colors=0):
    crop = src.crop(fit_box(box, size[0] / size[1], src.size))
    img = reduce_none(tone(crop.resize(size, Image.LANCZOS)))
    return limit_colors(img, colors) if colors else img


def main():
    src_path = sys.argv[1]
    write = '--write' in sys.argv
    out_dir = os.path.join(ROOT, 'resources', 'images') if write \
        else os.path.join(ROOT, 'build', 'shop_art')
    os.makedirs(out_dir, exist_ok=True)
    src = Image.open(src_path).convert('RGB')
    print('元画像: %dx%d  出力先: %s' % (src.width, src.height, out_dir))

    total = 0
    for plat, size in SCREENS.items():
        limit = SCENE_COLORS[plat]
        scene = render(src, SCENE_BOX, size, colors=limit)
        path = os.path.join(out_dir, 'shop_scene~%s.png' % plat)
        scene.save(path)
        n = count_colors(scene)
        ram = size[0] * size[1] // (2 if n <= 16 else 1)      # 16色以下は4ビット
        total += os.path.getsize(path)
        print('  shop_scene~%-7s %3dx%-3d 色数=%-3d RAM=%dKB' % (plat, size[0], size[1], n, ram // 1024))

        fsize = FACE_SIZES[plat]
        face = render(src, FACE_BOX, fsize)
        path = os.path.join(out_dir, 'keeper_face~%s.png' % plat)
        face.save(path)
        total += os.path.getsize(path)
        print('  keeper_face~%-6s %3dx%-3d 色数=%d' %
              (plat, fsize[0], fsize[1], count_colors(face)))

    # タグ無しの控え（機種判定が外れたときに使われる）
    render(src, SCENE_BOX, SCREENS['basalt'], colors=16).save(
        os.path.join(out_dir, 'shop_scene.png'))
    render(src, FACE_BOX, FACE_SIZES['basalt']).save(os.path.join(out_dir, 'keeper_face.png'))
    print('  (控えとして基本ファイルも出力)')
    print('ファイル合計 %.1fKB' % (total / 1024))


if __name__ == '__main__':
    main()
