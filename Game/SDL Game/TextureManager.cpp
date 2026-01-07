#include "TextureManager.h"
#include <iostream>

std::unordered_map<std::string, SDL_Texture*> TextureManager::textures;

SDL_Texture* TextureManager::LoadTexture(const std::string& filePath, SDL_Renderer* renderer) {
    // check if we already loaded this texture to save performance
    if (textures.find(filePath) == textures.end()) {

        SDL_Surface* tempSurface = IMG_Load(filePath.c_str());
        if (!tempSurface) {
            std::cerr << "Error loading image: " << IMG_GetError() << std::endl;
            return nullptr;
        }

        // convert the raw pixel data to a gpu optimized texture
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
        SDL_FreeSurface(tempSurface);

        if (!texture) {
            std::cerr << "Error creating texture from surface: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        textures[filePath] = texture;
    }

    return textures[filePath];
}

void TextureManager::CleanUp() {
    // make sure we free the gpu memory for all textures
    for (auto const& [key, val] : textures) {
        SDL_DestroyTexture(val);
    }
    textures.clear();
}