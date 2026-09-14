# -*- coding: utf-8 -*-
"""任意の画像を Pebble のカラー機で表示できる形に変換する。

Pebble の色は RGB 各チャンネルが 0x00 / 0x55 / 0xAA / 0xFF の4段階に固定された
64色。各チャンネルを最も近い段階に丸めれば、それが最も近い色になる（パレットが
立方格子なので、単純な丸めが最近傍探索と一致する）。

減色の方式を3つ出して比べられるようにしている。
  none    … 丸めるだけ。面がはっきりするが、緩やかな階調は縞になる
  ordered … 4x4のベイヤーディザ。規則的な網点になり、ドット絵と馴染みやすい
  fs      … 誤差拡散（Floyd-Steinberg）。階調は最も滑らかだが、粒が散る

使い方:
  python tools/img_to_pebble.py 入力画像 [出力先ディレクトリ]
"""
import os
import sys

from PIL import Image

LEVELS = (0x00, 0x55, 0xAA, 0xFF)
STEP = 85.0

# 出力するサイズ。basalt は画面 144x168、emery は 200x228。
SIZES = [(112, 144), (96, 120), (72, 90)]

BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]


def quant(v):
    """0..255 を Pebble の4段階に丸める。"""
    if v < 0:
        v = 0
    elif v > 255:
        v = 255
    return LEVELS[int(v / STEP + 0.5)]


def crop_to_aspect(img, tw, th):
    """目標の縦横比に合わせて中央を切り出す。人物なので上寄りに取る。"""
    want = tw / th
    have = img.width / img.height
    if have > want:                       # 横に広い → 左右を削る
        w = int(round(img.height * want))
        x = (img.width - w) // 2
        return img.crop((x, 0, x + w, img.height))
    h = int(round(img.width / want))      # 縦に長い → 下を削る（顔を残す）
    return img.crop((0, 0, img.width, h))


def prepare(img, size, gamma=1.0, sat=1.0):
    """切り出して縮小し、必要なら明るさと彩度を補正する。"""
    img = crop_to_aspect(img.convert('RGB'), *size)
    img = img.resize(size, Image.LANCZOS)
    if gamma == 1.0 and sat == 1.0:
        return img
    px = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b = px[x, y]
            if gamma != 1.0:
                r = 255 * (r / 255) ** gamma
                g = 255 * (g / 255) ** gamma
                b = 255 * (b / 255) ** gamma
            if sat != 1.0:
                m = (r + g + b) / 3
                r, g, b = (m + (r - m) * sat, m + (g - m) * sat, m + (b - m) * sat)
            px[x, y] = (int(max(0, min(255, r))), int(max(0, min(255, g))),
                        int(max(0, min(255, b))))
    return img


def reduce_none(img):
    out = img.copy()
    px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            r, g, b = px[x, y]
            px[x, y] = (quant(r), quant(g), quant(b))
    return out


def reduce_ordered(img):
    out = img.copy()
    px = out.load()
    for y in range(out.height):
        for x in range(out.width):
            # 段階の間隔の分だけ、規則的に上下に振ってから丸める
            t = (BAYER4[y % 4][x % 4] + 0.5) / 16.0 - 0.5
            r, g, b = px[x, y]
            px[x, y] = (quant(r + t * STEP), quant(g + t * STEP), quant(b + t * STEP))
    return out


def reduce_fs(img):
    """Floyd-Steinberg 誤差拡散。"""
    w, h = img.size
    src = [[list(img.getpixel((x, y))) for x in range(w)] for y in range(h)]
    out = Image.new('RGB', (w, h))
    px = out.load()
    for y in range(h):
        for x in range(w):
            old = src[y][x]
            new = [quant(v) for v in old]
            px[x, y] = tuple(new)
            err = [old[i] - new[i] for i in range(3)]
            for (dx, dy, f) in ((1, 0, 7 / 16), (-1, 1, 3 / 16), (0, 1, 5 / 16), (1, 1, 1 / 16)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h:
                    for i in range(3):
                        src[ny][nx][i] += err[i] * f
    return out


METHODS = {'none': reduce_none, 'ordered': reduce_ordered, 'fs': reduce_fs}


def count_colors(img):
    return len({p for p in img.getdata()})


def main():
    src_path = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else 'build/pebble_ref'
    os.makedirs(out_dir, exist_ok=True)
    src = Image.open(src_path)
    print('入力: %s  %dx%d' % (os.path.basename(src_path), src.width, src.height))

    BG = (0x2A, 0x22, 0x28)
    for size in SIZES:
        base = prepare(src, size)
        row = []
        for name, fn in METHODS.items():
            img = fn(base)
            img.save(os.path.join(out_dir, 'ref_%dx%d_%s.png' % (size[0], size[1], name)))
            print('  %dx%-4d %-8s 色数=%d' % (size[0], size[1], name, count_colors(img)))
            row.append(img)
        # 3方式を並べた比較（4倍）
        gap = 4
        sheet = Image.new('RGB', (len(row) * (size[0] + gap) + gap, size[1] + gap * 2), BG)
        for i, im in enumerate(row):
            sheet.paste(im, (gap + i * (size[0] + gap), gap))
        sheet.resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(
            os.path.join(out_dir, 'compare_%dx%d_x4.png' % size))

    # 明るさを段階的に落とした版。丸めるだけの方式だと肌のハイライトが
    # 白へ飛びやすいので、どのくらい落とすと肌が残るかを並べて比べる。
    size = SIZES[0]
    row, labels = [], []
    for gamma, sat in ((1.0, 1.0), (1.2, 1.05), (1.4, 1.1), (1.7, 1.2)):
        img = reduce_none(prepare(src, size, gamma=gamma, sat=sat))
        tag = 'g%02d' % int(gamma * 10)
        img.save(os.path.join(out_dir, 'ref_%dx%d_none_%s.png' % (size[0], size[1], tag)))
        print('  %dx%-4d none %-4s 色数=%d' % (size[0], size[1], tag, count_colors(img)))
        row.append(img)
        labels.append(tag)
    gap = 4
    sheet = Image.new('RGB', (len(row) * (size[0] + gap) + gap, size[1] + gap * 2), BG)
    for i, im in enumerate(row):
        sheet.paste(im, (gap + i * (size[0] + gap), gap))
    sheet.resize((sheet.width * 4, sheet.height * 4), Image.NEAREST).save(
        os.path.join(out_dir, 'compare_brightness_x4.png'))
    print('-> %s' % out_dir)


if __name__ == '__main__':
    main()
