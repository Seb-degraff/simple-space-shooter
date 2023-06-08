/Users/seb/dev/engine/tools/emsdk/emsdk_env.sh # setup the environement variables and stuff

mkdir -p bin/web # create bin and web dir if it does not exist

# Build with emcc. Found most of the arguments by looking in the CMake file of MiniFB or googling.
/Users/seb/dev/engine/tools/emsdk/upstream/emscripten/emcc code/space_shooter.cpp \
    -o bin/web/game.js \
    -std=c++11 \
    -Wall -Wextra -Werror -Wno-error=unused-parameter \
    -I libs/include \
    libs/wasm/libminifb.a \
    -fcolor-diagnostics \
    -fansi-escape-codes \
    -sEXPORT_NAME=my_game \
    -sASYNCIFY \
    --preload-file assets \
    -sSTRICT=1 \
    -sENVIRONMENT=web \
    -sLLD_REPORT_UNDEFINED \
    -sMODULARIZE=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sALLOW_TABLE_GROWTH \
    -sMALLOC=emmalloc \
    -sEXPORT_ALL=1 \
    -sEXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_main\"] \
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
    -sASYNCIFY \
    --no-entry \
    -O3 \
    
#    -sSINGLE_FILE
# python3 -m http.server 8000 bin/web/
