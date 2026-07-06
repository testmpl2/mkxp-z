==============================================
Bitmapクラスの拡張 (DLL版)
　　　　　　Ver 0.2.0.0
　　　　http://www.tktkgame.com/


  DLL version 0.2.0.0
  Script Version 0.1.2.6

==============================================
■ 注意
ver 0.1.2.5で試験的に導入した機能
・Bitmap.newで大きいサイズBitmap.new(5000,5000)等も指定できるように
ですが、非公開のオブジェクト情報を推測で弄ってる為結構危険です。
なので不要な方は
LARGE_BITMAP = false
としてこの機能をOFFにしてください
==============================================
・概要
 ・RPGツクールXP/VX/VXA共用
 ・Marshaldump可能に
 ・Bitmap#png_save(filename [,level [,filter]])
 　　PNGファイルとして保存
 ・Bitmap#change_tone(red, green, blue [,simplify])
 　　適当色調変更。


・使用方法
　・「hn_rg_bitmap.dll」をゲームと同じフォルダにコピーします
　・「hn_rg_bitmap.txt」の内容をスクリプトのmainの前あたりに挿入します

　・スクリプト追加命令
・PNG形式で保存
Bitmap#png_save(filename [,level [,filter]])
 String filename: 保存するファイルの名前
 Fixnum level:    圧縮レベルです。level の有効な値は 0(無圧縮)～9（最大圧縮）
                  デフォルトは　9
 Fixnum filter: 　圧縮処理に使うフィルタ（デフォルト：PNG_NO_FILTERS）
                　フィルタなしの方が圧縮率が高い事が多い気がします

・ぼかし効果
Bitmap#blur2(r)
 Fixnum r: ぼかし強度 1～1000


・色調変更
Bitmap#change_tone(red, green, blue [,simplify])
 Fixnum red,green,blue:     各色のカラーバランス？-256～256の範囲の整数を指定してください
 Fixnum simplify: αチャンネルが０（完全透明）のときに処理をしない。0以外のときON(デフォルト:1)

・マスクによる画像の切り抜き（アルファとの乗算
Bitmap#clip_mask(mask, x, y [,outer])
 Bitmap mask : 切り抜きに使うマスク画像
 Fixnum x, y : マスクの位置
 Fixnum outer: マスク外の不透明度

・色の反転
Bitmap#invert()

・モザイク効果(画像全体・領域指定)
Bitmap#mosaic([msw, msh])
Bitmap#mosaic_rect(rect [, msw, msh])
 Fixnum msw, msh : モザイクのブロックの幅及び高さ
 Rect   rect: モザイクをかける領域

・ブレンディング（※下地は透過なし画像にのみ対応）
Bitmap#blend_blt(x, y, src_bitmap, src_rect, blend_type, opacity)
 Fixnum x, y       : 合成先の座標
 Bitmap src_bitmap : 合成に使用するするフィルタのBitmapオブジェクト
 Rect   src_rect   : フィルタの転送元矩形
 Fixnum blend_type : 合成タイプ（0～7）
 Fixnum opacity    : 不透明度（0～255）※通常、加算、減算でのみ使用
　合成タイプ
　　 0: 通常(bltと同じ)
　　 1: 加算
　　 2: 減算
　　 3: 乗算
　　 4: 覆い焼き（ハイライト）
　　 5: 焼きこみ
　　 6: スクリーン
　　 7: オーバーレイ

===============================

・開発環境
　・VisualStudio2008
　・WindowsXP sp3

・更新履歴
 2011/12/20 ver 0.2.0.0
　VX Aceに対応
 2010/12/13 ver 0.1.2.6
　dllの名称を"hn_rg_bitmap.dll"から"tktk_bitmap.dll"に変更
　LARGE_BITMAP機能でメモリを確保できなかった場合の処理を追加
 2010/10/12 ver 0.1.2.5(デンジャラスベータ版)
　大きいサイズのBitmapオブジェクトを機能を試験的に実装（危険）
 2010/07/14 ver 0.1.2.4
　libpngライブラリを最新のバージョンにＵＰ
 2010/03/29 ver 0.1.2.2
　ブレンディング機能をかすかに軽量化
 2010/03/24 ver 0.1.2.1
　ブレンディング機能関連のバグフィックス
 2010/03/22 ver 0.1.2.0
　加算合成等のブレンディング機能の追加
 2010/02/08 ver 0.1.1.0
　マーシャル化の処理の一部をDLLに移動
 2010/01/17 ver 0.1.0.0
 　dllの名称を"hn_rx_bitmap.dll"から"hn_rg_bitmap.dll"に変更
 　切り抜き効果・モザイク効果・色反転・ぼかし効果の追加
 2010/01/14 var 0.0.3
 　バグFix: png保存時に画像が一列下にずれてしまう不具合を修正
 2009/07/21 var 0.0.2
 　バグFix
 2009/07/21 0.0.1a
 　PNG圧縮系のオプションが適用されていなかったのを修正（スクリプトのみ）
 2009/05/27 ver 0.0.1
 　DLLを使用するバージョンの公開(hn_rx_bitmap)
 2009/03/?? 非DLL版
 　RGSSのみのバージョン公開


-------------------------------------------------------------
　tktk_bitmap.dllでは、zlib/libpngライブラリを使用させていただきました。
　ありがとうございます。

zlib version 1.2.5, April 19th, 2010
 Copyright (C) 1995-2010 Jean-loup Gailly and Mark Adler

libpng version 1.4.5 - December 9, 2010
 Copyright (c) 1998-2010 Glenn Randers-Pehrson
 (version 0.96) Copyright (c) 1996, 1997 Andreas Dilger
 (version 0.88) Copyright (c) 1995, 1996 Guy Eric, Schalnat, Group 42, Inc.

The zlib/libpng License
http://opensource.org/licenses/zlib-license.php

zlib/libpngライセンス（日本語訳）
http://sourceforge.jp/projects/opensource/wiki/licenses%2Fzlib_libpng_license

-------------------------------------------------------------

　RPGTKOOLXP/RGSS Wiki様のBitmapのMarshal対応スクリプトを参考にさせていただきました。 

RPGTKOOLXP/RGSS Wiki
http://tkool.web-ghost.net/wiki/wiki.cgi?page=FrontPage

-------------------------------------------------------------
