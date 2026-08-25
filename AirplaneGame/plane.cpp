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
const int ENEMY_COUNT_MAX = 50; // [cite: 65]

// --- Mac 兼容性工具函数 ---

void gotoxy(int x, int y) { // 对应文档 gotoXY [cite: 39, 41]
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}

void hideCursor() { // 对应文档 HideCursor [cite: 39, 46]
    printf("\033[?25l");
}

int kbhit() { // 检测键盘按下 [cite: 7, 22]
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

char getch() { // 获取字符 [cite: 7, 26]
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

int score = 5; // 初始分数，设为5避免立即结束 
Entity player = {WIDTH / 2, HEIGHT - 5}; // 初始位置 
vector<Entity> bullets;
vector<Entity> enemies;

// 绘制玩家飞机 [cite: 61, 62, 63]
void drawPlayer(int x, int y, bool erase = false) {
    string s1 = erase ? "   " : "/=\\";
    string s2 = erase ? "   " : ">> ";
    string s3 = erase ? "   " : "** ";
    gotoxy(x, y);     cout << s1;
    gotoxy(x, y + 1); cout << s2;
    gotoxy(x, y + 2); cout << s3;
}

void draw() {
    // 显示分数
    gotoxy(1, 1);
    cout << "得分: " << score << "  (按 Q 退出)";

    // 绘制子弹
    for (auto& b : bullets) {
        gotoxy(b.x, b.y); cout << "|";
    }

    // 绘制敌机 [cite: 65]
    for (auto& e : enemies) {
        gotoxy(e.x, e.y); cout << "V"; 
    }

    drawPlayer(player.x, player.y);
    fflush(stdout);
}

int main() {
    srand(time(NULL));
    printf("\033[2J"); // 清屏
    hideCursor();

    int frame = 0;

    while (score > 0) { // 分数为0结束游戏 
        // 1. 输入处理 
        if (kbhit()) {
            char input = getch();
            drawPlayer(player.x, player.y, true); // 擦除旧位置
            if (input == 'w' && player.y > 2) player.y--;
            if (input == 's' && player.y < HEIGHT - 3) player.y++;
            if (input == 'a' && player.x > 2) player.x--;
            if (input == 'd' && player.x < WIDTH - 3) player.x++;
            if (input == ' ') bullets.push_back({player.x + 1, player.y - 1});
            if (input == 'q') break;
        }

        // 2. 逻辑更新
        // 子弹移动
        for (int i = 0; i < bullets.size(); i++) {
            gotoxy(bullets[i].x, bullets[i].y); cout << " "; // 擦除
            bullets[i].y--;
            if (bullets[i].y < 2) bullets.erase(bullets.begin() + i--);
        }

        // 敌机生成与移动 [cite: 65]
        if (frame % 20 == 0 && enemies.size() < ENEMY_COUNT_MAX) {
            enemies.push_back({rand() % (WIDTH - 4) + 2, 2});
        }
        if (frame % 10 == 0) {
            for (int i = 0; i < enemies.size(); i++) {
                gotoxy(enemies[i].x, enemies[i].y); cout << " ";
                enemies[i].y++;
                if (enemies[i].y >= HEIGHT) {
                    enemies.erase(enemies.begin() + i--);
                }
            }
        }

        // 3. 碰撞检测 
        for (int i = 0; i < bullets.size(); i++) {
            for (int j = 0; j < enemies.size(); j++) {
                if (bullets[i].x == enemies[j].x && bullets[i].y == enemies[j].y) {
                    gotoxy(enemies[j].x, enemies[j].y); cout << " ";
                    enemies.erase(enemies.begin() + j--);
                    bullets.erase(bullets.begin() + i--);
                    score++; // 击中增加1分
                    break;
                }
            }
        }
        
        // 玩家被击中（此处简化为与敌机碰撞）
        for (int i = 0; i < enemies.size(); i++) {
            if (abs(enemies[i].x - player.x) < 3 && abs(enemies[i].y - player.y) < 2) {
                score--; // 被击中减1分
                enemies.erase(enemies.begin() + i--);
            }
        }

        draw();
        usleep(30000); // 游戏速度控制
        frame++;
    }

    gotoxy(WIDTH / 2 - 5, HEIGHT / 2);
    cout << "游戏结束！最终得分: " << score << endl;
    printf("\033[?25h"); // 恢复光标
    return 0;
}