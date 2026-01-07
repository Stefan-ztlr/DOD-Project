#pragma once
#include <SDL.h>

const int MAX_ENTITIES = 100000;

struct TransformComponent {
    float x, y;
    int width, height;
};

struct RigidBodyComponent {
    float velocityX, velocityY;
    float accelerationX, accelerationY;
    bool isGrounded; // mainly used if we enable gravity logic
};

enum class ColliderType {
    NONE,
    PADDLE,
    PLAYER,
    BALL,
    WALL,
    BRICK,
    POWERUP
};

struct ColliderComponent {
    ColliderType type;
    SDL_Rect aabb;
};

struct PlayerControllerComponent {
    bool enabled;
    int currentJumps;
    int maxJumps;
    bool wasJumpPressed; // input debounce
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
    bool hasTexture; // switches between sprite drawing and simple color rectangles
    bool flipX;      // for facing left or right

    Uint8 r, g, b, a;
    bool isVisible;
};

struct PowerupComponent {
    int type; // currently only 0 for MultiBall 
};

struct PaddleZoneComponent {
    // defines the box that the paddle is allowed to move inside
    float minX, maxX;
    float minY, maxY;
};