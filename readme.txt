Title: minizip plugin
Author: �킽�Ȃׂ���

������͂ȂɁH

�g���g���� zip �A�[�J�C�u�������v���O�C���ł��B
�e��f�[�^�̏o�͂�Ǘ��Ȃǂɗ��p�ł��܂�

This unzip package allow creates .ZIP file, compatible with PKZip 2.04g
WinZip, InfoZip tools and compatible.
Multi volume ZipFile (span) are not supported.
Encryption compatible with pkzip 2.04g only supported
Old compressions used by old PKZip 1.x are not supported

�����C�Z���X

zlib �t���� contrib/minizip ����fork ���� nmoinvaz/minizip
(https://github.com/nmoinvaz/minizip) ���v���O�C�����������̂ł�
zlib ���C�Z���X�ɂȂ�܂��B

���r���h

�r���h�ɂ�Windows�p��cmake���K�v�ł��B

VisualStudio �̃R�}���h���C������ȉ��őΉ�

```
# win32�w�肵�Ȃ���x64�ɂȂ�ꍇ����
cmake -A win32 -B build
cmake --build build --config Release

build/Release/minizip.dll

```

���g����

manual.tjs �Q��



