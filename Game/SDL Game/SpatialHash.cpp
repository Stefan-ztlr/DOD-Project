#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "ECS.h"

class SpatialHash {
private:
    int cellSize;
    int cols;
    int rows;

    // The Grid: A vector of vectors. 
    // Index = cell_x + cell_y * cols
    std::vector<std::vector<int>> cells;

public:
    SpatialHash(int screenW, int screenH, int cellSize)
        : cellSize(cellSize) {
        cols = std::ceil((float)screenW / cellSize);
        rows = std::ceil((float)screenH / cellSize);

        // Resize grid once
        cells.resize(cols * rows);

        // Reserve memory for each cell to avoid allocation during runtime
        for (auto& cell : cells) {
            cell.reserve(16); // Expect roughly 16 entities max per cell
        }
    }

    void Clear() {
        for (auto& cell : cells) {
            cell.clear();
        }
    }

    // Insert an entity into the cells it overlaps
    void Insert(int entityId, const SDL_Rect& rect) {
        int minX = std::max(0, rect.x / cellSize);
        int minY = std::max(0, rect.y / cellSize);
        int maxX = std::min(cols - 1, (rect.x + rect.w) / cellSize);
        int maxY = std::min(rows - 1, (rect.y + rect.h) / cellSize);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                cells[x + y * cols].push_back(entityId);
            }
        }
    }

    // Retrieve potential colliders from relevant cells
    // We pass a reference to a reusable vector to avoid allocation
    void Query(const SDL_Rect& rect, std::vector<int>& results) {
        results.clear();

        int minX = std::max(0, rect.x / cellSize);
        int minY = std::max(0, rect.y / cellSize);
        int maxX = std::min(cols - 1, (rect.x + rect.w) / cellSize);
        int maxY = std::min(rows - 1, (rect.y + rect.h) / cellSize);

        // We use a simple ID check or sort/unique later to avoid duplicates
        // For raw speed in this specific case, raw insertion is often faster 
        // than checking duplicates if object density is low.

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const auto& cell = cells[x + y * cols];
                results.insert(results.end(), cell.begin(), cell.end());
            }
        }
    }
};