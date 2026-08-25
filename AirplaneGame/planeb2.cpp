#include <iostream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <windows.h>
using namespace std;

#ifdef _WIN32
#pragma execution_character_set("gbk")
#endif
// 游戏常量定义：画布宽高、敌人上限、初始分数
const int WIDTH = 55;
const int HEIGHT = 35;
const int ENEMY_MAX = 50;
const int INITIAL_SCORE = 10;

// 玩家/敌人 宽高（像素字符数）
const int PLAYER_W = 5;
const int PLAYER_H = 3;
const int ENEMY_W = 3;
const int ENEMY_H = 2;

// 游戏画布：二维字符数组
char canvas[HEIGHT][WIDTH];

// 移动控制台光标到指定行、列
void gotoXY(int row, int col)
{
    COORD pos;
    pos.X = static_cast<SHORT>(col);  // X=列
    pos.Y = static_cast<SHORT>(row); // Y=行
    // 调用Windows API 设置光标位置
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

// 隐藏控制台光标
void hideCursor()
{
    CONSOLE_CURSOR_INFO info = { 1, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
}

// 初始化控制台：设置标题、清空屏幕
void setupConsole()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("Airplane Game"); // 设置窗口标题
    gotoXY(0, 0);                      // 光标移到左上角
}

// 玩家结构体：存储坐标
struct WJ
{
    int x;
    int y;
};

// 子弹结构体：坐标+是否来自玩家
struct Bullet
{
    int x, y;
    bool fromPlayer;
};

// 敌人结构体：坐标+射击计时器
struct Enemy
{
    int x, y;
    int shootTimer;
};

// 全局游戏变量
int score = INITIAL_SCORE;
WJ player;
vector<Bullet> bullets;  // 子弹列表
vector<Enemy> enemies;  // 敌人列表

// 在画布指定位置画一个字符
void putChar(int row, int col, char ch)
{
    // 边界判断：不超出画布才绘制
    if (row >= 0 && row < HEIGHT && col >= 0 && col < WIDTH)
        canvas[row][col] = ch;
}

// 在画布指定位置画字符串
void putString(int row, int col, const char* s)
{
    // 循环遍历字符串每个字符，逐个画到画布
    for (int i = 0; s[i] != '\0'; ++i)
        putChar(row, col + i, s[i]);
}

// 清空画布 + 绘制游戏边框
void clearCanvas()
{
    // 1. 把所有字符填充为空格
    for (int r = 0; r < HEIGHT; ++r)
        for (int c = 0; c < WIDTH; ++c)
            canvas[r][c] = ' ';

    // 2. 画上下边框
    for (int c = 0; c < WIDTH; ++c)
    {
        putChar(0, c, '-');
        putChar(HEIGHT - 1, c, '-');
    }
    // 3. 画左右边框
    for (int r = 0; r < HEIGHT; ++r)
    {
        putChar(r, 0, '|');
        putChar(r, WIDTH - 1, '|');
    }
}

// 绘制玩家飞机
void drawPlayer(int px, int py)
{
    putString(px,     py, " /=\\");  // 第一行
    putString(px + 1, py, "<(*)>"); // 第二行
    putString(px + 2, py, " * *");  // 第三行
}

// 绘制敌人飞机
void drawEnemy(int ex, int ey)
{
    putString(ex,     ey, "\\+/");  // 第一行
    putString(ex + 1, ey, " | ");   // 第二行
}

// 绘制顶部状态栏（分数、敌人数量、操作提示）
void drawHUD()
{
    char hud[WIDTH - 2];
    // 格式化字符串：分数、敌人数量、操作说明
    snprintf(hud, sizeof(hud), "Score:%d Enemy:%d/%d  WASD移动 空格射击 Q退出",
             score, (int)enemies.size(), ENEMY_MAX);
    putString(1, 2, hud);
}

// 把画布内容输出到屏幕
void present()
{
    gotoXY(0, 0); // 光标回到左上角，覆盖重绘
    // 逐行输出画布内容
    for (int r = 0; r < HEIGHT; ++r)
    {
        cout.write(canvas[r], WIDTH);
        if (r != HEIGHT - 1)
            cout << "\r\n";
    }
    cout.flush(); // 强制刷新输出
}

// 判断坐标是否在游戏区域内（不含边框）
bool inBounds(int row, int col)
{
    return row >= 1 && row < HEIGHT - 1 && col >= 1 && col < WIDTH - 1;
}

// 判断玩家是否可以移动到目标位置（防止出界）
bool playerCanMoveTo(int row, int col)
{
    return row >= 1 && row <= HEIGHT - 1 - PLAYER_H
        && col >= 1 && col <= WIDTH - 1 - PLAYER_W;
}

// 随机生成敌人
void spawnEnemy()
{
    // 敌人数量达到上限，不再生成
    if ((int)enemies.size() >= ENEMY_MAX)
        return;
    Enemy e;
    // 随机生成敌人坐标
    e.x = 1 + rand() % (HEIGHT - 10);
    e.y = 1 + rand() % (WIDTH - 1 - ENEMY_W);
    e.shootTimer = rand() % 40; // 随机射击倒计时
    enemies.push_back(e);      // 加入敌人列表
}

// 碰撞检测：点是否在矩形内
bool pointInRect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rh && py >= ry && py < ry + rw;
}

// 判断敌人子弹是否击中玩家
bool bulletHitsPlayer(const Bullet& b)
{
    return !b.fromPlayer && pointInRect(b.x, b.y, player.x, player.y, PLAYER_W, PLAYER_H);
}

