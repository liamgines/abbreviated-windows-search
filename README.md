file-search
========
A command-line file search utility for Windows.

![Screenshot](screenshot.png)

Installation
--------
With Visual Studio installed, you can run:

	vcvarsall.bat x64

Then compile with:

	cl /ZI main.cpp sqlite3.c Shell32.lib /Fe:fs.exe

Finally, add the executable to your PATH.

Usage
--------
First, run `fs` without any arguments to create an index of system files.

	fs

Then run `fs` with a filename to find where a file might be. Note that the filename doesn't have to be an exact match.

	fs <filename>

Attributions
--------
1. `mftreader.hpp` (modified for this project) derived from [Tutorial/Parsing the MFT](https://handmade.network/forums/articles/t/7002-tutorial_parsing_the_mft) by Nakst, [The Unlicense](https://unlicense.org/)
2. `stb_ds.h` from [stb](https://github.com/nothings/stb) by Sean Barrett, [The Unlicense](https://unlicense.org/)
3. `sqlite3.c` and `sqlite3.h` from [SQLite](https://sqlite.org/) by D. Richard Hipp and other developers, [Public Domain](https://sqlite.org/copyright.html)
