#pragma once
#include "ECS.h"
#include "SpatialHash.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>

// use multiple locks to allow different threads to write to different entities at the same time
const int NUM_LOCKS = 32;
std::mutex registryShards[NUM_LOCKS];

std::mutex& GetMutex(int entityID) {
    return registryShards[entityID % NUM_LOCKS];
}

int GetMutexIndex(int entityID) {
    return entityID % NUM_LOCKS;
}

void ProcessCollisionChunk(
    Registry* reg,
    int startIndex,
    int endIndex,
    SpatialHash* grid,
    float currentBallSize,
    float currentBallSpeed,
    std::vector<SDL_FPoint>* deferredSpawns,
    std::vector<int>* deferredDestroys
) {
    std::vector<int> potentialColliders;
    potentialColliders.reserve(64);

    for (int k = startIndex; k < endIndex; ++k) {
        int i = reg->entities_physics[k];

        // skip inactive or entities without colliders
        if (!reg->activeEntities[i]) continue;
        if (!reg->signatures[i].test(2)) continue;

        SDL_Rect rectA = reg->colliders[i].aabb;
        ColliderType typeA = reg->colliders[i].type;

        // limit results to avoid freezing if thousands of objects stack up
        grid->Query(rectA, potentialColliders, 32);

        int checks = 0;
        int max_checks = 16;

        for (int j : potentialColliders) {
            // stop checking if we hit the limit to keep frame rate stable
            if (checks > max_checks) break;
            if (i == j) continue;

            // read only check is thread safe
            if (!reg->activeEntities[j]) continue;

            SDL_Rect rectB = reg->colliders[j].aabb;

            // skip ball vs ball collisions to simulate fluid movement and save massive performance
            ColliderType typeB = reg->colliders[j].type;
            if (typeA == ColliderType::BALL && typeB == ColliderType::BALL) {
                continue;
            }

            if (SDL_HasIntersection(&rectA, &rectB)) {
                checks++;

                SDL_Rect intersection;
                SDL_IntersectRect(&rectA, &rectB, &intersection);

                // ball logic
                if (typeA == ColliderType::BALL) {
                    if (typeB != ColliderType::POWERUP) {

                        // optimization: update self without locks since this thread owns entity i
                        if (intersection.w < intersection.h) {
                            if (rectA.x < rectB.x) reg->transforms[i].x -= intersection.w;
                            else                   reg->transforms[i].x += intersection.w;
                            reg->rigidBodies[i].velocityX *= -1;
                        }
                        else {
                            if (rectA.y < rectB.y) reg->transforms[i].y -= intersection.h;
                            else                   reg->transforms[i].y += intersection.h;

                            if (typeB == ColliderType::PADDLE) {
                                // bounce based on where it hit the paddle
                                float paddleCenter = rectB.x + (rectB.w / 2.0f);
                                float ballCenter = rectA.x + (rectA.w / 2.0f);
                                float offset = (ballCenter - paddleCenter) / (rectB.w / 2.0f);
                                reg->rigidBodies[i].velocityX = offset * currentBallSpeed;
                                reg->rigidBodies[i].velocityY = -currentBallSpeed;
                            }
                            else {
                                reg->rigidBodies[i].velocityY *= -1;
                            }
                        }

                        reg->colliders[i].aabb.x = (int)reg->transforms[i].x;
                        reg->colliders[i].aabb.y = (int)reg->transforms[i].y;
                        rectA = reg->colliders[i].aabb;

                        // brick damage requires locking because another thread might hit this same brick
                        if (typeB == ColliderType::BRICK && reg->signatures[j].test(4)) {
                            std::lock_guard<std::mutex> lock(GetMutex(j));
                            if (reg->activeEntities[j]) {
                                reg->healths[j].current -= 10;
                            }
                        }
                    }
                }
                // powerup logic
                else if (typeA == ColliderType::POWERUP) {
                    if (typeB == ColliderType::PADDLE) {
                        // find all balls to duplicate them
                        for (int id : reg->entities_physics) {
                            if (reg->signatures[id].test(2) && reg->colliders[id].type == ColliderType::BALL) {
                                SDL_FPoint pos = { reg->transforms[id].x, reg->transforms[id].y };
                                deferredSpawns->push_back(pos);
                            }
                        }
                        deferredDestroys->push_back(i);
                        break;
                    }
                    else if (typeB == ColliderType::WALL && rectB.y > 600) {
                        deferredDestroys->push_back(i);
                        break;
                    }
                }
            }
        }
    }
}

