mkdir -p bin

clang++ code/space_shooter.cpp \
    -o bin/game \
    -std=c++11 \
    -Wall -Wextra -Werror -Wno-error=unused-parameter \
    -fsanitize=address,undefined \
    -I libs/include \
    libs/mac-x64/libminifb.a \
    -framework Cocoa \
    -framework Metal \
    -framework MetalKit \
    -framework AudioToolbox \
    -g \
    -fcolor-diagnostics \
    -fansi-escape-codes
