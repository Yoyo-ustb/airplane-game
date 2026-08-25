@echo off
REM Build game.cpp into WebAssembly (game.js + game.wasm)
REM Requires Emscripten (emcc on PATH).

emcc game.cpp -O2 -o game.js -s MODULARIZE=1 -s EXPORT_NAME=createGameModule -s "EXPORTED_FUNCTIONS=['_wasm_init','_wasm_move','_wasm_fire','_wasm_update','_wasm_get_canvas','_wasm_get_score','_wasm_get_hits','_wasm_get_shots','_wasm_get_frames','_wasm_get_alive_enemies','_wasm_get_enemy_total','_wasm_is_over','_wasm_get_width','_wasm_get_height','_malloc','_free']" -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','HEAPU8']" -s ALLOW_MEMORY_GROWTH=1

if %errorlevel% neq 0 (
  echo.
  echo [FAILED] Compile error. Make sure Emscripten is installed and emcc works.
  exit /b %errorlevel%
)

echo.
echo [OK] Generated game.js and game.wasm
echo Next: start a local server and open the page
echo     python -m http.server 8000
echo     then open http://localhost:8000
