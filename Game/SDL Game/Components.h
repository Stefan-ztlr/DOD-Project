#pragma once
#include <SDL.h>

const int MAX_ENTITIES = 50000;

struct TransformComponent {
    float x, y;
    int width, height;
};

struct RigidBodyComponent {
    float velocityX, velocityY;
    float accelerationX, accelerationY;
    bool isGrounded; 
};

enum class ColliderType {
    NONE,
    PLAYER,
    PLATFORM,
    ENEMY
};

struct ColliderComponent {
    ColliderType type;
    SDL_Rect aabb; 
};

struct PlayerControllerComponent {
    bool enabled;
    int currentJumps;    
    int maxJumps;       
    bool wasJumpPressed; 
};

struct HealthComponent {
    int current;
    int max;
    float invulnTimer;
    bool isDead;
};

struct DamageComponent {
    int amount;
};

struct RenderComponent {
    SDL_Texture* texture;   
    SDL_Rect srcRect;      
    bool hasTexture;        // True = Draw Sprite, False = Draw Color Rect
    bool flipX;             // True = Face Left, False = Face Right

    Uint8 r, g, b, a;       // Color (used if hasTexture is false)
    bool isVisible;
};