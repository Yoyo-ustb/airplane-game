#include <iostream>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <windows.h>
using namespace std ;
const int WIDTH = 55;
const int HEIGHT = 35;
const int ENEMY_MAX = 50;
const int INITIAL_SCORE = 10;

const int PLAYER_W = 5;
const int PLAYER_H = 3;
const int ENEMY_W = 3;
const int ENEMY_H = 2;

char canvas[HEIGHT][WIDTH];

void gotoXY(int row, int col)
{
    COORD pos;
    pos.X = static_cast<SHORT>(col);
    pos.Y = static_cast<SHORT>(row);
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void hideCursor()
{
    CONSOLE_CURSOR_INFO info = { 1, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    std::cout << "\x1b[?25l";
}

void setupConsole()
{
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SetConsoleTitleA("Airplane Game");
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode))
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    std::cout << "\x1b[?1049h\x1b[2J\x1b[H";
}

struct WJ
{
    int x;
    int y;
};

struct Bullet
{
    int x, y;
    bool fromPlayer;
};

struct Enemy
{
    int x, y;
    int shootTimer;
};

int score = INITIAL_SCORE;
WJ player;
std::vector<Bullet> bullets;
std::vector<Enemy> enemies;

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

void drawPlayer(int px, int py)
{
    putString(px,     py, " /=\\");
    putString(px + 1, py, "<(*)>");
    putString(px + 2, py, " * *");
}

void drawEnemy(int ex, int ey)
{
    putString(ex,     ey, "\\+/");
    putString(ex + 1, ey, " | ");
}

void drawHUD()
{
    char hud[WIDTH - 2];
    snprintf(hud, sizeof(hud), "Score:%d Enemy:%d/%d  WASD Move  Space Fire  Q Quit",
              score, (int)enemies.size(), ENEMY_MAX);
    putString(1, 2, hud);
}

void present()
{
    std::cout << "\x1b[H";
    for (int r = 0; r < HEIGHT; ++r)
    {
        std::cout.write(canvas[r], WIDTH);
        if (r != HEIGHT - 1)
            std::cout << "\r\n";
    }
    std::cout.flush();
}

bool inBounds(int row, int col)
{
    return row >= 1 && row < HEIGHT - 1 && col >= 1 && col < WIDTH - 1;
}

bool playerCanMoveTo(int row, int col)
{
    return row >= 1 && row <= HEIGHT - 1 - PLAYER_H
        && col >= 1 && col <= WIDTH - 1 - PLAYER_W;
}

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

void render()
{
    clearCanvas();
    drawHUD();

    for (auto& b : bullets)
        putChar(b.x, b.y, b.fromPlayer ? '^' : 'v');

    for (auto& e : enemies)
        drawEnemy(e.x, e.y);

    drawPlayer(player.x, player.y);
    present();
}

void handleInput()
{
    if (!_kbhit())
        return;

    char key = _getch();
    if (key == ' ')
    {
        Bullet b;
        b.x = player.x - 1;
        b.y = player.y + PLAYER_W / 2;
        b.fromPlayer = true;
        if (inBounds(b.x, b.y))
            bullets.push_back(b);
        return;
    }

    int nx = player.x;
    int ny = player.y;
    if (key == 'w' || key == 'W') nx--;
    else if (key == 's' || key == 'S') nx++;
    else if (key == 'a' || key == 'A') ny--;
    else if (key == 'd' || key == 'D') ny++;
    else if (key == 'q' || key == 'Q')
    {
        score = 0;
        return;
    }
    else
        return;

    if (playerCanMoveTo(nx, ny))
    {
        player.x = nx;
        player.y = ny;
    }
}

void updateBullets()
{
    for (size_t i = 0; i < bullets.size(); )
    {
        if (bullets[i].fromPlayer)
            bullets[i].x--;
        else
            bullets[i].x++;

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
                removed = true;
                break;
            }
        }
        if (!removed)
            ++bi;
    }

    for (size_t bi = 0; bi < bullets.size(); )
    {
        if (bulletHitsPlayer(bullets[bi]))
        {
            bullets.erase(bullets.begin() + bi);
            score--;
        }
        else
            ++bi;
    }
}

int main()
{
    srand(static_cast<unsigned>(time(nullptr)));

    setupConsole();
    hideCursor();

    player.x = HEIGHT - 1 - PLAYER_H;
    player.y = (WIDTH - PLAYER_W) / 2;

    int frame = 0;
    while (score > 0)
    {
        handleInput();
        updateBullets();
        updateEnemies(frame);
        checkCollisions();
        render();

        Sleep(40);
        frame++;
    }

    gotoXY(HEIGHT / 2, WIDTH / 2 - 8);
    std::cout << "Game Over! Score: " << score;
    gotoXY(HEIGHT / 2 + 1, WIDTH / 2 - 8);
    std::cout << "Press any key to exit...";
    _getch();

    CONSOLE_CURSOR_INFO info = { 1, TRUE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    std::cout << "\x1b[?25h\x1b[?1049l";
    return 0;
}
