#include "ObjectOriented.h"

#include "ObjectOriented.h"

void Update_OO(std::vector<GameObject_OO>& objects, float deltaTime, int screenWidth, int screenHeight, SpatialHash& grid) {
    // Update position for every object
    for (auto& obj : objects) {
        obj.position.x += obj.velocity.x * deltaTime;
        obj.position.y += obj.velocity.y * deltaTime;
    }

    // Build spatial hash 
    grid.Clear();
    for (size_t i = 0; i < objects.size(); ++i) {
        // We use a temporary rect based on the current position to populate the grid
        SDL_Rect tempRect = { (int)objects[i].position.x, (int)objects[i].position.y, objects[i].rect.w, objects[i].rect.h };
        grid.Insert(tempRect, i);
    }

    // Collision Detection
    for (size_t i = 0; i < objects.size(); ++i) {
        // Wall collisions
        if (objects[i].position.x < 0) {
            objects[i].position.x = 0;
            objects[i].velocity.x = -objects[i].velocity.x;
        }
        else if (objects[i].position.x + objects[i].rect.w > screenWidth) {
            objects[i].position.x = screenWidth - objects[i].rect.w;
            objects[i].velocity.x = -objects[i].velocity.x;
        }
        if (objects[i].position.y < 0) {
            objects[i].position.y = 0;
            objects[i].velocity.y = -objects[i].velocity.y;
        }
        else if (objects[i].position.y + objects[i].rect.h > screenHeight) {
            objects[i].position.y = screenHeight - objects[i].rect.h;
            objects[i].velocity.y = -objects[i].velocity.y;
        }

        // Object-to-object collisions
        SDL_Rect queryRect = { (int)objects[i].position.x, (int)objects[i].position.y, objects[i].rect.w, objects[i].rect.h };
        std::vector<int> potentialColliders = grid.Query(queryRect);
        for (int otherId : potentialColliders) {
            if (otherId <= i) continue;

            SDL_Rect rectI = { (int)objects[i].position.x, (int)objects[i].position.y, objects[i].rect.w, objects[i].rect.h };
            SDL_Rect rectJ = { (int)objects[otherId].position.x, (int)objects[otherId].position.y, objects[otherId].rect.w, objects[otherId].rect.h };
            SDL_Rect intersection;

            if (SDL_IntersectRect(&rectI, &rectJ, &intersection)) {
                // Positional Correction
                if (intersection.w < intersection.h) {
                    float half_w = intersection.w / 2.0f;
                    if (objects[i].position.x < objects[otherId].position.x) {
                        objects[i].position.x -= half_w; objects[otherId].position.x += half_w;
                    }
                    else {
                        objects[i].position.x += half_w; objects[otherId].position.x -= half_w;
                    }
                }
                else {
                    float half_h = intersection.h / 2.0f;
                    if (objects[i].position.y < objects[otherId].position.y) {
                        objects[i].position.y -= half_h; objects[otherId].position.y += half_h;
                    }
                    else {
                        objects[i].position.y += half_h; objects[otherId].position.y -= half_h;
                    }
                }

                // Physics-Based Velocity Response
                float normalX = (objects[otherId].position.x + rectJ.w / 2.0f) - (objects[i].position.x + rectI.w / 2.0f);
                float normalY = (objects[otherId].position.y + rectJ.h / 2.0f) - (objects[i].position.y + rectI.h / 2.0f);
                float mag = sqrt(normalX * normalX + normalY * normalY);
                if (mag == 0.0f) mag = 1.0f;
                normalX /= mag; normalY /= mag;

                float p1 = objects[i].velocity.x * normalX + objects[i].velocity.y * normalY;
                float p2 = objects[otherId].velocity.x * normalX + objects[otherId].velocity.y * normalY;

                objects[i].velocity.x += (p2 - p1) * normalX;
                objects[i].velocity.y += (p2 - p1) * normalY;
                objects[otherId].velocity.x += (p1 - p2) * normalX;
                objects[otherId].velocity.y += (p1 - p2) * normalY;
            }
        }
    }

    // After physics is done sync positions to rects for rendering 
    for (auto& obj : objects) {
        obj.rect.x = static_cast<int>(obj.position.x);
        obj.rect.y = static_cast<int>(obj.position.y);
    }
}

void Render_OO(SDL_Renderer* renderer, const std::vector<GameObject_OO>& objects) {
    for (const auto& obj : objects) {
        SDL_RenderCopy(renderer, obj.texture, nullptr, &obj.rect);
    }
}