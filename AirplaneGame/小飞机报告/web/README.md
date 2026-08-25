# 飞机大战 · Web 版（C++ → WebAssembly）

本目录把 `planeb3.cpp` 的游戏逻辑改写为**可移植 C++**（`game.cpp`），
用 **Emscripten** 编译成 **WebAssembly**，再由 `index.html` 加载并**调用其中的 C++ 函数**。

新增功能：

- 开始界面（标题、玩法说明、开始按钮）
- 结束界面，结算成就：击落数量、命中率、存活时间、最终分数、评级与徽章

---

## 文件说明

| 文件 | 作用 |
|------|------|
| `game.cpp` | 游戏核心逻辑（C++），导出供 JS 调用的接口 |
| `index.html` | 网页前端：开始界面、战场渲染、结束结算 |
| `build.bat` | 编译脚本：`game.cpp` → `game.js` + `game.wasm` |
| `game.js` / `game.wasm` | **编译后才会生成**，由浏览器加载 |

---

## 一、安装 Emscripten（只需一次）

```powershell
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
.\emsdk install latest
.\emsdk activate latest
.\emsdk_env.ps1        # 让当前终端能用 emcc
```

验证：

```powershell
emcc --version
```

## 二、编译

在本目录（`web`）下，确保 `emcc` 可用后运行：

```powershell
.\build.bat
```

成功后会生成 `game.js` 和 `game.wasm`。

## 三、运行（必须用本地服务器，不能直接双击 html）

浏览器加载 `.wasm` 需要通过 http 协议：

```powershell
python -m http.server 8000
```

然后浏览器打开：<http://localhost:8000>

> 也可以用 Emscripten 自带的 `emrun --port 8000 index.html`。

---

## 工作原理（HTML 如何调用 C++）

1. `game.cpp` 用 `extern "C"` + `EMSCRIPTEN_KEEPALIVE` 导出函数，例如
   `wasm_init / wasm_move / wasm_fire / wasm_update / wasm_get_canvas`。
2. `emcc` 把它编译成 `game.wasm`，并生成 `game.js` 胶水代码。
3. `index.html` 通过 `createGameModule()` 加载模块，用 `Module.cwrap(...)`
   把 C++ 函数包装成 JS 函数。
4. 每帧 JS 调用 `wasm_update()` 推进 C++ 里的游戏逻辑，再从 WASM 内存里
   读取 `canvas` 字符缓冲并渲染到网页上。

游戏的核心规则（战场 55×35、玩家/敌机造型、移动、射击、碰撞、计分）
全部仍在 C++ 中执行，网页只负责输入与显示。
