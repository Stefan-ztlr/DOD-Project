#include "SpatialHash.h"

SpatialHash::SpatialHash(int width, int height, int size)
    : screenWidth(width), screenHeight(height), cellSize(size) {
    gridWidth = screenWidth / cellSize + 1;
}

void SpatialHash::Insert(const SDL_Rect& rect, int objectId) {
    int startX = rect.x / cellSize;
    int startY = rect.y / cellSize;
    int endX = (rect.x + rect.w) / cellSize;
    int endY = (rect.y + rect.h) / cellSize;

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            grid[getCellIndex(x, y)].push_back(objectId);
        }
    }
}

std::vector<int> SpatialHash::Query(const SDL_Rect& rect) {
    std::vector<int> potentialColliders;
    int startX = rect.x / cellSize;
    int startY = rect.y / cellSize;
    int endX = (rect.x + rect.w) / cellSize;
    int endY = (rect.y + rect.h) / cellSize;

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            int index = getCellIndex(x, y);
            if (grid.count(index)) {
                potentialColliders.insert(potentialColliders.end(), grid[index].begin(), grid[index].end());
            }
        }
    }
    //This can return duplicates
   
    return potentialColliders;
}

void SpatialHash::Clear() {
    grid.clear();
}

int SpatialHash::getCellIndex(int x, int y) {
    return x + y * gridWidth;
}