// 判断玩家子弹是否击中敌人
bool bulletHitsEnemy(const Bullet& b, const Enemy& e)
{
    return b.fromPlayer && pointInRect(b.x, b.y, e.x, e.y, ENEMY_W, ENEMY_H);
}

// 渲染所有游戏元素
void render()
{
    clearCanvas();    // 清空画布
    drawHUD();        // 画状态栏

    // 绘制所有子弹
    for (auto& b : bullets)
        putChar(b.x, b.y, b.fromPlayer ? '^' : 'v');

    // 绘制所有敌人
    for (auto& e : enemies)
        drawEnemy(e.x, e.y);

    drawPlayer(player.x, player.y); // 画玩家
    present();                      // 输出到屏幕
}

// 处理键盘输入
void handleInput()
{
    if (!_kbhit()) // 没有按键按下，直接返回
        return;

    char key = _getch(); // 获取按键
    if (key == ' ')      // 空格：发射玩家子弹
    {
        Bullet b;
        b.x = player.x - 1;
        b.y = player.y + PLAYER_W / 2;
        b.fromPlayer = true;
        if (inBounds(b.x, b.y))
            bullets.push_back(b);
        return;
    }

    // WASD 移动：计算目标坐标
    int nx = player.x;
    int ny = player.y;
    if (key == 'w' || key == 'W') nx--;    // 上
    else if (key == 's' || key == 'S') nx++; // 下
    else if (key == 'a' || key == 'A') ny--; // 左
    else if (key == 'd' || key == 'D') ny++; // 右
    else if (key == 'q' || key == 'Q')      // Q：退出游戏
    {
        score = 0;
        return;
    }
    else
        return;

    // 可以移动则更新玩家坐标
    if (playerCanMoveTo(nx, ny))
    {
        player.x = nx;
        player.y = ny;
    }
}

// 更新子弹位置 + 移除出界子弹
void updateBullets()
{
    for (size_t i = 0; i < bullets.size(); )
    {
        if (bullets[i].fromPlayer)
            bullets[i].x--; // 玩家子弹向上飞
        else
            bullets[i].x++; // 敌人子弹向下飞

        // 出界则删除子弹
        if (!inBounds(bullets[i].x, bullets[i].y))
            bullets.erase(bullets.begin() + i);
        else
            ++i;
    }
}

// 更新敌人逻辑：移动、射击、生成
void updateEnemies(int frame)
{
    if (frame % 25 == 0)  // 每25帧生成一个敌人
        spawnEnemy();

    for (size_t i = 0; i < enemies.size(); )
    {
        if (frame % 20 == 0) // 每20帧敌人向下移动
            enemies[i].x++;

        enemies[i].shootTimer--; // 射击倒计时
        if (enemies[i].shootTimer <= 0) // 倒计时结束，发射子弹
        {
            Bullet b;
            b.x = enemies[i].x + ENEMY_H;
            b.y = enemies[i].y + ENEMY_W / 2;
            b.fromPlayer = false;
            if (inBounds(b.x, b.y))
                bullets.push_back(b);
            enemies[i].shootTimer = 30 + rand() % 40; // 重置倒计时
        }

        // 敌人到达底部，删除
        if (enemies[i].x >= HEIGHT - 2)
            enemies.erase(enemies.begin() + i);
        else
            ++i;
    }
}

// 碰撞检测：子弹打敌人、子弹打玩家
void checkCollisions()
{
    // 玩家子弹击中敌人
    for (size_t bi = 0; bi < bullets.size(); )
    {
        bool removed = false;
        for (size_t ei = 0; ei < enemies.size(); ++ei)
        {
            if (bulletHitsEnemy(bullets[bi], enemies[ei]))
            {
                enemies.erase(enemies.begin() + ei); // 删除敌人
                bullets.erase(bullets.begin() + bi); // 删除子弹
                score++;                            // 加分
                removed = true;
                break;
            }
        }
        if (!removed)
            ++bi;
    }

    // 敌人子弹击中玩家
    for (size_t bi = 0; bi < bullets.size(); )
    {
        if (bulletHitsPlayer(bullets[bi]))
        {
            bullets.erase(bullets.begin() + bi); // 删除子弹
            score--;                            // 减分
        }
        else
            ++bi;
    }
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr))); // 随机数种子

    setupConsole();  // 初始化控制台
    hideCursor();    // 隐藏光标

    // 玩家初始位置：底部中间
    player.x = HEIGHT - 1 - PLAYER_H;
    player.y = (WIDTH - PLAYER_W) / 2;

    int frame = 0;
    // 游戏主循环：分数>0就一直运行
    while (score > 0)
    {
        handleInput();    // 处理输入
        updateBullets();  // 更新子弹
        updateEnemies(frame); // 更新敌人
        checkCollisions();// 碰撞检测
        render();         // 渲染画面

        Sleep(40);  // 控制帧率：约25帧/秒
        frame++;    // 帧计数+1
    }

    // 游戏结束
    gotoXY(HEIGHT / 2, WIDTH / 2 - 8);
    cout << "Game Over! Score: " << score;
    gotoXY(HEIGHT / 2 + 1, WIDTH / 2 - 8);
    cout << "按任意键退出...";
    _getch();

    // 恢复光标
    CONSOLE_CURSOR_INFO info = { 1, TRUE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return 0;
}