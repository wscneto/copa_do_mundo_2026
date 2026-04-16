#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "GameInternal.h"

using namespace game_internal;

void Game::updateBallControl(float dt) {
    if (ballControlCooldown_ > 0.0f) {
        ballControlCooldown_ = std::max(0.0f, ballControlCooldown_ - dt);
    }

    // Keyboard state is converted into a 2D direction vector each frame.
    Vec2 movementDir(0.0f, 0.0f);

    if (keys_['w'] || specialKeys_[GLUT_KEY_UP]) {
        movementDir.y += 1.0f;
    }
    if (keys_['s'] || specialKeys_[GLUT_KEY_DOWN]) {
        movementDir.y -= 1.0f;
    }
    if (keys_['a'] || specialKeys_[GLUT_KEY_LEFT]) {
        movementDir.x -= 1.0f;
    }
    if (keys_['d'] || specialKeys_[GLUT_KEY_RIGHT]) {
        movementDir.x += 1.0f;
    }

    // Normalize movement direction if there's input
    if (movementDir.lengthSquared() > 0.0f) {
        ballInputDirection_ = movementDir.normalized();
    } else {
        ballInputDirection_ = Vec2(0.0f, 0.0f); // No input, no desired direction
    }

    // Apply direct input to the ball's velocity if not charging a shot
    if (ballInputDirection_.lengthSquared() > 0.0f && !ballShootHeld_) {
        const float controlAcceleration = tuning_.dribbleCarrySpeed * 2.4f;
        ball_.velocity += ballInputDirection_ * (controlAcceleration * dt);
        // Cap ball speed from direct input
        const float maxInputSpeed = tuning_.dribbleCarrySpeed * 2.2f;
        if (ball_.velocity.lengthSquared() > maxInputSpeed * maxInputSpeed) {
            ball_.velocity = ball_.velocity.normalized() * maxInputSpeed;
        }

        // When ball is directly controlled, Team A has possession for AI purposes
        possessionTeam_ = TeamSide::A;
        possessionActive_ = true;
        possessionCooldown_ = 0.20f; // Give a small grace period for AI to react
    } else if (!ballShootHeld_) {
        // If no direct movement input and not charging, player is not actively controlling
        // AI will determine possession based on proximity
        possessionActive_ = false;
    }

    // Charge-and-release shot model: hold SPACE to fill power, release to kick.
    if (ballShootHeld_) {
        shotCharge_ = clampf(shotCharge_ + dt * 1.15f, 0.0f, 1.0f);
        possessionTeam_ = TeamSide::A; // Player is charging, so Team A has possession
        possessionActive_ = true;
        possessionCooldown_ = 0.20f;
    } else {
        shotCharge_ = std::max(0.0f, shotCharge_ - dt * 0.85f);
    }

    if (ballRequestShoot_ && !gameOver_ && ballControlCooldown_ <= 0.0f) {
        const float charge = clampf(shotCharge_, 0.0f, 1.0f);
        const float baseSpeed = tuning_.shotMinSpeed + (tuning_.shotMaxSpeed - tuning_.shotMinSpeed) * charge;

        // Determine shot direction based on current ball input direction
        Vec2 shotDirection = ballInputDirection_;
        if (shotDirection.lengthSquared() < 1e-6f) {
            // If no input direction, default to shooting towards opponent's goal
            shotDirection = Vec2(1.0f, 0.0f);
        }
        shotDirection = shotDirection.normalized();

        // Low charge has more angular error; full charge is stronger and straighter.
        const float errorFactor = (1.0f - charge) * 0.15f; // Reduced error for direct ball control
        const float deterministicNoise = std::sin(matchTimeSeconds_ * 8.3f);
        shotDirection.y += deterministicNoise * errorFactor;
        shotDirection = shotDirection.normalized();

        // Apply kick to ball
        ball_.velocity = shotDirection * baseSpeed;
        // Add a slight forward bias to the kick
        if (shotDirection.x > 0) {
             ball_.velocity.x += 3.0f;
        } else {
            ball_.velocity.x += 1.0f; // Small push even backwards
        }

        // Reset possession related flags after kick
        possessionActive_ = false; // Ball is loose after player kick

        pendingKickSound_ = true;
        crowdExcitement_ = std::max(crowdExcitement_, 0.45f + charge * 0.4f);

        // Prevent immediate re-kick
        ballControlCooldown_ = 0.42f;

        // After a shot, the ball is considered loose
        possessionTeam_ = TeamSide::A; // But let's keep it A for a moment for AI reaction
        possessionCooldown_ = 0.5f;
    }

    ballRequestShoot_ = false;
    ballShootReleased_ = false;
}

void Game::handleKeyDown(unsigned char key) {
    const auto lower = static_cast<unsigned char>(std::tolower(key));
    keys_[lower] = true;

    // Handle ball shoot charge
    if (lower == ' ') {
        ballShootHeld_ = true;
    }

    // Runtime toggles used in demo: difficulty presets + reset + day/night.
    if (lower == '1') {
        applyDifficultyPreset(Difficulty::Easy);
    } else if (lower == '2') {
        applyDifficultyPreset(Difficulty::Medium);
    } else if (lower == '3') {
        applyDifficultyPreset(Difficulty::Hard);
    }

    if (lower == 'r') {
        resetMatch();
    }

    if (lower == 'n') {
        nightMode_ = !nightMode_;
    }

    if (key == 27) { // ESC
        audio_.shutdown();
        std::exit(0);
    }
}

void Game::handleKeyUp(unsigned char key) {
    const auto lower = static_cast<unsigned char>(std::tolower(key));
    keys_[lower] = false;

    // Handle ball shoot release
    if (lower == ' ') {
        ballShootHeld_ = false;
        ballShootReleased_ = true;
        ballRequestShoot_ = true;
    }
}

void Game::handleSpecialDown(int key) {
    if (key >= 0 && key < 256) {
        specialKeys_[static_cast<size_t>(key)] = true;
    }
}

void Game::handleSpecialUp(int key) {
    if (key >= 0 && key < 256) {
        specialKeys_[static_cast<size_t>(key)] = false;
    }
}
