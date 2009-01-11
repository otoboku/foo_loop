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

foobar2000 �p�̃��[�v�Đ��}�l�[�W���ł��B

�T�|�[�g���Ă���t�H�[�}�b�g:
  * �g���g���� SLI:
      �^�C�v��: sli
      �T�|�[�g���Ă���@�\: ���x���ƃ����N
  * LoopStart/LoopLength:
      �^�C�v��: loopstartlength
      ����: ���^���� LOOPSTART �� LOOPLENGTH ���g���ă��[�v�Đ����܂��B
      �̗p��: �ŋ߂� FALCOM �Q�[���ȂǁB
  * Two Files:
      �^�C�v��: twofiles
      ����: ���(���ƃ��[�v����)�t�@�C�����g���ă��[�v�Đ����܂��B
      �̗p��: FORTUNE ARTERIAL (AUGUST) �� To Heart 2 (Leaf/AQUAPLUS) �ȂǁB
  * Wave(RIFF) Sampler:
      �^�C�v��: sampler
      ����: RIFF smpl �`�����N�̏����g���ă��[�v�Đ����܂��B
             smpl �`�����N�������Ă��t�@�C���̌��̕��ɂ���̂ŁA
             ���[�J���t�@�C���݂̂̃T�|�[�g�Ƃ��܂����B
      �̗p��: �퍑�����X�ȂǁB

�g����:
  SLI: .sli �����̂܂� foobar2000 �Ƀh���b�v���Ă��������B
  LoopStart/LoopLength: �󂩁A�������� "type=loopstartlength" �Ə������t�@�C����
                        [���y�t�@�C����].loop �Ƃ������O�ŕۑ����A������h���b�v���Ă��������B
  Wave(RIFF) Sampler: �󂩁A�������� "type=sampler" �Ə������t�@�C����
                      [���y�t�@�C����].loop �Ƃ������O�ŕۑ����A������h���b�v���Ă��������B
  Two Files: �܂��A�t�@�C���������ʕ����ƈႤ�����ɕ����܂��B
             [����][head�ŗL].[�g���q] / [����][body�ŗL].[�g���q] �Ƃ���ƁA
             "type=twofiles head-suffix=[head�ŗL] body-suffix=[body�ŗL]" �Ƃ������e�̃t�@�C����
             �쐬���A[����].[�g���q].loop �Ƃ������O�ŕۑ����܂��B
             foo.ogg.loop �̓��e�� "type=twofiles head-suffix=_a body-suffix=_b" �̂Ƃ��A
             foo_a.ogg �� foo_b.ogg ���g���ă��[�v�Đ����܂��B
             foo_.ogg.loop �̓��e�� "type=twofiles head-suffix=a body-suffix=b" �ƂȂ��Ă��Ă������ł��B
  
      