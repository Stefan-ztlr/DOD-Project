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
        }
    }

    int CreateEntity() {
        for (int i = 0; i < MAX_ENTITIES; ++i) {
            if (!activeEntities[i]) {
                activeEntities[i] = true;
                signatures[i].reset(); // Clean old components
                entityCount++;
                return i;
            }
        }
        return -1; // No space left
    }

    void DestroyEntity(int entity) {
        if (entity >= 0 && entity < MAX_ENTITIES) {
            activeEntities[entity] = false;
            signatures[entity].reset();
            entityCount--;
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
    }

    void AddController(int entity) {
        controllers[entity] = { true, 0, 2, false };
        signatures[entity].set(3);
    }

    void AddHealth(int entity, int hp) {
        healths[entity] = { hp, hp, 0.0f, false };
        signatures[entity].set(4);
    }

    void AddDamage(int entity, int amount) {
        damages[entity] = { amount };
        signatures[entity].set(5);
    }

    void AddRender(int entity, Uint8 r, Uint8 g, Uint8 b) {
        // Texture=nullptr, src={0,0,0,0}, hasTexture=false, flipX=false
        renderers[entity] = { nullptr, {0,0,0,0}, false, false, r, g, b, 255, true };
        signatures[entity].set(6);
    }

    void AddSprite(int entity, const std::string& path, SDL_Renderer* renderer) {
        //  Load Texture via Manager 
        SDL_Texture* tex = TextureManager::LoadTexture(path, renderer);

        // Get Width/Height of the image automatically
        int w, h;
        SDL_QueryTexture(tex, NULL, NULL, &w, &h);
        SDL_Rect src = { 0, 0, w, h }; // Use the full image by default

        // hasTexture=true, flipX=false, Color defaults to White
        renderers[entity] = { tex, src, true, false, 255, 255, 255, 255, true };
        signatures[entity].set(6);
    }
};