Downwell

実行
Run\Downwell.exeを起動してください。
タイトル画面でSpaceキーを押すと開始します。
←／→キーまたはA／Dキーで移動、Spaceキーでジャンプします。短く押すと低く、長く押すと高く跳びます。
空中でSpaceキーを押し直すと下へ発射し、そのまま押すと連射します。跳ぶための長押しでは発射しません。
発射すると落下が弱まり、少し上へ押し戻されます。弾は10発で、着地・敵の踏み付け・被弾で補充されます。
発射・踏み付け・被弾の時は画面が短く揺れます。
弾の命中・踏み付けは青白い光、敵の消滅は小さな爆発で表示します。着地・ジャンプ・ブロック破壊では砂ぼこりが出ます。
体力は4で、被弾ごとに1減ります。左上のハートが体力、右側の縦のゲージが残弾です。
弾は破壊可能ブロックを一発で壊し、固定ブロックで止まります。
弾は青い細長い画像で表示します。
壁に触れているとき、壁から離れる方向とSpaceキーを押すと斜め上へ壁ジャンプします。高さは通常の約半分で、横への勢いが少し残ります。
枠形のブロックは下から頭で壊せます。岩のブロックは壊れません。
移動方向を押したままジャンプから着地すると、わずかに跳ねます。
左右に移動しながら跳ぶと、着地するまで主人公が回転します。
カエルとコウモリは上から踏むと倒せます。横から触れると赤く点滅し、押し戻されます。
スライムは踏めず、弾一発で倒せます。足場の側面や下面も進み、支えるブロックがなくなると落下します。
カエルは弾をすべて防ぎ、踏み付けで倒せます。各個体が4～6秒ずつ進んで休み、壁や足場の端で折り返します。
コウモリは弾三発で倒せ、当たると下へ押し戻されます。慣性を残して主人公を追い、天井に当たると一マス分押し戻されます。
終点の下を抜けると画面が上から消え、新しいマップの上側から再び落下します。
体力が0になると主人公が倒れ、1秒間点滅して消えます。その後、画面が消えて結果画面に進みます。Spaceキーでタイトルへ戻ります。
新しく始めると体力4・弾10発に戻ります。通常の進行では体力を引き継ぎます。
Escキー、またはウィンドウの閉じるボタンで終了します。

Xboxコントローラー（XInput対応）
左スティックまたは十字キーの左右で移動します。スティック中央付近では移動しません。
AボタンはSpaceキーと同じ操作です。開始・ジャンプ・空中で押し直して射撃・結果画面からタイトルへ戻る操作ができます。
ビューボタン＋メニューボタン（Xbox 360ではBACK＋START）の同時押しで終了します。
キーボードと併用できます。起動後の接続・再接続は約1秒以内に認識します。
別のウィンドウを操作している間は入力を受け付けません。

ビルド
Visual Studio 2022の「C++によるデスクトップ開発」とWindows SDKが必要です。
Project1.slnを開き、DebugまたはRelease、x64またはx86を選択してビルドしてください。
DXライブラリのヘッダーとライブラリはthird_party\DxLibに含まれています。
Debugは/MTd、Releaseは/MTを使用します。

マップ編集
Data\stage_chunksのCSVを編集してください。各行は9列で、0は空白、1は固定、2は破壊可能です。
start.csvは開始区画、end.csvは終了区画です。他の10個から毎回5個を選びます。
行を増減すると区画の高さが変わります。保存後、ゲームを起動し直すと反映されます。

使用ライブラリ
DXライブラリ：https://dxlib.xsrv.jp/
著作権等の詳細はthird_party\DxLib\DxLib.txtとRun\DxLib-Notice.txtを参照してください。

音楽・効果音
BGM：「Bitwise Chase」／制作：林 永泰（オリジナル楽曲）
効果音：mOsh「ULTIMATE 8BIT SFX LIBRARY VOL 1」（CC0）
配布元：https://ef9.itch.io/ultimate-8bit-sfx-library-vol-1
詳細はData\audio\music\LICENSE.txtとData\audio\effects\LICENSE.txtを参照してください。

画像素材
弾：davidaf3「Set of pixel art projectiles」（CC0）
https://opengameart.org/content/set-of-pixel-art-projectiles
主人公・UI：loveOS「Gloomy Knight」（CC0）
https://loveosstudio.itch.io/gloomy-knight-16x16
地形：Adam Saltsman「Monochrome Caves」（Public Domain）
https://adamatomic.itch.io/mc-caves
敵（コウモリ・カエル）：Luis Zuno（ansimuz）「Chibi Monsters Set 01」（CC BY 3.0）
https://opengameart.org/content/chibi-monsters-set-01
敵（スライム）：Luis Zuno（ansimuz）「Grotto Escape Pack / Platform Pixel Art Assets」（CC0）
https://opengameart.org/content/platform-pixel-art-assets
命中・踏み付け：Luis Zuno（ansimuz）「Warped Shooting Fx」（CC0）
https://ansimuz.itch.io/warped-shooting-fx
敵の消滅：Luis Zuno（ansimuz）「Explosion Animations Pack」（CC0）
https://ansimuz.itch.io/explosion-animations-pack
砂ぼこり：Spring Spring「Dust Animation」（CC0）、PNG化：2D_PlatformerGuy
https://opengameart.org/content/dust-animation
詳細はData\sprites内の各LICENSE.txtを参照してください。
