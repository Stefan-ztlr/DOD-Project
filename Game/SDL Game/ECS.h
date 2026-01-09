#pragma once
#include <vector>
#include <bitset>
#include <queue>
#include "Components.h"
#include "TextureManager.h"

// Define a signature to know which components an entity has
// 0 = Transform, 1 = RigidBody, 2 = Collider, 3 = Controller, 4 = Health, 5 = Damage, 6 = Render
const int MAX_COMPONENTS = 32;
typedef std::bitset<MAX_COMPONENTS> Signature;

class Registry {
public:
    // Data Storage 
    TransformComponent transforms[MAX_ENTITIES];
    RigidBodyComponent rigidBodies[MAX_ENTITIES];
    ColliderComponent colliders[MAX_ENTITIES];
    PlayerControllerComponent controllers[MAX_ENTITIES];
    HealthComponent healths[MAX_ENTITIES];     
    DamageComponent damages[MAX_ENTITIES];    
    RenderComponent renderers[MAX_ENTITIES];
    PowerupComponent powerups[MAX_ENTITIES];
    PaddleZoneComponent paddleZones[MAX_ENTITIES];

    std::queue<int> freeIds;

    // Lists used for iteration by systems
    std::vector<int> entities_renderable;
    std::vector<int> entities_physics;
    std::vector<int> entities_collidable;
    std::vector<int> entities_with_health;

    // Management data
    Signature signatures[MAX_ENTITIES]; // Keeps track of which components are active for an entity
    bool activeEntities[MAX_ENTITIES];  // Is this ID currently in use?
    int entityCount = 0;

    // Entity Management
    Registry() {
        // Initialize all as inactive
        for (int i = 0; i < MAX_ENTITIES; ++i) {
            activeEntities[i] = false;
            signatures[i].reset();

            freeIds.push(i);
        }

        entities_physics.reserve(MAX_ENTITIES);
        entities_renderable.reserve(MAX_ENTITIES);
        entities_collidable.reserve(MAX_ENTITIES);
        entities_with_health.reserve(MAX_ENTITIES);
    }

    int CreateEntity() {
        if (freeIds.empty()) {
            return -1; // No space left
        }

        int id = freeIds.front(); // Grab the next available ID
        freeIds.pop();            // Remove it from the pile

        activeEntities[id] = true;
        signatures[id].reset();
        entityCount++;
        return id;
    }

    void DestroyEntity(int entity) {
        if (activeEntities[entity]) {
            activeEntities[entity] = false;
            signatures[entity].reset();
            entityCount--;

            freeIds.push(entity);
        }
    }

    void RefreshLists() {
        // Helper lambda to remove inactive IDs from a vector
        auto cleanVector = [&](std::vector<int>& list) {
            // This is the "Erase-Remove Idiom". It packs the vector in one pass. Very fast.
            list.erase(std::remove_if(list.begin(), list.end(),
                [&](int id) { return !activeEntities[id]; }),
                list.end());
            };

        cleanVector(entities_physics);
        cleanVector(entities_collidable);
        cleanVector(entities_renderable);
        cleanVector(entities_with_health);
    }


    void RemoveFromList(std::vector<int>& list, int entityID) {
        for (int i = 0; i < list.size(); ++i) {
            if (list[i] == entityID) {
                // Swap with the last element
                list[i] = list.back();
                // Remove the last element
                list.pop_back();
                return;
            }
        }
    }

    // These just set the bit in the signature so Systems know to process this entity
    void AddTransform(int entity, float x, float y, int w, int h) {
        transforms[entity] = { x, y, w, h };
        signatures[entity].set(0);
    }

    void AddRigidBody(int entity) {
        rigidBodies[entity] = { 0.0f, 0.0f, 0.0f, 0.0f, false };
        signatures[entity].set(1);
        entities_physics.push_back(entity); // <--- Cache it!
    }

    void AddCollider(int entity, ColliderType type) {
        colliders[entity].type = type;

        // Check if this entity has a Transform (Bit 0)
        // If it does, we automatically sync the Collider size to the Transform size
        if (signatures[entity].test(0)) {
            colliders[entity].aabb.x = static_cast<int>(transforms[entity].x);
            colliders[entity].aabb.y = static_cast<int>(transforms[entity].y);
            colliders[entity].aabb.w = transforms[entity].width;
            colliders[entity].aabb.h = transforms[entity].height;
        }
        else {
            // Fallback 
            colliders[entity].aabb = { 0, 0, 0, 0 };
        }

        signatures[entity].set(2);
        entities_collidable.push_back(entity);
    }

    void AddController(int entity) {
        controllers[entity] = { true, 0, 2, false };
        signatures[entity].set(3);
    }

    void AddHealth(int entity, int hp) {
        healths[entity] = { hp, hp, 0.0f, false };
        signatures[entity].set(4);
        entities_with_health.push_back(entity);
    }

    void AddDamage(int entity, int amount) {
        damages[entity] = { amount };
        signatures[entity].set(5);
    }

    void AddRender(int entity, Uint8 r, Uint8 g, Uint8 b) {
        // Texture=nullptr, src={0,0,0,0}, hasTexture=false, flipX=false
        renderers[entity] = { nullptr, {0,0,0,0}, false, false, r, g, b, 255, true };

        // ONLY push to list if it wasn't already a renderable entity
        if (!signatures[entity].test(6)) {
            entities_renderable.push_back(entity);
            signatures[entity].set(6);
        }
    }

    void AddSprite(int entity, const std::string& path, SDL_Renderer* renderer) {
        //  Load Texture via Manager 
        SDL_Texture* tex = TextureManager::LoadTexture(path, renderer);
        int w, h;
        SDL_QueryTexture(tex, NULL, NULL, &w, &h);
        SDL_Rect src = { 0, 0, w, h };

        renderers[entity] = { tex, src, true, false, 255, 255, 255, 255, true };

        // ONLY push to list if it wasn't already a renderable entity
        if (!signatures[entity].test(6)) {
            entities_renderable.push_back(entity);
            signatures[entity].set(6);
        }
    }

    void AddPowerup(int entity, int type) {
        powerups[entity] = { type };
        signatures[entity].set(7);
    }

    void AddPaddleZone(int entity, float minX, float maxX, float minY, float maxY) {
        paddleZones[entity] = { minX, maxX, minY, maxY };
        signatures[entity].set(8);
    }
};