SpatialHash* globalSpatialHash = nullptr;

void System_Collision_Threaded(Registry& reg, float currentBallSize, float currentBallSpeed, int screenWidth, int screenHeight) {

    if (globalSpatialHash == nullptr) {
        // 16 is a good cell size for high density
        globalSpatialHash = new SpatialHash(screenWidth, screenHeight, 16);
    }

    globalSpatialHash->Clear();

    // only insert static things into the hash to save time
    for (int id : reg.entities_collidable) {
        if (reg.activeEntities[id]) {
            globalSpatialHash->Insert(id, reg.colliders[id].aabb);
        }
    }

    int totalPhysics = (int)reg.entities_physics.size();
    if (totalPhysics == 0) return;

    // don't use threads if object count is low because creating threads is expensive
    int THREAD_THRESHOLD = 5000;

    if (totalPhysics < THREAD_THRESHOLD) {
        std::vector<SDL_FPoint> mainThreadSpawns;
        std::vector<int> mainThreadDestroys;

        ProcessCollisionChunk(&reg, 0, totalPhysics, globalSpatialHash, currentBallSize, currentBallSpeed, &mainThreadSpawns, &mainThreadDestroys);

        for (int id : mainThreadDestroys) reg.DestroyEntity(id);

        // check registry limit before spawning
        if (reg.entityCount + mainThreadSpawns.size() <= MAX_ENTITIES) {
            for (const auto& pos : mainThreadSpawns) {
                int newBall = reg.CreateEntity();
                if (newBall != -1) {
                    reg.AddTransform(newBall, pos.x, pos.y, (int)currentBallSize, (int)currentBallSize);
                    reg.AddRigidBody(newBall);
                    reg.AddCollider(newBall, ColliderType::BALL);
                    reg.AddRender(newBall, 255, 255, 0);

                    float dirX = ((rand() % 100) - 50) / 60.0f;
                    float dirY = -1.0f;
                    float length = std::sqrt(dirX * dirX + dirY * dirY);
                    if (length != 0) { dirX /= length; dirY /= length; }
                    reg.rigidBodies[newBall].velocityX = dirX * currentBallSpeed;
                    reg.rigidBodies[newBall].velocityY = dirY * currentBallSpeed;
                }
            }
        }
        return;
    }

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 2;

    int chunkSize = totalPhysics / numThreads;
    std::vector<std::thread> threads;
    std::vector<std::vector<SDL_FPoint>> threadSpawnLists(numThreads);
    std::vector<std::vector<int>> threadDestroyLists(numThreads);

    // split the work evenly across cores
    for (unsigned int t = 0; t < numThreads; ++t) {
        int start = t * chunkSize;
        int end = (t == numThreads - 1) ? totalPhysics : start + chunkSize;

        threads.emplace_back(
            ProcessCollisionChunk,
            &reg,
            start,
            end,
            globalSpatialHash,
            currentBallSize,
            currentBallSpeed,
            &threadSpawnLists[t],
            &threadDestroyLists[t]
        );
    }

    for (auto& t : threads) {
        t.join();
    }

    for (const auto& list : threadDestroyLists) {
        for (int id : list) {
            if (reg.activeEntities[id]) {
                reg.DestroyEntity(id);
            }
        }
    }

    int totalRequested = 0;
    for (const auto& list : threadSpawnLists) {
        totalRequested += list.size();
    }

    if (reg.entityCount + totalRequested <= MAX_ENTITIES) {
        for (const auto& list : threadSpawnLists) {
            for (const auto& pos : list) {
                int newBall = reg.CreateEntity();
                if (newBall != -1) {
                    float offsetX = (rand() % 5) - 2.0f;
                    float offsetY = (rand() % 5) - 2.0f;
                    reg.AddTransform(newBall, pos.x + offsetX, pos.y + offsetY, (int)currentBallSize, (int)currentBallSize);
                    reg.AddRigidBody(newBall);
                    reg.AddCollider(newBall, ColliderType::BALL);
                    reg.AddRender(newBall, 255, 255, 0);

                    float dirX = ((rand() % 100) - 50) / 60.0f;
                    float dirY = -1.0f;
                    float length = std::sqrt(dirX * dirX + dirY * dirY);
                    if (length != 0) { dirX /= length; dirY /= length; }

                    reg.rigidBodies[newBall].velocityX = dirX * currentBallSpeed;
                    reg.rigidBodies[newBall].velocityY = dirY * currentBallSpeed;
                }
            }
        }
    }
}

