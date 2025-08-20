#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <iostream>
#include "types.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

class Ball {
public:
    float x;
    float y;
    vec velocity;
    int w, h;
    int attack;
    SDL_Texture* texture;

    Ball(float startX, float startY, vec v, SDL_Texture* tex, int width, int height, int attackPower = 2)
        : x(startX), y(startY), velocity(v), w(width), h(height), texture(tex), attack(attackPower) {}

    void update(int screenW, int screenH) {
        x += velocity.x;
        y += velocity.y;

        // Bounce on walls
        if (x < 0) { x = 0; velocity.x *= -1; }
        if (x + w > screenW) { x = screenW - w; velocity.x *= -1; }
        if (y < 0) { y = 0; velocity.y *= -1; }
        if (y + h > screenH) { y = screenH - h; velocity.y *= -1; }
    }

    void render(SDL_Renderer* renderer) {
        SDL_Rect dest = { (int)x, (int)y, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &dest);
    }
};

class Brick {
public:
    float x, y;
    int w, h;
    int hp;
    SDL_Texture* texture;

    Brick(float X, float Y, int W, int H, int HP, SDL_Texture* tex)
        : x(X), y(Y), w(W), h(H), hp(HP), texture(tex) {}

    bool isAlive() const { return hp > 0; }

    void render(SDL_Renderer* renderer) {
        if (!isAlive()) return;
        SDL_Rect rect{ (int)x, (int)y, w, h };
        SDL_RenderCopy(renderer, texture, NULL, &rect);
    }
};


bool collided(float x1, float y1, float w1, float h1,
              float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 &&
            x1 + w1 > x2 &&
            y1 < y2 + h2 &&
            y1 + h1 > y2);
}

