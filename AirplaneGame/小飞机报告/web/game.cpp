// ============================================================
//  game.cpp  —— 飞机大战 Web 版核心逻辑（WebAssembly）
//
//  本文件保留了 planeb3.cpp 的全部游戏逻辑（战场、玩家、敌机、
//  子弹、碰撞、计分），但去掉了 Windows 专属的 conio.h / windows.h，
//  并把“主循环 + 控制台绘制”拆成可以被 HTML/JavaScript 调用的接口。
//
//  浏览器无法直接运行 C++，所以这里用 Emscripten 把它编译成
//  WebAssembly(.wasm)，再由 index.html 加载并调用下面这些函数。
// ============================================================

#include <vector>
#include <cstdlib>
#include <cstring>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace std;

// ---------- 游戏常量----------
const int WIDTH = 55;
const int HEIGHT = 35;
const int ENEMY_MAX = 50;
const int INITIAL_SCORE = 10;

const int PLAYER_W = 5;
const int PLAYER_H = 3;
const int ENEMY_W = 3;
const int ENEMY_H = 2;

// 战场画布：每个格子一个字符
char canvas[HEIGHT][WIDTH];
// 提供给 JS 读取的一维缓冲（行优先展开）
char flatBuffer[HEIGHT * WIDTH + 1];

// ---------- 游戏对象 ----------
struct WJ { int x, y; };
struct Bullet { int x, y; bool fromPlayer; };
struct Enemy { int x, y; int shootTimer; };

int score = INITIAL_SCORE;
WJ player;
vector<Bullet> bullets;
vector<Enemy> enemies;

// ---------- 成就 / 统计数据 ----------
int hits = 0;        // 累计击落敌机数（“打中多少”）
int shots = 0;       // 累计发射子弹数
int frames = 0;      // 存活帧数
int gameOver = 0;    // 是否结束

// ---------- 画布 ----------
void putChar(int row, int col, char ch)
{
    if (row >= 0 && row < HEIGHT && col >= 0 && col < WIDTH)
        canvas[row][col] = ch;
}

void putString(int row, int col, const char* s)
{
    for (int i = 0; s[i] != '\0'; ++i)
        putChar(row, col + i, s[i]);
}

void clearCanvas()
{
    for (int r = 0; r < HEIGHT; ++r)
        for (int c = 0; c < WIDTH; ++c)
            canvas[r][c] = ' ';

    // 边框
    for (int c = 0; c < WIDTH; ++c)
    {
        putChar(0, c, '-');
        putChar(HEIGHT - 1, c, '-');
    }
    for (int r = 0; r < HEIGHT; ++r)
    {
        putChar(r, 0, '|');
        putChar(r, WIDTH - 1, '|');
    }
}

// ---------- 飞机造型 ----------
void drawPlayer(int px, int py)
{
    putString(px, py, " /=\\");
    putString(px + 1, py, "<(*)>");
    putString(px + 2, py, " * *");
}

void drawEnemy(int ex, int ey)
{
    putString(ex, ey, "\\+/");
    putString(ex + 1, ey, " | ");
}

// ---------- 边界与碰撞检测 ----------
bool inBounds(int row, int col)
{
    return row >= 1 && row < HEIGHT - 1 && col >= 1 && col < WIDTH - 1;
}

bool playerCanMoveTo(int row, int col)
{
    return row >= 1 && row <= HEIGHT - 1 - PLAYER_H
        && col >= 1 && col <= WIDTH - 1 - PLAYER_W;
}

bool pointInRect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rh && py >= ry && py < ry + rw;
}

bool bulletHitsPlayer(const Bullet& b)
{
    return !b.fromPlayer && pointInRect(b.x, b.y, player.x, player.y, PLAYER_W, PLAYER_H);
}

bool bulletHitsEnemy(const Bullet& b, const Enemy& e)
{
    return b.fromPlayer && pointInRect(b.x, b.y, e.x, e.y, ENEMY_W, ENEMY_H);
}

// ---------- 游戏逻辑 ----------
void spawnEnemy()
{
    if ((int)enemies.size() >= ENEMY_MAX)
        return;
    Enemy e;
    e.x = 1 + rand() % (HEIGHT - 10);
    e.y = 1 + rand() % (WIDTH - 1 - ENEMY_W);
    e.shootTimer = rand() % 40;
    enemies.push_back(e);
}

void updateBullets()
{
    for (size_t i = 0; i < bullets.size(); )
    {
        bullets[i].fromPlayer ? bullets[i].x-- : bullets[i].x++;

        if (!inBounds(bullets[i].x, bullets[i].y))
            bullets.erase(bullets.begin() + i);
        else
            ++i;
    }
}

