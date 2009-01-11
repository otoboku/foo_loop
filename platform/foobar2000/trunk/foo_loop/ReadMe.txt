Loop Manager for foobar2000.

supported format:
  * kirikiri's SLI:
      typename: sli
      supported features: label and link.
      description: kirikiri's seemless? loop information file.
  * LoopStart/LoopLength:
      typename: loopstartlength
      description: looping with LOOPSTART and LOOPLENGTH metainfo.
      example: recently FALCOM games.
  * Two Files:
      typename: twofiles
      description: looping with two (head and body) files.
      example: FORTUNE ARTERIAL (AUGUST), To Heart 2 (Leaf/AQUAPLUS), and so on.
  * Wave(RIFF) Sampler:
      typename: sampler
      description: looping with RIFF smpl chunk. (local file only)
      example: SENGOKU RANCE, and so on.
      
HOW TO USE:
  SLI: drop .sli to foobar2000.
  LoopStart/LoopLength:
      create blank (or with "type=loopstartlength") file, as [musicfile].loop.
      ex. ED6563.ogg and ED6563.ogg.loop
  Two Files:
      if you create foo.ogg.loop with "type=twofiles head-suffix=_a body-suffix=_b",
      attempt to use foo_a.ogg and foo_b.ogg.
  Wave(RIFF) Sampler:
      create blank (or with "type=sampler") file, as [musicfile].loop.
      ex. foo.wav and foo.wav.loop



----

foobar2000 用のループ再生マネージャです。

サポートしているフォーマット:
  * 吉里吉里の SLI:
      タイプ名: sli
      サポートしている機能: ラベルとリンク
  * LoopStart/LoopLength:
      タイプ名: loopstartlength
      説明: メタ情報の LOOPSTART と LOOPLENGTH を使ってループ再生します。
      採用例: 最近の FALCOM ゲームなど。
  * Two Files:
      タイプ名: twofiles
      説明: 二つの(頭とループ部分)ファイルを使ってループ再生します。
      採用例: FORTUNE ARTERIAL (AUGUST) や To Heart 2 (Leaf/AQUAPLUS) など。
  * Wave(RIFF) Sampler:
      タイプ名: sampler
      説明: RIFF smpl チャンクの情報を使ってループ再生します。
             smpl チャンクがたいていファイルの後ろの方にあるので、
             ローカルファイルのみのサポートとしました。
      採用例: 戦国ランスなど。

使い方:
  SLI: .sli をそのまま foobar2000 にドロップしてください。
  LoopStart/LoopLength: 空か、もしくは "type=loopstartlength" と書いたファイルを
                        [音楽ファイル名].loop という名前で保存し、それをドロップしてください。
  Wave(RIFF) Sampler: 空か、もしくは "type=sampler" と書いたファイルを
                      [音楽ファイル名].loop という名前で保存し、それをドロップしてください。
  Two Files: まず、ファイル名を共通部分と違う部分に分けます。
             [共通][head固有].[拡張子] / [共通][body固有].[拡張子] とすると、
             "type=twofiles head-suffix=[head固有] body-suffix=[body固有]" という内容のファイルを
             作成し、[共通].[拡張子].loop という名前で保存します。
             foo.ogg.loop の内容が "type=twofiles head-suffix=_a body-suffix=_b" のとき、
             foo_a.ogg と foo_b.ogg を使ってループ再生します。
             foo_.ogg.loop の内容が "type=twofiles head-suffix=a body-suffix=b" となっていても同じです。
  
      