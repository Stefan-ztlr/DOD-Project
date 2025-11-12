#pragma once
#include <vector>
#include <SDL.h>
#include "SpatialHash.h"
#include <iostream>
#include <string.h>

class GameObject_OO {
public:
    SDL_FPoint position; 
    SDL_FPoint velocity;
    SDL_Rect rect;       
    SDL_Texture* texture;
};


void Update_OO(std::vector<GameObject_OO>& objects, float deltaTime, int screenWidth, int screenHeight, SpatialHash& spatialHash);
void Render_OO(SDL_Renderer* renderer, const std::vector<GameObject_OO>& objects);