void System_Input(Registry& reg, const Uint8* keyboardState) {
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (reg.activeEntities[i] && reg.signatures[i].test(1) && reg.signatures[i].test(3)) {

            float speed = 500.0f;

            if (!reg.controllers[i].enabled) {
                reg.rigidBodies[i].velocityX = 0;
                continue;
            }

            reg.rigidBodies[i].velocityX = 0;
            if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
                reg.rigidBodies[i].velocityX = -speed;
            }
            if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
                reg.rigidBodies[i].velocityX = speed;
            }
        }
    }
}

void System_Physics(Registry& reg, float dt) {
    float gravity = 0.0f;

    for (int i : reg.entities_physics) {
        if (!reg.activeEntities[i]) continue;

        reg.rigidBodies[i].accelerationY = gravity;
        reg.rigidBodies[i].velocityY += reg.rigidBodies[i].accelerationY * dt;

        reg.transforms[i].x += reg.rigidBodies[i].velocityX * dt;
        reg.transforms[i].y += reg.rigidBodies[i].velocityY * dt;

        // keep paddle inside its specific zone
        if (reg.signatures[i].test(8)) {
            float minX = reg.paddleZones[i].minX;
            float maxX = reg.paddleZones[i].maxX;
            float minY = reg.paddleZones[i].minY;
            float maxY = reg.paddleZones[i].maxY;

            if (reg.transforms[i].x < minX) {
                reg.transforms[i].x = minX;
                reg.rigidBodies[i].velocityX = 0;
            }
            else if (reg.transforms[i].x > maxX) {
                reg.transforms[i].x = maxX;
                reg.rigidBodies[i].velocityX = 0;
            }

            if (reg.transforms[i].y < minY) {
                reg.transforms[i].y = minY;
                reg.rigidBodies[i].velocityY = 0;
            }
            else if (reg.transforms[i].y > maxY) {
                reg.transforms[i].y = maxY;
                reg.rigidBodies[i].velocityY = 0;
            }
        }

        if (reg.signatures[i].test(2)) {
            reg.colliders[i].aabb.x = static_cast<int>(reg.transforms[i].x);
            reg.colliders[i].aabb.y = static_cast<int>(reg.transforms[i].y);
            reg.colliders[i].aabb.w = reg.transforms[i].width;
            reg.colliders[i].aabb.h = reg.transforms[i].height;
        }
    }
}

