#pragma once
#include <vector>
#include <unordered_map>
#include <SDL.h>

class SpatialHash {
public:
    SpatialHash(int screenWidth, int screenHeight, int cellSize);

    void Insert(const SDL_Rect& rect, int objectId);
    // Gets potential colliders for a given rect
    std::vector<int> Query(const SDL_Rect& rect);
    void Clear();

private:
    int getCellIndex(int x, int y);

    int screenWidth;
    int screenHeight;
    int cellSize;
    int gridWidth;
    std::unordered_map<int, std::vector<int>> grid;
};