#pragma once
#include <vector>
#include <SDL.h>
#include "SpatialHash.h"


struct PhysicsComponent {
    SDL_FPoint position;
    SDL_FPoint velocity;
    SDL_Rect rect;
};

struct GameData_DO {
    
    std::vector<PhysicsComponent> physics;

 
    std::vector<SDL_Texture*> textures;
};

void Update_DO(GameData_DO& data, float deltaTime, int screenWidth, int screenHeight, SpatialHash& spatialHash);
void Render_DO(SDL_Renderer* renderer, const GameData_DO& data);