
clang++ code/space_shooter.cpp ^
    -o bin/game.exe ^
    -std=c++11 ^
    -Wall -Wextra ^
    -I libs/include ^
    libs/win-x64/minifb.lib ^
    -lkernel32 -luser32 -lshell32 -ldxgi -ld3d11 -lole32 -lgdi32 -lwinmm -lopengl32 ^
    -fsanitize=address,undefined
