# ccleste-DS

![screenshot](https://raw.githubusercontent.com/nachoz12341/ccleste-ds/master/screenshot.png)

This is a C source port of the [original celeste (Celeste classic)](https://www.lexaloffle.com/bbs/?tid=2145) for the PICO-8, designed to be portable. This is a port to the Nintendo Ds the original source can be found [here](https://github.com/lemon-sherbet/ccleste)

Go to [the releases tab](https://github.com/nachoz12341/ccleste-ds/releases) for the latest pre-built binaries.


celeste.c + celeste.h is where the game code is, translated from the pico 8 lua code by hand.
These files don't depend on anything other than the c standard library and don't perform any allocations (it uses its own internal global state).

template.c provides a "frontend" written in gl2d and MaxMod9 which implements graphics and audio output. It can be compiled on unix-like platforms by running
```
make    # this will generate ccleste-ds.nds
```
You will need devkitPro installed

# Controls

|NDS                |Action              |
|:-----------------:|-------------------:|
|LEFT               | Move left          |
|RIGHT              | Move right         |
|DOWN               | Look down          |
|UP                 | Look up            |
|A                  | Jump               |
|B                  | Dash               |
|START              | Pause              |
|Y                  | Toggle screenshake |
|L                  | Load state         |
|R                  | Save state         |
|Hold SELECT+START+Y| Reset              |
|SELECT             | Fullscreen         |

# todo
DONE    Add different scaling modes
DONE    textures are scaled slightly wrong
sprites not reflecting palette changes
pause draws nothing
stream music
randomize
pretty load screen
multiple sounds at once


# credits

Sound wave files are taken from [https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data](https://github.com/JeffRuLz/Celeste-Classic-GBA/tree/master/maxmod_data),
music ogg files were obtained by converting the .wav dumps from pico 8, which I did using audacity & ffmpeg.

All credit for the original game goes to the original developers (Maddy Thorson & Noel Berry).