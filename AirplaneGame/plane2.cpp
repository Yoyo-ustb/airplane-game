#include <iostream>
#include <vector>
#include <cstdio>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <ctime>

using namespace std;

// 游戏配置 
const int WIDTH = 55;
const int HEIGHT = 35;

// --- Mac 兼容性工具函数 ---

void gotoxy(int x, int y) {
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}

void hideCursor() {
    printf("\033[?25l");
}

int kbhit() {
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) { ungetc(ch, stdin); return 1; }
    return 0;
}

char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// --- 游戏对象 ---

struct Entity {
    int x, y;
};

int score = 5; // 初始分数 [cite: 75]
Entity player = {WIDTH / 2, HEIGHT - 5}; // 初始位置在最下方中间 
vector<Entity> bullets;
vector<Entity> enemies;

// 绘制我方飞机 [cite: 61, 62, 63]
void drawPlayer(int x, int y, bool erase = false) {
    if (erase) {
        gotoxy(x, y);     cout << "     ";
        gotoxy(x, y + 1); cout << "     ";
        gotoxy(x, y + 2); cout << "     ";
    } else {
        // 第一行 [cite: 62]
        gotoxy(x, y);     cout << " /=\\ "; 
        // 第二行 [cite: 62]
        gotoxy(x, y + 1); cout << " >>  "; 
        // 第三行 [cite: 63]
        gotoxy(x, y + 2); cout << " ** "; 
    }
}

// 绘制敌方飞机 (示例样式) [cite: 64]
void drawEnemy(int x, int y, bool erase = false) {
    if (erase) {
        gotoxy(x, y); cout << "   ";
    } else {
        gotoxy(x, y); cout << "+V+";
    }
}

void updateDisplay() {
    gotoxy(1, 1);
    cout << "得分: " << score << "  |  操作: ASWD 移动, 空格发射 [cite: 66, 74]";

    // 绘制子弹 [cite: 74]
    for (auto& b : bullets) {
        gotoxy(b.x, b.y); cout << "^";
    }

    // 绘制敌机 [cite: 65]
    for (auto& e : enemies) {
        drawEnemy(e.x, e.y);
    }

    drawPlayer(player.x, player.y);
    fflush(stdout);
}

int main() {
    srand(time(NULL));
    printf("\033[2J"); // 清屏
    hideCursor();

    int frame = 0;

    while (score > 0) {
        // 1. 输入处理 [cite: 66, 67, 74]
        if (kbhit()) {
            char input = getch();
            drawPlayer(player.x, player.y, true); // 先擦除旧位置
            if (input == 'w' && player.y > 2) player.y--;
            if (input == 's' && player.y < HEIGHT - 4) player.y++;
            if (input == 'a' && player.x > 2) player.x--;
            if (input == 'd' && player.x < WIDTH - 6) player.x++;
            if (input == ' ') bullets.push_back({player.x + 2, player.y - 1});
            if (input == 'q') break;
        }

        // 2. 逻辑更新
        // 子弹移动
        for (int i = 0; i < bullets.size(); i++) {
            gotoxy(bullets[i].x, bullets[i].y); cout << " "; 
            bullets[i].y--;
            if (bullets[i].y < 2) bullets.erase(bullets.begin() + i--);
        }

        // 敌机生成与移动 [cite: 65]
        if (frame % 30 == 0) {
            enemies.push_back({rand() % (WIDTH - 6) + 2, 2});
        }
        if (frame % 15 == 0) {
            for (int i = 0; i < enemies.size(); i++) {
                drawEnemy(enemies[i].x, enemies[i].y, true);
                enemies[i].y++;
                if (enemies[i].y >= HEIGHT) {
                    enemies.erase(enemies.begin() + i--);
                }
            }
        }

        // 3. 碰撞检测 [cite: 75]
        for (int i = 0; i < bullets.size(); i++) {
            for (int j = 0; j < enemies.size(); j++) {
                // 判定子弹是否击中敌机
                if (bullets[i].x >= enemies[j].x && bullets[i].x <= enemies[j].x + 2 && bullets[i].y == enemies[j].y) {
                    drawEnemy(enemies[j].x, enemies[j].y, true);
                    enemies.erase(enemies.begin() + j--);
                    bullets.erase(bullets.begin() + i--);
                    score++; // 击中增加1分 [cite: 75]
                    break;
                }
            }
        }
        
        // 玩家与敌机碰撞 [cite: 75]
        for (int i = 0; i < enemies.size(); i++) {
            if (abs(enemies[i].x - player.x) < 4 && abs(enemies[i].y - player.y) < 3) {
                score--; // 被击中减1分 [cite: 75]
                drawEnemy(enemies[i].x, enemies[i].y, true);
                enemies.erase(enemies.begin() + i--);
            }
        }

        updateDisplay();
        usleep(30000); 
        frame++;
    }

    gotoxy(WIDTH / 2 - 5, HEIGHT / 2);
    cout << "游戏结束！最终得分: " << score << endl;
    printf("\033[?25h"); 
    return 0;
}