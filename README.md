# Downwell

現在のゲームソース・素材・ビルド設定だけを保存した最小構成です。
Visual Studio 2022 の C++ デスクトップ開発と Windows SDK を用意し、次を実行します。

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Configuration Release -Platform x64
```

初回は `setup-dxlib.ps1` が公式 DxLib 3.25a を取得し、固定 SHA256 を検証します。
実行ファイルは `build/x64/Release/Downwell.exe` です。

ビルド済み実行ファイル、再取得できるライブラリ、テスト、開発履歴、キャッシュは含めていません。
`README.txt` に操作と素材の著作権表記があります。同ファイルの `Run/Downwell.exe` は元の配布版のパスです。
