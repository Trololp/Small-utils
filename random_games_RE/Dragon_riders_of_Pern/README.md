# Dragon Riders Chronicles of Pern tools
This game have dragons, but unfortunatelly we cannot fly them.

### decompression tool
To compile `tool.c` you need to obtain sources of Dynamite library: [link](https://github.com/twogood/dynamite)
tool can be used to extract one specific file at time. If you run tool in folder with files `data.pfi` and `data.pfs`
It will generate list of contents in that file and put it to `content.txt` file.
Usage: `tool -e CHARACTERS.TXT` , this will extract and decompress this file from archive.

### 3D_to_gltf.py
This script used to convert .3D files from archive to .gltf file that blender can understand.
Levels also in .3D format, so can be exported too.
