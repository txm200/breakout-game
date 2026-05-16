#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>

// ===== 常量定义 =====
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PADDLE_WIDTH = 120;
const int PADDLE_HEIGHT = 16;
const int PADDLE_Y = SCREEN_HEIGHT - 60;
const int BALL_SIZE = 14;
const float BALL_SPEED_INIT = 5.0f;
const float BALL_SPEED_MAX = 9.0f;
const float BALL_SPEED_INC = 0.15f;
const int BRICK_ROWS = 7;
const int BRICK_COLS = 10;
const int BRICK_WIDTH = 73;
const int BRICK_HEIGHT = 24;
const int BRICK_PAD = 4;
const int BRICK_OFFSET_X = 26;
const int BRICK_OFFSET_Y = 50;
const int LIVES = 3;

// ===== 数据结构 =====
struct Ball {
    float x, y;
    float dx, dy;
    float speed;
    bool launched;
};

struct Paddle {
    float x;
    int width;
};

struct Brick {
    SDL_Rect rect;
    bool alive;
    int colorIndex; // 0-6, 越高压值越小(颜色越红)
};

struct Particle {
    float x, y;
    float dx, dy;
    int life;
    int maxLife;
    int r, g, b;
};

// ===== 全局变量 =====
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
TTF_Font* fontSmall = nullptr;

Paddle paddle;
Ball ball;
std::vector<Brick> bricks;
std::vector<Particle> particles;

int score = 0;
int lives = LIVES;
int level = 1;
bool gameOver = false;
bool gameWin = false;
bool paused = false;
float ballSpeed = BALL_SPEED_INIT;

Uint32 lastTime = 0;

// 砖块颜色表 (7行渐变)
SDL_Color brickColors[7] = {
    {255, 60,  60,  255},  // 红
    {255, 120, 40,  255},  // 橙
    {255, 200, 30,  255},  // 黄
    {100, 220, 60,  255},  // 绿
    {40,  180, 220, 255},  // 青
    {60,  100, 240, 255},  // 蓝
    {150, 60,  220, 255}   // 紫
};

// 砖块分值表
int brickScores[7] = {70, 60, 50, 40, 30, 20, 10};

// ===== 初始化 =====
bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
    if (TTF_Init() < 0) return false;

    window = SDL_CreateWindow("打砖块 Breakout",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) return false;

    // 尝试加载中文字体
    font = TTF_OpenFont("C:/Windows/Fonts/msyh.ttc", 28);
    fontSmall = TTF_OpenFont("C:/Windows/Fonts/msyh.ttc", 18);
    if (!font) {
        font = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 28);
        fontSmall = TTF_OpenFont("C:/Windows/Fonts/arial.ttf", 18);
    }

    return true;
}

void spawnParticles(float x, float y, SDL_Color color) {
    for (int i = 0; i < 12; i++) {
        Particle p;
        p.x = x;
        p.y = y;
        float angle = (rand() % 360) * M_PI / 180.0f;
        float speed = 1.5f + (rand() % 100) / 50.0f;
        p.dx = cos(angle) * speed;
        p.dy = sin(angle) * speed;
        p.life = 30 + rand() % 20;
        p.maxLife = p.life;
        p.r = color.r;
        p.g = color.g;
        p.b = color.b;
        particles.push_back(p);
    }
}

void initLevel() {
    bricks.clear();
    particles.clear();

    int startY = BRICK_OFFSET_Y + level * 8; // 每关略微下移

    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            Brick b;
            b.rect.x = BRICK_OFFSET_X + col * (BRICK_WIDTH + BRICK_PAD);
            b.rect.y = startY + row * (BRICK_HEIGHT + BRICK_PAD);
            b.rect.w = BRICK_WIDTH;
            b.rect.h = BRICK_HEIGHT;
            b.alive = true;

            // 高层行 = 更高分值, 小行号 = 顶部
            if (row < 2)      b.colorIndex = 0 + level % 3;      // 红/橙
            else if (row < 4) b.colorIndex = 2 + level % 3;      // 黄/绿
            else              b.colorIndex = 4 + level % 3;      // 青/蓝/紫
            if (b.colorIndex > 6) b.colorIndex = 6;

            bricks.push_back(b);
        }
    }

    // 重置球
    ball.x = SCREEN_WIDTH / 2.0f;
    ball.y = PADDLE_Y - PADDLE_HEIGHT / 2 - BALL_SIZE;
    ball.launched = false;

    // 增加速度
    ballSpeed = BALL_SPEED_INIT + (level - 1) * 0.6f;
    if (ballSpeed > BALL_SPEED_MAX) ballSpeed = BALL_SPEED_MAX;

    float angle = -M_PI / 2 + ((rand() % 80) - 40) * M_PI / 180.0f;
    ball.dx = cos(angle) * ballSpeed;
    ball.dy = sin(angle) * ballSpeed;
    ball.speed = ballSpeed;

    // 重置挡板
    paddle.x = SCREEN_WIDTH / 2.0f - PADDLE_WIDTH / 2;
    paddle.width = PADDLE_WIDTH;

    gameOver = false;
    gameWin = false;
    paused = false;
}

