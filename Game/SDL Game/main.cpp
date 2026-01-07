#include <iostream>
#include <chrono>
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "ECS.h"
#include "Systems.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int WALL_THICKNESS = 20;

// base configuration for level 1 scaling
const float BASE_PADDLE_W = 120.0f;
const float BASE_PADDLE_H = 20.0f;
const float BASE_PADDLE_OFFSET_Y = 60.0f;

const float BASE_BALL_SIZE = 16.0f;
const float BASE_BALL_SPEED = 450.0f;

const float BASE_BRICK_H = 20.0f;
const float BASE_BRICK_START_Y = 50.0f;
const float BASE_BRICK_GAP = 5.0f;

const float BASE_POWERUP_SIZE = 20.0f;
const float BASE_POWERUP_SPEED = 150.0f;

float globalScale = 1.0f;
float currentBallSize = BASE_BALL_SIZE;
float currentBallSpeed = BASE_BALL_SPEED;
float currentPowerupSize = BASE_POWERUP_SIZE;
float currentPowerupSpeed = BASE_POWERUP_SPEED;

Registry gameRegistry;
int currentLevel = 1;

void ClearWorld() {
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (gameRegistry.activeEntities[i]) {
            gameRegistry.DestroyEntity(i);
        }
    }
}

void GenerateLevel(SDL_Renderer* renderer, int levelIndex) {
    ClearWorld();

    // calculate grid size: level 1 is 1x1, level 2 is 2x2 etc
    int cols = levelIndex;
    int rows = levelIndex;
    int numGames = cols * rows;

    float subGameWidth = (float)SCREEN_WIDTH / cols;
    float subGameHeight = (float)SCREEN_HEIGHT / rows;

    // scale everything down based on how many rows we have
    globalScale = 1.0f / levelIndex;

    currentBallSize = std::max(4.0f, BASE_BALL_SIZE * globalScale);
    currentBallSpeed = BASE_BALL_SPEED * globalScale;

    currentPowerupSize = std::max(4.0f, BASE_POWERUP_SIZE * globalScale);
    currentPowerupSpeed = BASE_POWERUP_SPEED * globalScale;

    float paddleW = BASE_PADDLE_W * globalScale;
    float paddleH = BASE_PADDLE_H * globalScale;
    float paddleYOffset = BASE_PADDLE_OFFSET_Y * globalScale;

    float brickH = BASE_BRICK_H * globalScale;
    float brickGap = BASE_BRICK_GAP * globalScale;
    float brickYOffset = BASE_BRICK_START_Y * globalScale;

    int currentWallThick = std::max(2, (int)(WALL_THICKNESS * globalScale));

    std::cout << "LEVEL " << levelIndex << " | Matrix: " << cols << "x" << rows
        << " | Scale: " << globalScale << std::endl;

    // Global Walls
    int topWall = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(topWall, 0, 0, SCREEN_WIDTH, currentWallThick);
    gameRegistry.AddCollider(topWall, ColliderType::WALL);
    gameRegistry.AddRender(topWall, 100, 100, 100);

    int leftWall = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(leftWall, 0, 0, currentWallThick, SCREEN_HEIGHT);
    gameRegistry.AddCollider(leftWall, ColliderType::WALL);
    gameRegistry.AddRender(leftWall, 100, 100, 100);

    int rightWall = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(rightWall, SCREEN_WIDTH - currentWallThick, 0, currentWallThick, SCREEN_HEIGHT);
    gameRegistry.AddCollider(rightWall, ColliderType::WALL);
    gameRegistry.AddRender(rightWall, 100, 100, 100);

    // Sub Games Generation
    for (int i = 0; i < numGames; ++i) {

        int gridX = i % cols;
        int gridY = i / cols;

        float offsetX = gridX * subGameWidth;
        float offsetY = gridY * subGameHeight;

        // Paddle
        float paddleX = offsetX + (subGameWidth / 2.0f) - (paddleW / 2.0f);
        float paddleY = offsetY + subGameHeight - paddleYOffset;

        int paddle = gameRegistry.CreateEntity();
        gameRegistry.AddTransform(paddle, paddleX, paddleY, (int)paddleW, (int)paddleH);
        gameRegistry.AddRigidBody(paddle);
        gameRegistry.AddCollider(paddle, ColliderType::PADDLE);
        gameRegistry.AddController(paddle);
        gameRegistry.AddRender(paddle, 0, 255, 255);
        gameRegistry.AddHealth(paddle, 100);
        gameRegistry.AddSprite(paddle, "sprite.png", renderer);

        // define the zone this specific paddle is trapped in
        float zoneMinX = offsetX;
        float zoneMaxX = offsetX + subGameWidth - paddleW;

        if (gridX == 0) zoneMinX += currentWallThick;
        if (gridX == cols - 1) zoneMaxX -= currentWallThick;

        float zoneMinY = paddleY - (10 * globalScale);
        float zoneMaxY = paddleY + (10 * globalScale);

        gameRegistry.AddPaddleZone(paddle, zoneMinX, zoneMaxX, zoneMinY, zoneMaxY);

        // Ball
        int ball = gameRegistry.CreateEntity();
        gameRegistry.AddTransform(ball, paddleX + paddleW / 2 - currentBallSize / 2, paddleY - (currentBallSize * 2), (int)currentBallSize, (int)currentBallSize);
        gameRegistry.AddRigidBody(ball);
        gameRegistry.AddCollider(ball, ColliderType::BALL);
        gameRegistry.AddRender(ball, 255, 255, 255);

        float angle = ((rand() % 100) / 100.0f) * 0.5f - 0.25f;
        gameRegistry.rigidBodies[ball].velocityX = std::sin(angle) * currentBallSpeed;
        gameRegistry.rigidBodies[ball].velocityY = -currentBallSpeed;

        // Bricks
        int bCols = 6;
        int bRows = 4 + levelIndex;

        float usableWidth = subGameWidth - (20 * globalScale);
        if (gridX == 0) usableWidth -= currentWallThick;
        if (gridX == cols - 1) usableWidth -= currentWallThick;

        float brickW = (usableWidth - (bCols - 1) * brickGap) / bCols;

        float startBrickX = offsetX + (subGameWidth - usableWidth) / 2.0f;
        float startBrickY = offsetY + brickYOffset;

        for (int r = 0; r < bRows; ++r) {
            for (int c = 0; c < bCols; ++c) {
                int brick = gameRegistry.CreateEntity();
                float bx = startBrickX + c * (brickW + brickGap);
                float by = startBrickY + r * (brickH + brickGap);

                gameRegistry.AddTransform(brick, bx, by, (int)brickW, (int)brickH);
                gameRegistry.AddCollider(brick, ColliderType::BRICK);
                gameRegistry.AddHealth(brick, 10);

                Uint8 red = (gridY * 50) + (r * 30);
                Uint8 green = 255 - (gridX * 50);
                Uint8 blue = 100 + (c * 20);
                gameRegistry.AddRender(brick, red, green, blue);
            }
        }
    }
}


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("ECS Platformer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    // disable vsync for uncapped fps testing
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    GenerateLevel(renderer, 1);
    int powerupSpawnRate = 20;
    bool quit = false;
    SDL_Event e;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
        }

        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        if (System_CheckLevelClear(gameRegistry)) {
            currentLevel++;
            GenerateLevel(renderer, currentLevel);
            lastTime = std::chrono::high_resolution_clock::now();
            continue;
        }

        // remove dead entities from cache lists
        gameRegistry.RefreshLists();

        System_Input(gameRegistry, keyboardState);
        System_Health(gameRegistry, deltaTime, powerupSpawnRate, currentPowerupSize, currentPowerupSpeed);
        System_Physics(gameRegistry, deltaTime);

        // main collision logic with multithreading
        System_Collision_Threaded(gameRegistry, currentBallSize, currentBallSpeed, SCREEN_WIDTH, SCREEN_HEIGHT);

        System_Bounds(gameRegistry, SCREEN_WIDTH, SCREEN_HEIGHT);

        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);

        System_Render(gameRegistry, renderer);

        // fps smoothing for display
        float instantFPS = 1.0f / deltaTime;
        static float smoothFPS = 0.0f;
        smoothFPS = (smoothFPS * 0.9f) + (instantFPS * 0.1f);

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ECS Engine Debugger");

        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", smoothFPS);
            ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);

            ImGui::Separator();

            float usage = (float)gameRegistry.entityCount / MAX_ENTITIES;
            char buf[32];
            sprintf_s(buf, "%d/%d Entities", gameRegistry.entityCount, MAX_ENTITIES);
            ImGui::ProgressBar(usage, ImVec2(0.0f, 0.0f), buf);

            double memoryMB = sizeof(Registry) / (1024.0 * 1024.0);
            ImGui::Text("ECS Memory: %.2f MB", memoryMB);
        }

        if (ImGui::CollapsingHeader("Game Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Powerup Chance (%)", &powerupSpawnRate, 0, 100);

            if (ImGui::Button("STRESS TEST: Spawn 5k Balls")) {
                for (int i = 0; i < 5000; i++) {
                    int newBall = gameRegistry.CreateEntity();

                    if (newBall == -1) {
                        std::cout << "Registry Full!" << std::endl;
                        break;
                    }

                    float x = static_cast<float>(rand() % (SCREEN_WIDTH - 20));
                    float y = static_cast<float>(rand() % (SCREEN_HEIGHT - 200) + 100);

                    gameRegistry.AddTransform(newBall, x, y, (int)currentBallSize, (int)currentBallSize);
                    gameRegistry.AddRigidBody(newBall);
                    gameRegistry.AddCollider(newBall, ColliderType::BALL);
                    gameRegistry.AddRender(newBall, 255, 255, 0);

                    float angle = ((rand() % 100) / 100.0f) * 6.28f;
                    gameRegistry.rigidBodies[newBall].velocityX = std::cos(angle) * currentBallSpeed;
                    gameRegistry.rigidBodies[newBall].velocityY = std::sin(angle) * currentBallSpeed;
                }
            }
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    TextureManager::CleanUp();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}