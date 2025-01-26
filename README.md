# さんぱくん外出用BonDriver  

ベース：2016-02-19, Ver. 0.1.3  

## 【改造箇所】  
１．Visual C++ 2022 にてソリューション作り直し  
２．各種パラメータ、チャンネル情報をiniファイル化  
３．ビルド時のワーニング除去  
４．内蔵カードリーダーを使用可能にした  
５．細かいバグ修正  

## 【Windows バージョン】  
無改造：USB Isochronous転送  
改造  ：USB Bulk転送  
USB Isochronous転送はWindows8.1 以上で動作するので  
それに合わせてビルドしている  
Windows8.1 以上で使用すること  

## 【チューニングスペース】  
BS/CSのみ使用、またはCATVを使用する等  
チューニングスペースを動的に設定可能としている  
詳細はiniファイル参照  
