#include <iostream>
#include <ctime>
#include <cmath> 
#include <vector>
#include <random>
#include <chrono>
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "TextureManager.h"
#include "ObjectOriented.h"
#include "DataOriented.h"
#include "SpatialHash.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Configuration
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int MAX_OBJECTS = 100000;
const int SPRITE_SIZE = 1;
const int CELL_SIZE = SPRITE_SIZE * 8;

const float OBJECT_SPEED = 200.0f; // Define a constant speed

int main(int argc, char* argv[]) {
    //SDL Initialization
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("DOD Project", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    //ImGui Initialization
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    SpatialHash spatialHash(SCREEN_WIDTH, SCREEN_HEIGHT, CELL_SIZE);

    //Game Variables
    bool useDataOriented = false;
    int numObjects = 5000;
    std::vector<GameObject_OO> objects_OO;
    GameData_DO gameData_DO;
    SDL_Texture* spriteTexture = TextureManager::LoadTexture("sprite.png", renderer);

    if (!spriteTexture) {
        return 1;
    }

    auto respawnObjects = [&]() {
        objects_OO.clear();

        gameData_DO.physics.clear(); 
        gameData_DO.textures.clear();


        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distribX(0, SCREEN_WIDTH - SPRITE_SIZE);
        std::uniform_int_distribution<> distribY(0, SCREEN_HEIGHT - SPRITE_SIZE);

        std::uniform_real_distribution<float> distribAngle(0.0f, 2.0f * static_cast<float>(M_PI));

        for (int i = 0; i < numObjects; ++i) {
            SDL_Rect newRect;
            bool positionIsSafe;
            int maxTries = 100;

            do {
                
                positionIsSafe = true;
                newRect = { distribX(gen), distribY(gen), SPRITE_SIZE, SPRITE_SIZE };
                if (useDataOriented) {
                    for (const auto& comp : gameData_DO.physics) { 
                        if (SDL_HasIntersection(&newRect, &comp.rect)) {
                            positionIsSafe = false;
                            break;
                        }
                    }
                }
                else { 
                    for (const auto& existingObj : objects_OO) {
                        if (SDL_HasIntersection(&newRect, &existingObj.rect)) {
                            positionIsSafe = false;
                            break;
                        }
                    }
                }
                maxTries--;
            } while (!positionIsSafe && maxTries > 0);

            if (positionIsSafe) {
                float angle = distribAngle(gen);
                SDL_FPoint vel = { OBJECT_SPEED * cos(angle), OBJECT_SPEED * sin(angle) };

                if (useDataOriented) {
                    SDL_FPoint pos = { (float)newRect.x, (float)newRect.y };
                    gameData_DO.physics.push_back({ pos, vel, newRect }); // Add the new component
                    gameData_DO.textures.push_back(spriteTexture);
                }
                else {
                    objects_OO.push_back({ {(float)newRect.x, (float)newRect.y}, vel, newRect, spriteTexture });
                }
            }
        }
        };

    respawnObjects();

    //Main Loop
    bool quit = false;
    SDL_Event e;
    auto lastTime = std::chrono::high_resolution_clock::now();
    float fps = 0.0f;
    Uint32 frameCount = 0;
    Uint32 startTime = SDL_GetTicks();

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        //FPS Calculation 
        frameCount++;
        if (SDL_GetTicks() - startTime > 1000) {
            fps = frameCount;
            frameCount = 0;
            startTime = SDL_GetTicks();
        }

        //Logic Update
        if (useDataOriented) {
            Update_DO(gameData_DO, deltaTime, SCREEN_WIDTH, SCREEN_HEIGHT, spatialHash);
        }
        else {
            Update_OO(objects_OO, deltaTime, SCREEN_WIDTH, SCREEN_HEIGHT, spatialHash);
        }

        //UI (ImGui)
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Control Panel");
        ImGui::Text("Performance:");
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Separator();
        ImGui::Text("Simulation Settings:");

        if (ImGui::Checkbox("Use Data-Oriented", &useDataOriented)) {
            respawnObjects();
        }

        if (ImGui::SliderInt("Number of Objects", &numObjects, 1, MAX_OBJECTS)) {
            respawnObjects();
        }
        ImGui::End();

        //Rendering
        SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255);
        SDL_RenderClear(renderer);

        if (useDataOriented) {
            Render_DO(renderer, gameData_DO);
        }
        else {
            Render_OO(renderer, objects_OO);
        }

        ImGui::Render();
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    //Cleanup 
    TextureManager::CleanUp();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
};