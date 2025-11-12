#include "DataOriented.h"

void Update_DO(GameData_DO& data, float deltaTime, int screenWidth, int screenHeight, SpatialHash& grid) {
    // Update position for every object
    for (size_t i = 0; i < data.physics.size(); ++i) {
        data.physics[i].position.x += data.physics[i].velocity.x * deltaTime;
        data.physics[i].position.y += data.physics[i].velocity.y * deltaTime;
    }

    // Build spatial hash 
    grid.Clear();
    for (size_t i = 0; i < data.physics.size(); ++i) {
        SDL_Rect tempRect = { (int)data.physics[i].position.x, (int)data.physics[i].position.y, data.physics[i].rect.w, data.physics[i].rect.h };
        grid.Insert(tempRect, i);
    }

    // Collision Detection
    for (size_t i = 0; i < data.physics.size(); ++i) {
        // Wall collisions
        if (data.physics[i].position.x < 0) {
            data.physics[i].position.x = 0;
            data.physics[i].velocity.x = -data.physics[i].velocity.x;
        }
        else if (data.physics[i].position.x + data.physics[i].rect.w > screenWidth) {
            data.physics[i].position.x = screenWidth - data.physics[i].rect.w;
            data.physics[i].velocity.x = -data.physics[i].velocity.x;
        }
        if (data.physics[i].position.y < 0) {
            data.physics[i].position.y = 0;
            data.physics[i].velocity.y = -data.physics[i].velocity.y;
        }
        else if (data.physics[i].position.y + data.physics[i].rect.h > screenHeight) {
            data.physics[i].position.y = screenHeight - data.physics[i].rect.h;
            data.physics[i].velocity.y = -data.physics[i].velocity.y;
        }

        // Object-to-object collisions
        SDL_Rect queryRect = { (int)data.physics[i].position.x, (int)data.physics[i].position.y, data.physics[i].rect.w, data.physics[i].rect.h };
        std::vector<int> potentialColliders = grid.Query(queryRect);
        for (int otherId : potentialColliders) {
            if (otherId <= i) continue;

            SDL_Rect rectI = { (int)data.physics[i].position.x, (int)data.physics[i].position.y, data.physics[i].rect.w, data.physics[i].rect.h };
            SDL_Rect rectJ = { (int)data.physics[otherId].position.x, (int)data.physics[otherId].position.y, data.physics[otherId].rect.w, data.physics[otherId].rect.h };
            SDL_Rect intersection;

            if (SDL_IntersectRect(&rectI, &rectJ, &intersection)) {
                // Positional Correction
                if (intersection.w < intersection.h) {
                    float half_w = intersection.w / 2.0f;
                    if (data.physics[i].position.x < data.physics[otherId].position.x) {
                        data.physics[i].position.x -= half_w; data.physics[otherId].position.x += half_w;
                    }
                    else {
                        data.physics[i].position.x += half_w; data.physics[otherId].position.x -= half_w;
                    }
                }
                else {
                    float half_h = intersection.h / 2.0f;
                    if (data.physics[i].position.y < data.physics[otherId].position.y) {
                        data.physics[i].position.y -= half_h; data.physics[otherId].position.y += half_h;
                    }
                    else {
                        data.physics[i].position.y += half_h; data.physics[otherId].position.y -= half_h;
                    }
                }

                // Physics-Based Velocity Response
                float normalX = (data.physics[otherId].position.x + rectJ.w / 2.0f) - (data.physics[i].position.x + rectI.w / 2.0f);
                float normalY = (data.physics[otherId].position.y + rectJ.h / 2.0f) - (data.physics[i].position.y + rectI.h / 2.0f);
                float mag = sqrt(normalX * normalX + normalY * normalY);
                if (mag == 0.0f) mag = 1.0f;
                normalX /= mag; normalY /= mag;

                float p1 = data.physics[i].velocity.x * normalX + data.physics[i].velocity.y * normalY;
                float p2 = data.physics[otherId].velocity.x * normalX + data.physics[otherId].velocity.y * normalY;

                data.physics[i].velocity.x += (p2 - p1) * normalX;
                data.physics[i].velocity.y += (p2 - p1) * normalY;
                data.physics[otherId].velocity.x += (p1 - p2) * normalX;
                data.physics[otherId].velocity.y += (p1 - p2) * normalY;
            }
        }
    }

    // After physics is done sync positions to rects for rendering 
    for (size_t i = 0; i < data.physics.size(); ++i) {
        data.physics[i].rect.x = static_cast<int>(data.physics[i].position.x);
        data.physics[i].rect.y = static_cast<int>(data.physics[i].position.y);
    }
}

void Render_DO(SDL_Renderer* renderer, const GameData_DO& data) {
    for (size_t i = 0; i < data.physics.size(); ++i) {
        // The render loop now has to get the rect from the physics component
        SDL_RenderCopy(renderer, data.textures[i], nullptr, &data.physics[i].rect);
    }
}