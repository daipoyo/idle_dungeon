# -*- coding: utf-8 -*-
"""QRコードを文字マップにする（時計の画面に描くため）。

支援ページのURLのように内容が決まっているものは、ここで作って
src/c/qr_data.h に書き出しておく。時計側は並べて塗るだけで済む。

'K' = 黒いマス、'W' = 白いマス。読み取りに必要な余白（4マス）も含める。
"""

QUIET = 4   # 規格が求める余白（マス）


def qr_rows(data, quiet=QUIET):
    """data を QR にして、'K'/'W' の行の並びで返す。"""
    import qrcode   # 生成時だけ必要（時計側では使わない）

    q = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_L, box_size=1, border=quiet)
    q.add_data(data)
    q.make(fit=True)
    matrix = q.get_matrix()   # True = 黒
    return [''.join('K' if cell else 'W' for cell in row) for row in matrix], q.version