void updateEnemies(int frame)
{
    if (frame % 25 == 0)
        spawnEnemy();

    for (size_t i = 0; i < enemies.size(); )
    {
        if (frame % 20 == 0)
            enemies[i].x++;

        enemies[i].shootTimer--;
        if (enemies[i].shootTimer <= 0)
        {
            Bullet b;
            b.x = enemies[i].x + ENEMY_H;
            b.y = enemies[i].y + ENEMY_W / 2;
            b.fromPlayer = false;
            if (inBounds(b.x, b.y))
                bullets.push_back(b);
            enemies[i].shootTimer = 30 + rand() % 40;
        }

        if (enemies[i].x >= HEIGHT - 2)
            enemies.erase(enemies.begin() + i);
        else
            ++i;
    }
}

void checkCollisions()
{
    // 玩家子弹击落敌机
    for (size_t bi = 0; bi < bullets.size(); )
    {
        bool removed = false;
        for (size_t ei = 0; ei < enemies.size(); ++ei)
        {
            if (bulletHitsEnemy(bullets[bi], enemies[ei]))
            {
                enemies.erase(enemies.begin() + ei);
                bullets.erase(bullets.begin() + bi);
                score++;
                hits++;          // 记录击落数
                removed = true;
                break;
            }
        }
        if (!removed) ++bi;
    }

    // 敌机子弹击中玩家
    for (size_t bi = 0; bi < bullets.size(); )
    {
        if (bulletHitsPlayer(bullets[bi]))
        {
            bullets.erase(bullets.begin() + bi);
            score--;
        }
        else ++bi;
    }
}

void render()
{
    clearCanvas();

    for (auto& b : bullets)
        putChar(b.x, b.y, b.fromPlayer ? '^' : 'v');

    for (auto& e : enemies)
        drawEnemy(e.x, e.y);

    drawPlayer(player.x, player.y);

    // 展开为一维，方便 JS 一次性读取
    for (int r = 0; r < HEIGHT; ++r)
        for (int c = 0; c < WIDTH; ++c)
            flatBuffer[r * WIDTH + c] = canvas[r][c];
    flatBuffer[HEIGHT * WIDTH] = '\0';
}

// ============================================================
//  暴露给 JavaScript 调用的接口（C 链接，避免名字被修饰）
// ============================================================
extern "C" {

// 开始 / 重新开始一局
EMSCRIPTEN_KEEPALIVE void wasm_init(unsigned seed)
{
    srand(seed);

    score = INITIAL_SCORE;
    hits = 0;
    shots = 0;
    frames = 0;
    gameOver = 0;

    bullets.clear();
    enemies.clear();

    player.x = HEIGHT - 1 - PLAYER_H;
    player.y = (WIDTH - PLAYER_W) / 2;

    render();
}

// 处理移动按键：'w''a''s''d'，或 'q' 退出
EMSCRIPTEN_KEEPALIVE void wasm_move(int key)
{
    if (gameOver) return;

    int nx = player.x;
    int ny = player.y;

    if (key == 'w' || key == 'W') nx--;
    else if (key == 's' || key == 'S') nx++;
    else if (key == 'a' || key == 'A') ny--;
    else if (key == 'd' || key == 'D') ny++;
    else if (key == 'q' || key == 'Q') { score = 0; gameOver = 1; return; }

    if (playerCanMoveTo(nx, ny))
    {
        player.x = nx;
        player.y = ny;
    }
}

// 玩家发射子弹
EMSCRIPTEN_KEEPALIVE void wasm_fire()
{
    if (gameOver) return;

    Bullet b;
    b.x = player.x - 1;
    b.y = player.y + PLAYER_W / 2;
    b.fromPlayer = true;
    if (inBounds(b.x, b.y))
    {
        bullets.push_back(b);
        shots++;
    }
}

// 推进一帧（敌机、子弹、碰撞、绘制）
EMSCRIPTEN_KEEPALIVE void wasm_update()
{
    if (gameOver) return;

    updateBullets();
    updateEnemies(frames);
    checkCollisions();
    render();

    frames++;
    if (score <= 0)
        gameOver = 1;
}

// 让 JS 读取画布缓冲区的指针
EMSCRIPTEN_KEEPALIVE const char* wasm_get_canvas() { return flatBuffer; }

EMSCRIPTEN_KEEPALIVE int wasm_get_score()         { return score; }
EMSCRIPTEN_KEEPALIVE int wasm_get_hits()          { return hits; }
EMSCRIPTEN_KEEPALIVE int wasm_get_shots()         { return shots; }
EMSCRIPTEN_KEEPALIVE int wasm_get_frames()        { return frames; }
EMSCRIPTEN_KEEPALIVE int wasm_get_alive_enemies() { return (int)enemies.size(); }
EMSCRIPTEN_KEEPALIVE int wasm_get_enemy_total()   { return ENEMY_MAX; }
EMSCRIPTEN_KEEPALIVE int wasm_is_over()           { return gameOver; }
EMSCRIPTEN_KEEPALIVE int wasm_get_width()         { return WIDTH; }
EMSCRIPTEN_KEEPALIVE int wasm_get_height()        { return HEIGHT; }

} // extern "C"
