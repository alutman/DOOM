DOOM
====
Port for mingw64 on Windows using SDL.

See the [original readme](README.TXT)

### Build
* Download SDL2 development Kit/
* Unpack and place kit in `lib/`
* Run `cmake --build cmake-build-debug --target DOOM`


### Run
Run `DOOM.exe`.

Reads `DOOMWADDIR` env var to search for WADs and `default.cfg`. Defaults to the current directory.


### Changes
The original source required a few changes outside the `I_` implementation files.

* Enabled the `NORAMLUNIX` compile blocks (wad dir reading and key input stuff).
* Added/removed some imports here and there.
* Use `sizeof` for a few `Z_Malloc` calls instead of hardcoded int values.
* Removed some byte alignment stuff with `Z_Malloc` as this broke outside the debugger.

#### Implementations
* `i_net_win.c` : Empty implementation that only supports singleplayer.
* `i_sound_win.c` : Empty implementation.
* `i_video_sdl.c` : Implemented screen drawing and input gathering using SDL.


#### Build
* Added a basic `CMakeLists.txt` to include `linuxdoom` and `SDL` together. `ipx`, `sersrc` and `sndserv` are excluded.
* Excluded the unix implementations of the `I_` functions.