void initGame() {
    srand(static_cast<unsigned>(time(nullptr)));
    score = 0;
    lives = LIVES;
    level = 1;
    ballSpeed = BALL_SPEED_INIT;
    initLevel();
}

// ===== 渲染 =====
void drawText(const char* text, int x, int y, TTF_Font* f, SDL_Color color, bool center) {
    SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text, color);
    if (!surf) {
        // fallback: no text support, silently skip
        return;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect dst;
    if (center) {
        dst = {x - surf->w / 2, y - surf->h / 2, surf->w, surf->h};
    } else {
        dst = {x, y, surf->w, surf->h};
    }
    SDL_RenderCopy(renderer, tex, nullptr, &dst);
    SDL_FreeSurface(surf);
    SDL_DestroyTexture(tex);
}

void drawRect(int x, int y, int w, int h, SDL_Color color) {
    SDL_Rect r = {x, y, w, h};
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &r);
}

void drawBrick(const Brick& b) {
    SDL_Color c = brickColors[b.colorIndex];
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(renderer, &b.rect);

    // 顶部高亮
    auto clamp_add = [](Uint8 v, int add) -> Uint8 {
        int r = v + add;
        return (Uint8)(r > 255 ? 255 : r);
    };
    auto clamp_sub = [](Uint8 v, int sub) -> Uint8 {
        int r = (int)v - sub;
        return (Uint8)(r < 0 ? 0 : r);
    };
    SDL_Color hl = {clamp_add(c.r, 60), clamp_add(c.g, 60), clamp_add(c.b, 60), 255};
    SDL_Rect top = {b.rect.x, b.rect.y, b.rect.w, b.rect.h / 3};
    SDL_SetRenderDrawColor(renderer, hl.r, hl.g, hl.b, hl.a);
    SDL_RenderFillRect(renderer, &top);

    // 底部阴影
    SDL_Color sh = {clamp_sub(c.r, 50), clamp_sub(c.g, 50), clamp_sub(c.b, 50), 255};
    SDL_Rect bot = {b.rect.x, b.rect.y + b.rect.h * 2 / 3, b.rect.w, b.rect.h / 3};
    SDL_SetRenderDrawColor(renderer, sh.r, sh.g, sh.b, sh.a);
    SDL_RenderFillRect(renderer, &bot);
}

void drawBall() {
    // 球体光晕
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 40);
    for (int i = 4; i >= 0; i--) {
        SDL_Rect glow = {
            (int)(ball.x - BALL_SIZE / 2 - i * 2),
            (int)(ball.y - BALL_SIZE / 2 - i * 2),
            BALL_SIZE + i * 4, BALL_SIZE + i * 4
        };
        SDL_RenderFillRect(renderer, &glow);
    }
    // 球体
    drawRect(ball.x - BALL_SIZE/2, ball.y - BALL_SIZE/2,
             BALL_SIZE, BALL_SIZE, {255, 250, 240, 255});
    // 高光
    drawRect(ball.x - BALL_SIZE/4, ball.y - BALL_SIZE/4,
             BALL_SIZE/3, BALL_SIZE/3, {255, 255, 255, 200});
}

void drawPaddle() {
    // 阴影
    drawRect(paddle.x + 3, PADDLE_Y + 3, paddle.width, PADDLE_HEIGHT, {30, 30, 30, 100});
    // 主体
    drawRect(paddle.x, PADDLE_Y, paddle.width, PADDLE_HEIGHT, {70, 180, 255, 255});
    // 高光
    drawRect(paddle.x + 4, PADDLE_Y + 2, paddle.width - 8, PADDLE_HEIGHT / 3, {160, 220, 255, 200});
}