void System_Render(Registry& reg, SDL_Renderer* renderer) {

    // batch rendering buffer for balls
    for (int i : reg.entities_renderable) {

        if (!reg.renderers[i].isVisible) continue;
        if (!reg.activeEntities[i]) continue;

        if (reg.signatures[i].test(4) && reg.healths[i].invulnTimer > 0.0f) {
            if (static_cast<int>(reg.healths[i].invulnTimer * 15.0f) % 2 == 0) continue;
        }

        SDL_Rect dstRect;
        dstRect.x = static_cast<int>(reg.transforms[i].x);
        dstRect.y = static_cast<int>(reg.transforms[i].y);
        dstRect.w = reg.transforms[i].width;
        dstRect.h = reg.transforms[i].height;

        if (reg.renderers[i].hasTexture) {
            SDL_RendererFlip flip = reg.renderers[i].flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, reg.renderers[i].texture, &reg.renderers[i].srcRect, &dstRect, 0.0, NULL, flip);
        }
        else {
            SDL_SetRenderDrawColor(renderer, reg.renderers[i].r, reg.renderers[i].g, reg.renderers[i].b, reg.renderers[i].a);
            SDL_RenderFillRect(renderer, &dstRect);
        }
    }
}

void System_Health(Registry& reg, float dt, int powerupChance, float powerupSize, float powerupSpeed) {
    for (int i = 0; i < MAX_ENTITIES; ++i) {

        if (reg.activeEntities[i] && reg.signatures[i].test(4)) {

            if (reg.healths[i].invulnTimer > 0.0f) {
                reg.healths[i].invulnTimer -= dt;
            }

            if (reg.healths[i].current <= 0) {

                bool isPlayer = false;
                if (reg.signatures[i].test(2)) {
                    if (reg.colliders[i].type == ColliderType::PADDLE ||
                        reg.colliders[i].type == ColliderType::PLAYER) {
                        isPlayer = true;
                    }
                }

                if (isPlayer) {
                    if (!reg.healths[i].isDead) {
                        reg.healths[i].isDead = true;
                        reg.healths[i].current = 0;
                        if (reg.signatures[i].test(6)) {
                            reg.renderers[i].r = 50;
                            reg.renderers[i].g = 50;
                            reg.renderers[i].b = 50;
                        }
                        if (reg.signatures[i].test(3)) {
                            reg.controllers[i].enabled = false;
                        }
                    }
                }
                else {
                    if ((rand() % 100) < powerupChance) {

                        int pu = reg.CreateEntity();
                        if (pu != -1) {
                            float x = reg.transforms[i].x;
                            float y = reg.transforms[i].y;

                            reg.AddTransform(pu, x, y, (int)powerupSize, (int)powerupSize);
                            reg.AddRigidBody(pu);
                            reg.AddCollider(pu, ColliderType::POWERUP);
                            reg.AddRender(pu, 0, 255, 255);
                            reg.AddPowerup(pu, 0);

                            reg.rigidBodies[pu].velocityX = 0;
                            reg.rigidBodies[pu].velocityY = powerupSpeed;
                            reg.rigidBodies[pu].accelerationY = 0;
                        }
                    }

                    reg.DestroyEntity(i);
                }
            }
        }
    }
}

void System_Bounds(Registry& reg, int screenWidth, int screenHeight) {

    std::vector<int> toDestroy;
    int buffer = 100;

    for (int i : reg.entities_physics) {
        float x = reg.transforms[i].x;
        float y = reg.transforms[i].y;

        bool outBottom = (y > screenHeight + buffer);
        bool outTop = (y < -buffer);
        bool outRight = (x > screenWidth + buffer);
        bool outLeft = (x < -buffer);

        if (outBottom || outTop || outRight || outLeft) {
            toDestroy.push_back(i);
        }
    }

    for (int id : toDestroy) {
        reg.DestroyEntity(id);
    }
}

bool System_CheckLevelClear(Registry& reg) {
    int brickCount = 0;

    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (reg.activeEntities[i] && reg.signatures[i].test(2)) {
            if (reg.colliders[i].type == ColliderType::BRICK) {
                brickCount++;
            }
        }
    }

    return (brickCount == 0);
}