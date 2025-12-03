#pragma once
#include "ECS.h"
#include <iostream>

// Processes keyboard input for entities with a Controller component
void System_Input(Registry& reg, const Uint8* keyboardState) {
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        if (reg.activeEntities[i] && reg.signatures[i].test(1) && reg.signatures[i].test(3)) {

            float speed = 200.0f;
            float jumpForce = -400.0f;

            if (!reg.controllers[i].enabled) {
                reg.rigidBodies[i].velocityX = 0;
                continue;
            }

            // Horizontal Movement
            reg.rigidBodies[i].velocityX = 0;
            if (keyboardState[SDL_SCANCODE_A] || keyboardState[SDL_SCANCODE_LEFT]) {
                reg.rigidBodies[i].velocityX = -speed;

                if (reg.signatures[i].test(6)) {
                    reg.renderers[i].flipX = true;
                }
            }
            if (keyboardState[SDL_SCANCODE_D] || keyboardState[SDL_SCANCODE_RIGHT]) {
                reg.rigidBodies[i].velocityX = speed;

                if (reg.signatures[i].test(6)) {
                    reg.renderers[i].flipX = false;
                }
            }

            // Jump Logic
            bool isJumpDown = keyboardState[SDL_SCANCODE_SPACE] || keyboardState[SDL_SCANCODE_W];
            bool wasJumpDown = reg.controllers[i].wasJumpPressed;

            // Only act if key was JUST pressed 
            if (isJumpDown && !wasJumpDown) {

                // First Jump (from ground)
                if (reg.rigidBodies[i].isGrounded) {
                    reg.rigidBodies[i].velocityY = jumpForce;
                    reg.rigidBodies[i].isGrounded = false;
                    reg.controllers[i].currentJumps = 1; 
                }
                // Double Jump (in air)
                else if (reg.controllers[i].currentJumps < reg.controllers[i].maxJumps) {
                    reg.rigidBodies[i].velocityY = jumpForce; 
                    reg.controllers[i].currentJumps++; 
                }
            }

            // Store current state for next frame
            reg.controllers[i].wasJumpPressed = isJumpDown;
        }
    }
}


// Applies gravity and moves entities based on velocity
void System_Physics(Registry& reg, float dt) {
    float gravity = 980.0f;

    for (int i : reg.entities_physics) {

        if (!reg.activeEntities[i]) continue;

        // Apply Gravity
        reg.rigidBodies[i].accelerationY = gravity;

        // Apply Acceleration
        reg.rigidBodies[i].velocityY += reg.rigidBodies[i].accelerationY * dt;

        // Apply Velocity
        reg.transforms[i].x += reg.rigidBodies[i].velocityX * dt;
        reg.transforms[i].y += reg.rigidBodies[i].velocityY * dt;

        // Update the AABB box for collision detection later
        if (reg.signatures[i].test(2)) {
            reg.colliders[i].aabb.x = static_cast<int>(reg.transforms[i].x);
            reg.colliders[i].aabb.y = static_cast<int>(reg.transforms[i].y);    
            reg.colliders[i].aabb.w = reg.transforms[i].width;
            reg.colliders[i].aabb.h = reg.transforms[i].height;
        }
        
    }
}

