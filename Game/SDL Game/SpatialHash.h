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

    // using a 1D vector to represent the 2D grid improves cache locality
    std::vector<std::vector<int>> cells;

public:
    SpatialHash(int screenW, int screenH, int cellSize)
        : cellSize(cellSize) {
        cols = std::ceil((float)screenW / cellSize);
        rows = std::ceil((float)screenH / cellSize);

        cells.resize(cols * rows);

        // pre-allocate memory to avoid resizing during gameplay
        for (auto& cell : cells) {
            cell.reserve(16);
        }
    }

    void Clear() {
        for (auto& cell : cells) {
            cell.clear();
        }
    }

    void Insert(int entityId, const SDL_Rect& rect) {
        // determine the range of cells this object overlaps
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

    // finds all entities that share cells with the given rect
    void Query(const SDL_Rect& rect, std::vector<int>& results, int max_results) {
        results.clear();

        int minX = std::max(0, rect.x / cellSize);
        int minY = std::max(0, rect.y / cellSize);
        int maxX = std::min(cols - 1, (rect.x + rect.w) / cellSize);
        int maxY = std::min(rows - 1, (rect.y + rect.h) / cellSize);

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const auto& cell = cells[x + y * cols];

                // stop if we found enough neighbors to avoid performance drops
                if (results.size() >= max_results) return;

                int spaceLeft = max_results - (int)results.size();
                int countToCopy = std::min(spaceLeft, (int)cell.size());

                results.insert(results.end(), cell.begin(), cell.begin() + countToCopy);
            }
        }
    }
};