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

// Engine
Registry gameRegistry;

void SetupLevel(SDL_Renderer* renderer) {
    // Create Player
    int player = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(player, 100, 100, 32, 32);
    gameRegistry.AddRigidBody(player);
    gameRegistry.AddCollider(player, ColliderType::PLAYER);
    gameRegistry.AddController(player);
    gameRegistry.AddRender(player, 0, 255, 0);
    gameRegistry.AddHealth(player, 100);
    gameRegistry.AddSprite(player, "sprite.png", renderer);

    // Create Ground 
    int ground = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(ground, 0, 600, 1280, 50);
    gameRegistry.AddCollider(ground, ColliderType::PLATFORM);
    gameRegistry.AddRender(ground, 100, 100, 100); // Gray

    // Create a Floating Platform
    int platform = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(platform, 400, 450, 200, 30);
    gameRegistry.AddCollider(platform, ColliderType::PLATFORM);
    gameRegistry.AddRender(platform, 150, 150, 150);

    // Create a Monster
    int monster = gameRegistry.CreateEntity();
    gameRegistry.AddTransform(monster, 450, 100, 40, 40);
    gameRegistry.AddRigidBody(monster);
    gameRegistry.AddCollider(monster, ColliderType::ENEMY);
    gameRegistry.AddRender(monster, 255, 0, 0); // Red
    gameRegistry.AddHealth(monster, 50);  // Can die
    gameRegistry.AddDamage(monster, 10);  // Can hurt
    gameRegistry.AddSprite(monster, "sprite.png", renderer);

    /*for (int i = 0; i < 100000; i++) {
        int e = gameRegistry.CreateEntity();
        gameRegistry.AddTransform(e, rand() % 1280, rand() % 720, 4, 4);
        gameRegistry.AddRender(e, 50, 50, 200); // Blue particles
    }*/
}

int main(int argc, char* argv[]) {
    // SDL Init 
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("ECS Platformer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // ImGui Init 
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    SetupLevel(renderer);

    bool quit = false;
    SDL_Event e;
    auto lastTime = std::chrono::high_resolution_clock::now();

    //  Game Loop 
    while (!quit) {
        // Input
        while (SDL_PollEvent(&e) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) quit = true;
        }

        const Uint8* keyboardState = SDL_GetKeyboardState(NULL);

        // Time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Systems Update 
        System_Input(gameRegistry, keyboardState);
        System_Health(gameRegistry, deltaTime);
        System_Physics(gameRegistry, deltaTime);
        System_Collision(gameRegistry);

        // Render
        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255); // Background
        SDL_RenderClear(renderer);

        // Draw Entities
        System_Render(gameRegistry, renderer);

        // Draw UI
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("ECS Engine Debugger");

        // Performance Section
        if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
            // FPS and Frame Time
            ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
            ImGui::Text("Frame Time: %.3f ms", deltaTime * 1000.0f);

            ImGui::Separator();

            // Entity Count 
            float usage = (float)gameRegistry.entityCount / MAX_ENTITIES;
            char buf[32];
            sprintf_s(buf, "%d/%d Entities", gameRegistry.entityCount, MAX_ENTITIES);
            ImGui::ProgressBar(usage, ImVec2(0.0f, 0.0f), buf);

            // Memory Usage Calculation
            double memoryMB = sizeof(Registry) / (1024.0 * 1024.0);
            ImGui::Text("ECS Memory: %.2f MB", memoryMB);
        }

        // Player Data
        if (ImGui::CollapsingHeader("Player Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {

            // Find player ID
            int playerID = -1;
            for (int i = 0; i < MAX_ENTITIES; ++i) {
                if (gameRegistry.activeEntities[i] && gameRegistry.signatures[i].test(3)) {
                    playerID = i;
                    break;
                }
            }

            if (playerID != -1) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Player Found (ID: %d)", playerID);

                // Editable Position
                if (gameRegistry.signatures[playerID].test(0)) {
                    ImGui::SeparatorText("Transform");
                    float pos[2] = { gameRegistry.transforms[playerID].x, gameRegistry.transforms[playerID].y };
                    if (ImGui::DragFloat2("Position (X, Y)", pos)) {
                        gameRegistry.transforms[playerID].x = pos[0];
                        gameRegistry.transforms[playerID].y = pos[1];
                        // Update collider immediately if moved manually
                        if (gameRegistry.signatures[playerID].test(2)) {
                            gameRegistry.colliders[playerID].aabb.x = (int)pos[0];
                            gameRegistry.colliders[playerID].aabb.y = (int)pos[1];
                        }
                    }
                }

                // Physics read-only
                if (gameRegistry.signatures[playerID].test(1)) {
                    ImGui::SeparatorText("Physics");
                    ImGui::Text("Velocity: { %.2f, %.2f }",
                        gameRegistry.rigidBodies[playerID].velocityX,
                        gameRegistry.rigidBodies[playerID].velocityY
                    );

                    bool grounded = gameRegistry.rigidBodies[playerID].isGrounded;
                    ImGui::Checkbox("Is Grounded", &grounded); // Visualization only

                    if (ImGui::Button("Stop Velocity")) {
                        gameRegistry.rigidBodies[playerID].velocityX = 0;
                        gameRegistry.rigidBodies[playerID].velocityY = 0;
                    }
                }

                // Jump debugging 
                if (gameRegistry.signatures[playerID].test(3)) {
                    ImGui::SeparatorText("Controller");
                    ImGui::Text("Jumps: %d / %d",
                        gameRegistry.controllers[playerID].currentJumps,
                        gameRegistry.controllers[playerID].maxJumps
                    );
                }

                // Stats editable
                if (gameRegistry.signatures[playerID].test(4)) { // Check Bit 4
                    ImGui::SeparatorText("Health");
                    int hp = gameRegistry.healths[playerID].current;
                    if (ImGui::SliderInt("HP", &hp, 0, gameRegistry.healths[playerID].max)) {
                        gameRegistry.healths[playerID].current = hp;
                    }
                }

                // Damage 
                if (gameRegistry.signatures[playerID].test(5)) {
                    
                }

            }
            else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Player Entity Not Found!");
            }
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // Cleanup 
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}