void render() {
    // 背景渐变
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        int r = 10 + i / 12;
        int g = 10 + i / 16;
        int b = 35 + i / 10;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawLine(renderer, 0, i, SCREEN_WIDTH, i);
    }

    // 砖块
    for (auto& b : bricks) {
        if (b.alive) drawBrick(b);
    }

    // 粒子
    for (auto& p : particles) {
        float ratio = (float)p.life / p.maxLife;
        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, (Uint8)(200 * ratio));
        SDL_Rect r = {(int)p.x - 2, (int)p.y - 2, 4, 4};
        SDL_RenderFillRect(renderer, &r);
    }

    drawPaddle();
    drawBall();

    // 分数
    SDL_Color white = {255, 255, 255, 255};
    std::string scoreStr = "分数: " + std::to_string(score);
    drawText(scoreStr.c_str(), 10, 8, fontSmall, white, false);

    // 关卡
    std::string levelStr = "第 " + std::to_string(level) + " 关";
    drawText(levelStr.c_str(), SCREEN_WIDTH / 2, 8, fontSmall, white, true);

    // 生命
    std::string livesStr = "生命: " + std::to_string(lives);
    drawText(livesStr.c_str(), SCREEN_WIDTH - 10, 8, fontSmall, white, false);

    // 分隔线
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
    SDL_RenderDrawLine(renderer, 0, 32, SCREEN_WIDTH, 32);

    // 暂停提示
    if (paused && !gameOver && !gameWin) {
        SDL_Color yellow = {255, 255, 100, 255};
        drawText("已暂停 - 按 P 继续", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 20, font, yellow, true);
        drawText("按 ESC 重新开始", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 25, fontSmall, yellow, true);
    }

    // 游戏结束
    if (gameOver) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);
        SDL_Color red = {255, 80, 80, 255};
        drawText("游戏结束!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30, font, red, true);
        std::string finalScore = "最终分数: " + std::to_string(score);
        drawText(finalScore.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 15, fontSmall, white, true);
        drawText("按 R 重新开始", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 50, fontSmall, white, true);
    }

    // 通关
    if (gameWin) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
        SDL_Rect overlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);
        SDL_Color green = {100, 255, 100, 255};
        std::string winStr = "恭喜通关! 第 " + std::to_string(level) + " 关";
        drawText(winStr.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 30, font, green, true);
        std::string finalScore = "分数: " + std::to_string(score);
        drawText(finalScore.c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 15, fontSmall, white, true);
        drawText("按 N 进入下一关  |  按 R 重新开始", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 50, fontSmall, white, true);
    }

    // 未发射提示
    if (!ball.launched && !gameOver && !gameWin && !paused) {
        SDL_Color hint = {200, 200, 200, 150};
        drawText("按 空格键 发射 / 方向键 移动", SCREEN_WIDTH / 2, PADDLE_Y - 50, fontSmall, hint, true);
    }

    SDL_RenderPresent(renderer);
}

// ===== 碰撞检测 =====
bool ballBrickCollision(const Ball& b, const Brick& br) {
    float closestX = std::max((float)br.rect.x, std::min(b.x, (float)(br.rect.x + br.rect.w)));
    float closestY = std::max((float)br.rect.y, std::min(b.y, (float)(br.rect.y + br.rect.h)));
    float dx = b.x - closestX;
    float dy = b.y - closestY;
    return (dx * dx + dy * dy) < (BALL_SIZE / 2) * (BALL_SIZE / 2);
}

void updateParticles() {
    for (auto it = particles.begin(); it != particles.end();) {
        it->x += it->dx;
        it->y += it->dy;
        it->dy += 0.15f; // 重力
        it->life--;
        if (it->life <= 0)
            it = particles.erase(it);
        else
            ++it;
    }
}

