#pragma once
#include <string>
#include <unordered_map>
#include <SDL.h>
#include <SDL_image.h>

class TextureManager {
public:
    static SDL_Texture* LoadTexture(const std::string& filePath, SDL_Renderer* renderer);
    static void CleanUp();

private:
    // simple cache to prevent loading the same image from disk multiple times
    static std::unordered_map<std::string, SDL_Texture*> textures;
};