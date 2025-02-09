#!/bin/bash

RAYLIB_PATH="/Users/grey/softwares/raylib/src"

# Need this for raygui to work
RAYLIB_INCLUDE_PATH="/Users/grey/softwares/raylib/build/raylib/include"

# Add libcurl dependency
CURL_FLAGS=`pkg-config --cflags --libs libcurl`

case "$1" in
  speed|release)
    echo "Building for release"
    gcc -O3 -march=native -s game.c -I$RAYLIB_INCLUDE_PATH `pkg-config --libs --cflags raylib` $CURL_FLAGS -o build/game_release && ./build/game_release
    ;;
    
  wasm)
    # we use -DWASM to pass a wasm flag to C
    # FETCH needed for emscripten api calls to work
    echo "Building for wasm"
    emcc -o build/wasm/game.html game.c web_game.c -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces -Wunused-result \
      -O3 -DWASM -I$RAYLIB_INCLUDE_PATH -I "$RAYLIB_PATH" -I "$RAYLIB_PATH/external" \
      -L. -L "$RAYLIB_PATH" \
      --preload-file assets -s ASYNCIFY \
      -s USE_GLFW=3 -s TOTAL_MEMORY=1073741824 -s FORCE_FILESYSTEM=1 \
      -s MAX_WEBGL_VERSION=2 -s USE_WEBGL2=1 \
      -s FETCH=1 \
      --shell-file "$RAYLIB_PATH/minshell.html" "$RAYLIB_PATH/web/libraylib.a" \
      -s 'EXPORTED_FUNCTIONS=["_free","_malloc","_main"]' -s EXPORTED_RUNTIME_METHODS=ccall
    ;;
    
  *)
    echo "Default build"
    gcc -g -O0 -Wall game.c -I$RAYLIB_INCLUDE_PATH `pkg-config --libs --cflags raylib` $CURL_FLAGS -o build/game_debug && ./build/game_debug
    ;;
esac