void update() {
    if (gameOver || gameWin || paused) return;

    // 发射
    if (!ball.launched) {
        ball.x = paddle.x + paddle.width / 2.0f;
        ball.y = PADDLE_Y - PADDLE_HEIGHT / 2 - BALL_SIZE;
        return;
    }

    // 移动球
    ball.x += ball.dx;
    ball.y += ball.dy;

    // 墙壁碰撞
    if (ball.x - BALL_SIZE / 2 <= 0) {
        ball.x = BALL_SIZE / 2;
        ball.dx = -ball.dx;
    }
    if (ball.x + BALL_SIZE / 2 >= SCREEN_WIDTH) {
        ball.x = SCREEN_WIDTH - BALL_SIZE / 2;
        ball.dx = -ball.dx;
    }
    if (ball.y - BALL_SIZE / 2 <= 32) { // 顶部UI下方
        ball.y = 32 + BALL_SIZE / 2;
        ball.dy = -ball.dy;
    }

    // 挡板碰撞
    if (ball.dy > 0 &&
        ball.y + BALL_SIZE / 2 >= PADDLE_Y &&
        ball.y - BALL_SIZE / 2 <= PADDLE_Y + PADDLE_HEIGHT &&
        ball.x >= paddle.x &&
        ball.x <= paddle.x + paddle.width) {

        // 反弹角度取决于击中挡板的相对位置
        float hitPos = (ball.x - paddle.x) / paddle.width; // 0.0 - 1.0
        float angle = (hitPos - 0.5f) * M_PI * 0.65f; // -65% to +65% of PI
        ball.dx = sin(angle) * ball.speed;
        ball.dy = -cos(angle) * ball.speed;
        ball.y = PADDLE_Y - BALL_SIZE / 2;

        // 确保不会太平
        if (fabs(ball.dy) < 1.2f) {
            ball.dy = (ball.dy > 0 ? 1.2f : -1.2f);
            float len = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
            ball.dx = ball.dx / len * ball.speed;
            ball.dy = ball.dy / len * ball.speed;
        }
    }

    // 出界
    if (ball.y > SCREEN_HEIGHT + BALL_SIZE) {
        lives--;
        if (lives <= 0) {
            gameOver = true;
        } else {
            ball.launched = false;
            ball.x = paddle.x + paddle.width / 2.0f;
            ball.y = PADDLE_Y - PADDLE_HEIGHT / 2 - BALL_SIZE;
            float angle = -M_PI / 2 + ((rand() % 80) - 40) * M_PI / 180.0f;
            ball.dx = cos(angle) * ball.speed;
            ball.dy = sin(angle) * ball.speed;
        }
    }

    // 砖块碰撞
    bool levelCleared = true;
    for (auto& b : bricks) {
        if (!b.alive) continue;
        levelCleared = false;

        if (ballBrickCollision(ball, b)) {
            b.alive = false;
            score += brickScores[b.colorIndex];
            spawnParticles(b.rect.x + b.rect.w / 2, b.rect.y + b.rect.h / 2, brickColors[b.colorIndex]);

            // 简单的反弹处理
            float ballLeft = ball.x - BALL_SIZE / 2;
            float ballRight = ball.x + BALL_SIZE / 2;
            float ballTop = ball.y - BALL_SIZE / 2;
            float ballBottom = ball.y + BALL_SIZE / 2;

            float overlapLeft = ballRight - b.rect.x;
            float overlapRight = (b.rect.x + b.rect.w) - ballLeft;
            float overlapTop = ballBottom - b.rect.y;
            float overlapBottom = (b.rect.y + b.rect.h) - ballTop;

            float minOverlapX = std::min(overlapLeft, overlapRight);
            float minOverlapY = std::min(overlapTop, overlapBottom);

            if (minOverlapX < minOverlapY) {
                ball.dx = -ball.dx;
            } else {
                ball.dy = -ball.dy;
            }

            // 加速
            ballSpeed += BALL_SPEED_INC;
            if (ballSpeed > BALL_SPEED_MAX) ballSpeed = BALL_SPEED_MAX;
            ball.speed = ballSpeed;
            float len = sqrt(ball.dx * ball.dx + ball.dy * ball.dy);
            ball.dx = ball.dx / len * ball.speed;
            ball.dy = ball.dy / len * ball.speed;

            break; // 每帧只处理一次碰撞
        }
    }

    if (levelCleared) {
        gameWin = true;
        score += 200; // 通关加分
    }

    updateParticles();
}

// ===== 输入处理 =====
void handleInput(SDL_Event& e) {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);

    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
            case SDLK_SPACE:
                if (!ball.launched && !gameOver && !gameWin) {
                    ball.launched = true;
                }
                break;
            case SDLK_p:
                if (!gameOver && !gameWin) paused = !paused;
                break;
            case SDLK_r:
                initGame();
                break;
            case SDLK_n:
                if (gameWin) {
                    level++;
                    initLevel();
                }
                break;
            case SDLK_ESCAPE:
                if (paused) initGame();
                break;
        }
    }

    // 持续移动
    if (!paused && !gameOver && !gameWin) {
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
            paddle.x -= 8;
        }
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
            paddle.x += 8;
        }
        if (keys[SDL_SCANCODE_SPACE] && !ball.launched) {
            ball.launched = true;
        }

        // 鼠标/触控
        int mx;
        if (SDL_GetMouseState(&mx, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            paddle.x = mx - paddle.width / 2;
        } else if (ball.launched) {
            // 球发射后也可以用鼠标控制挡板
            // 但不要自动吸附
        }

        // 限制挡板
        if (paddle.x < 0) paddle.x = 0;
        if (paddle.x + paddle.width > SCREEN_WIDTH) paddle.x = SCREEN_WIDTH - paddle.width;
    }
}

// ===== 清理 =====
void cleanup() {
    TTF_CloseFont(font);
    TTF_CloseFont(fontSmall);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// ===== 主循环 =====
int main(int argc, char* argv[]) {
    if (!initSDL()) {
        SDL_Log("SDL 初始化失败!");
        return 1;
    }

    initGame();

    bool running = true;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            handleInput(e);
        }

        update();
        render();
        SDL_Delay(8); // ~120 FPS cap
    }

    cleanup();
    return 0;
}