int clamp (int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

std::vector<Brick> LoadBricks(SDL_Texture* brickTex1, SDL_Texture* brickTex2,
                              SDL_Texture* brickTex3, SDL_Texture* brickTex4,
                              int bw, int bh, int level, int windowWidth) {
    std::vector<Brick> bricks;
    int padding = 5;

    // Determine rows (increase with level, cap at 14)
    int rows = std::min(4 + level / 5, 14);

    // Target is centered horizontally
    int targetX = windowWidth / 2;

    // Compute column range that covers the target
    int targetColStart = (targetX - bw/2) / (bw + padding);
    int targetColEnd = (targetX + bw/2) / (bw + padding);

    bool brickAboveTargetPlaced = false;

    for (int r = 0; r < rows; ++r) {
        // Random number of bricks per row
        int cols = 5 + rand() % 15;
        int startX = 50 + rand() % 30; // horizontal offset for variety

        for (int c = 0; c < cols; ++c) {
            int x = startX + c * (bw + padding);
            int y = 50 + r * (bh + padding);

            // Random chance to skip a brick
            if (rand() % 100 < std::max(10 - level / 15, 2)) continue;

            // Force at least one brick above the target
            int col = (x - 50) / (bw + padding); // approximate column index
            if (!brickAboveTargetPlaced && col >= targetColStart && col <= targetColEnd && r < 3) {
                brickAboveTargetPlaced = true;
            } else if (!brickAboveTargetPlaced && r == rows - 1 && col >= targetColStart && col <= targetColEnd) {
                // ensure last chance to place a brick above target
                brickAboveTargetPlaced = true;
            }

            // Determine HP based on level
            int hp = 1;
            int chance = rand() % 100;

            if (level < 20) {
                if (chance < 40) hp = 1;
                else if (chance < 70) hp = 2;
                else if (chance < 90) hp = 3;
                else hp = 5;
            } else if (level < 60) {
                if (chance < 20) hp = 1;
                else if (chance < 50) hp = 2;
                else if (chance < 80) hp = 3;
                else if (chance < 95) hp = 5;
                else hp = 10;
            } else {
                if (chance < 10) hp = 2;
                else if (chance < 40) hp = 3;
                else if (chance < 70) hp = 5;
                else if (chance < 90) hp = 8;
                else hp = 10;
            }

            // Assign texture based on HP
            SDL_Texture* tex = nullptr;
            if (hp <= 2) tex = brickTex1;
            else if (hp <= 3) tex = brickTex2;
            else if (hp <= 5) tex = brickTex3;
            else tex = brickTex4;

            bricks.push_back(Brick(x, y, bw, bh, hp, tex));
        }
    }

    // Safety: if no brick ended up above target, force one
    if (!brickAboveTargetPlaced) {
        int y = 50 + rand() % std::min(3, rows) * (bh + padding); // place in top 3 rows
        bricks.push_back(Brick(targetX - bw/2, y, bw, bh, std::min(5 + level/10, 10), brickTex4));
    }

    return bricks;
}

int main(int argc, char* argv[]) {
    int level = 1;  // starting level, NOOB!
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cout << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cout << "TTF_Init Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    // Create window and renderer
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    SDL_Window* window = SDL_CreateWindow("Paddle Example", 100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load paddle texture
    SDL_Surface* tempSurface = IMG_Load("paddle.png");
    if (!tempSurface) {
        std::cout << "IMG_Load Error: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    SDL_Texture* paddleTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);
    SDL_FreeSurface(tempSurface);

    // Load ball texture
    SDL_Surface* tempBall = IMG_Load("ball.png");
    if (!tempBall) {
        std::cout << "IMG_Load Error: " << IMG_GetError() << std::endl;
        // cleanup and quit
        return 1;
    }
    SDL_Texture* ballTexture = SDL_CreateTextureFromSurface(renderer, tempBall);
    int ballW = tempBall->w;
    int ballH = tempBall->h;
    SDL_FreeSurface(tempBall);

    // Create Ball instance (start middle, moving diagonally)
    Ball ball(WINDOW_WIDTH/2 - ballW/2, WINDOW_HEIGHT/2 - ballH/2, vec(5.0f, 2.0f), ballTexture, ballW, ballH);

    // Load brick textures
    SDL_Surface* tmp1 = IMG_Load("brick1.png");
    SDL_Surface* tmp2 = IMG_Load("brick2.png");
    SDL_Surface* tmp3 = IMG_Load("brick3.png");
    SDL_Surface* tmp4 = IMG_Load("brick4.png");

    SDL_Texture* brickTex1 = SDL_CreateTextureFromSurface(renderer, tmp1);
    SDL_Texture* brickTex2 = SDL_CreateTextureFromSurface(renderer, tmp2);
    SDL_Texture* brickTex3 = SDL_CreateTextureFromSurface(renderer, tmp3);
    SDL_Texture* brickTex4 = SDL_CreateTextureFromSurface(renderer, tmp4);

    int bw = tmp1->w;  // assuming all bricks same size
    int bh = tmp1->h;

    SDL_FreeSurface(tmp1);
    SDL_FreeSurface(tmp2);
    SDL_FreeSurface(tmp3);
    SDL_FreeSurface(tmp4);

    // DA BRICKS
    std::vector<Brick> bricks;
    bricks = LoadBricks(brickTex1, brickTex2, brickTex3, brickTex4, bw, bh, level, WINDOW_WIDTH);

    // DA TARGET
    SDL_Surface* tmpTarget = IMG_Load("target.png");
    if (!tmpTarget) {
        std::cout << "IMG_Load Error: " << IMG_GetError() << std::endl;
        // handle cleanup and exit
    }

    SDL_Texture* targetTexture = SDL_CreateTextureFromSurface(renderer, tmpTarget);
    int targetW = tmpTarget->w;
    int targetH = tmpTarget->h;
    SDL_FreeSurface(tmpTarget);

    // Target position: top middle
    SDL_Rect targetRect;
    targetRect.w = targetW;
    targetRect.h = targetH;
    targetRect.x = (WINDOW_WIDTH - targetW) / 2;
    targetRect.y = 50; // 50px from top

    // FONT
    TTF_Font* fonty = TTF_OpenFont("font.ttf", 24); // <- FONTY. 24 = font size
    if (!fonty) {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
    // Prepare level text
    std::string levelText = "Level: " + std::to_string(level);
    SDL_Color colour = {255, 255, 255, 255}; // SDL dosn't spell colour right :-(

    SDL_Surface* textSurface = TTF_RenderText_Solid(fonty, levelText.c_str(), colour);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

    SDL_Rect textRect;
    textRect.x = 10;   // top-left corner
    textRect.y = 10;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;





    // Paddle rectangle
    SDL_Rect paddleRect;
    paddleRect.w = 128;  // width of paddle
    paddleRect.h = 32;   // height of paddle
    paddleRect.x = (WINDOW_WIDTH - paddleRect.w) / 2;
    paddleRect.y = WINDOW_HEIGHT - paddleRect.h - 50;

    bool running = true;
    SDL_Event e;
    const int paddleSpeed = 10;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
        }

        // Keyboard state for smooth movement
        const Uint8* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_LEFT]) {
            paddleRect.x -= paddleSpeed;
            if (paddleRect.x < 0) paddleRect.x = 0;
        }
        if (state[SDL_SCANCODE_RIGHT]) {
            paddleRect.x += paddleSpeed;
            if (paddleRect.x + paddleRect.w > WINDOW_WIDTH) paddleRect.x = WINDOW_WIDTH - paddleRect.w;
        }
        // Move ball
        ball.update(WINDOW_WIDTH, WINDOW_HEIGHT);
        if (collided(ball.x, ball.y, ball.w, ball.h,
                    paddleRect.x, paddleRect.y, paddleRect.w, paddleRect.h)) {
            // bounce the ball
            ball.velocity.y *= -1;
            ball.y = paddleRect.y - ball.h; // move ball above paddle
        }
        for (Brick& brick : bricks) {
            if (brick.isAlive() &&
                collided(ball.x, ball.y, ball.w, ball.h,
                        brick.x, brick.y, brick.w, brick.h)) {

                brick.hp -= ball.attack;
                ball.velocity.y *= -1;
                break;
            }
        }
        if (collided(ball.x, ball.y, ball.w, ball.h,
                    targetRect.x, targetRect.y, targetRect.w, targetRect.h)) {
            level++;  // go to next level
            std::cout << "Level Up! Now level " << level << std::endl;

            // Reset ball position to middle
            ball.x = WINDOW_WIDTH/2 - ball.w/2;
            ball.y = WINDOW_HEIGHT/2 - ball.h/2;
            ball.attack = clamp(2 + level, 2, 20); // increase attack power with level, max 20
            ball.velocity = vec(5.0f, 2.0f);  // reset movement

            // Reload bricks for next level
            bricks = LoadBricks(brickTex1, brickTex2, brickTex3, brickTex4, bw, bh, level, WINDOW_WIDTH);
        }



        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw bricks
        for (Brick& brick : bricks) {
            brick.render(renderer);
        }

        // Draw target
        SDL_RenderCopy(renderer, targetTexture, NULL, &targetRect);

        // Draw level text
        SDL_FreeSurface(textSurface);
        textSurface = TTF_RenderText_Solid(fonty, ("Level: " + std::to_string(level)).c_str(), colour);
        if (!textSurface) {
            std::cout << "Failed to render text: " << TTF_GetError() << std::endl;
            break; // exit loop on error
        }
        textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        // Draw paddle & ball
        SDL_RenderCopy(renderer, paddleTexture, NULL, &paddleRect);
        ball.render(renderer);

        // Update screen
        SDL_RenderPresent(renderer);

        // Delay for ~16ms (~60 FPS)
        SDL_Delay(16);
    }

    // Cleanup
    SDL_DestroyTexture(paddleTexture);
    SDL_DestroyTexture(ballTexture);
    SDL_DestroyTexture(brickTex1);
    SDL_DestroyTexture(brickTex2);
    SDL_DestroyTexture(brickTex3);
    SDL_DestroyTexture(brickTex4);
    SDL_DestroyTexture(textTexture);
    TTF_CloseFont(fonty);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