// Handles Platform collision and Enemy damage 
// Probably should split the logic between collision and enemy damage idk
void System_Collision(Registry& reg) {
    // Requirement: Active + RigidBody(1) + Collider(2)
    for (int i : reg.entities_physics) {

        if (!reg.signatures[i].test(2) && !reg.activeEntities[i]) continue;

        // Reset grounded state for this entity
        //reg.rigidBodies[i].isGrounded = false;
        SDL_Rect rectA = reg.colliders[i].aabb;

        // Requirement: Active + Collider(2)
        for (int j : reg.entities_collidable) {
            // Skip self
            if (i == j) continue;

            SDL_Rect rectB = reg.colliders[j].aabb;

            if (SDL_HasIntersection(&rectA, &rectB)) {

                // Hitting a Solid Platform
                if (reg.colliders[j].type == ColliderType::PLATFORM) {
                    SDL_Rect intersection;
                    SDL_IntersectRect(&rectA, &rectB, &intersection);

                    // Vertical Collision
                    if (intersection.w > intersection.h) {
                        float velY = reg.rigidBodies[i].velocityY;

                        if (velY > 0) {
                            // Landing on top
                            reg.transforms[i].y -= intersection.h;
                            reg.rigidBodies[i].velocityY = 0;
                            reg.rigidBodies[i].isGrounded = true;

                            // If this entity has a controller (like the Player), reset jumps
                            if (reg.signatures[i].test(3)) {
                                reg.controllers[i].currentJumps = 0;
                            }
                        }
                        else if (velY < 0) {
                            // Hitting head
                            reg.transforms[i].y += intersection.h;
                            reg.rigidBodies[i].velocityY = 0;
                        }
                    }
                    // Horizontal Collision
                    else {
                        if (rectA.x < rectB.x) {
                            reg.transforms[i].x -= intersection.w;
                        }
                        else {
                            reg.transforms[i].x += intersection.w;
                        }
                        reg.rigidBodies[i].velocityX = 0;
                    }

                    // Update the AABB immediately so subsequent checks in this frame represent the new position
                    reg.colliders[i].aabb.x = (int)reg.transforms[i].x;
                    reg.colliders[i].aabb.y = (int)reg.transforms[i].y;
                    rectA = reg.colliders[i].aabb; 
                }

                // Only happens if 'i' has Stats (can take damage) and 'j' has Stats (can deal damage)
                if (reg.signatures[j].test(5) && reg.signatures[i].test(4)) {

                    if (!reg.healths[i].isDead && reg.healths[i].invulnTimer <= 0.0f) {

                        // Apply Damage
                        int dmg = reg.damages[j].amount;
                        reg.healths[i].current -= dmg;

                        // Reset Timer (Invulnerability)
                        reg.healths[i].invulnTimer = 2.0f;
                    }
                }
                
            }
        }
    }
}

// Render System 
void System_Render(Registry& reg, SDL_Renderer* renderer) {
    for (int i : reg.entities_renderable) {
        
        if (!reg.renderers[i].isVisible) continue;

        // Invulnerability Blink Logic 
        if (reg.signatures[i].test(4) && reg.healths[i].invulnTimer > 0.0f) {
            if (static_cast<int>(reg.healths[i].invulnTimer * 15.0f) % 2 == 0) continue;
        }

        // Destination Rectangle 
        SDL_Rect dstRect;
        dstRect.x = static_cast<int>(reg.transforms[i].x);
        dstRect.y = static_cast<int>(reg.transforms[i].y);
        dstRect.w = reg.transforms[i].width;
        dstRect.h = reg.transforms[i].height;

        // Render Logic
        if (reg.renderers[i].hasTexture) {
            // Check flip flag
            SDL_RendererFlip flip = reg.renderers[i].flipX ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

            // RenderCopyEx allows rotation (0.0) and flipping
            SDL_RenderCopyEx(renderer, reg.renderers[i].texture, &reg.renderers[i].srcRect, &dstRect, 0.0, NULL, flip);
        }
        else {
            // Fallback
            SDL_SetRenderDrawColor(renderer, reg.renderers[i].r, reg.renderers[i].g, reg.renderers[i].b, reg.renderers[i].a);
            SDL_RenderFillRect(renderer, &dstRect);
        }
        
    }
}

void System_Health(Registry& reg, float dt) {
    for (int i = 0; i < MAX_ENTITIES; ++i) {
        // Only run for entities with Health Component (Bit 4)
        if (reg.activeEntities[i] && reg.signatures[i].test(4)) {

            // Countdown Invulnerability
            if (reg.healths[i].invulnTimer > 0.0f) {
                reg.healths[i].invulnTimer -= dt;
            }

            // Check Death
            if (reg.healths[i].current <= 0 && !reg.healths[i].isDead) {
                reg.healths[i].isDead = true;
                reg.healths[i].current = 0;

                // Visual feedback 
                if (reg.signatures[i].test(6)) {
                    reg.renderers[i].r = 100;
                    reg.renderers[i].g = 100;
                    reg.renderers[i].b = 100;
                }

                // Disable Controller
                if (reg.signatures[i].test(3)) {
                    reg.controllers[i].enabled = false;
                }
            }
        }
    }
}