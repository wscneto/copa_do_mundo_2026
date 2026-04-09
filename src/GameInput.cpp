#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

#include "GameInternal.h"

using namespace game_internal;

void Game::updateControlledPlayerFromInput(float dt) {
    if (controlledPlayerIndex_ < 0 || controlledPlayerIndex_ >= static_cast<int>(players_.size())) {
        return;
    }

    if (ballControlCooldown_ > 0.0f) {
        ballControlCooldown_ = std::max(0.0f, ballControlCooldown_ - dt);
    }

    Player& controlledPlayer = players_[static_cast<size_t>(controlledPlayerIndex_)];
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

    if (movementDir.lengthSquared() > 0.0f) {
        // Aim follows latest movement direction and is reused by pass/shot actions.
        movementDir = movementDir.normalized();
        controlledAimDirection_ = movementDir;
    }

    const float moveSpeed = controlledPlayer.speed + 2.7f;
    if (movementDir.lengthSquared() > 0.0f) {
        controlledPlayer.position += movementDir * (moveSpeed * dt);
    }

    const float halfLength = FIELD_LENGTH * 0.5f;
    const float halfWidth = FIELD_WIDTH * 0.5f;
    controlledPlayer.position.x = clampf(controlledPlayer.position.x, -halfLength + 0.9f, halfLength - 0.9f);
    controlledPlayer.position.y = clampf(controlledPlayer.position.y, -halfWidth + 0.9f, halfWidth - 0.9f);

    const float controlRadius = controlledPlayer.radius + ball_.radius + 0.52f;
    const float ballDistance = (ball_.position - controlledPlayer.position).length();
    const bool hasBall = ballDistance <= controlRadius;

    float nearestOpponentToBall = 1e9f;
    for (const Player& player : players_) {
        if (player.team != TeamSide::B) {
            continue;
        }

        nearestOpponentToBall = std::min(nearestOpponentToBall, (player.position - ball_.position).length());
    }

    const bool opponentContesting = nearestOpponentToBall < 1.95f;
    const float keepControlRadius = controlRadius + 1.15f;
    const bool keepPossessionWithoutDribble =
        possessionActive_ &&
        possessionTeam_ == TeamSide::A &&
        !opponentContesting &&
        ballDistance <= keepControlRadius;
    // Grace period avoids losing possession instantly during short dribble gaps.
    const bool controllingBall = hasBall || keepPossessionWithoutDribble;

    if (controllingBall && ballControlCooldown_ <= 0.0f) {
        possessionTeam_ = TeamSide::A;
        possessionActive_ = true;
        possessionCooldown_ = 0.20f;

        if (shootHeld_) {
            // Charging is continuous while the button is held.
            shotCharge_ = clampf(shotCharge_ + dt * 1.15f, 0.0f, 1.0f);
        } else {
            shotCharge_ = std::max(0.0f, shotCharge_ - dt * 0.7f);

            Vec2 followDir = controlledAimDirection_;
            if (followDir.lengthSquared() < 1e-6f) {
                followDir = Vec2(1.0f, 0.0f);
            }

            const Vec2 desiredBallPos = controlledPlayer.position + followDir * (controlledPlayer.radius + ball_.radius + 0.25f);
            // Smoothly pull ball to preferred dribble point in front of the player.
            const float interpolation = clampf(tuning_.dribbleFollow * dt, 0.0f, 1.0f);
            ball_.position += (desiredBallPos - ball_.position) * interpolation;

            const Vec2 dribbleVelocity = followDir * tuning_.dribbleCarrySpeed;
            ball_.velocity = ball_.velocity * 0.62f + dribbleVelocity * 0.38f;

            if (movementDir.lengthSquared() <= 0.0f) {
                ball_.velocity *= 0.86f;
            }
        }
    } else if (!shootHeld_) {
        shotCharge_ = std::max(0.0f, shotCharge_ - dt * 0.85f);
    }
}

void Game::handleControlledPlayerActions() {
    if (requestSwitchPlayer_) {
        // Player switch is blocked while Team A has clear possession.
        if (!(possessionActive_ && possessionTeam_ == TeamSide::A)) {
            float bestDistance = 1e9f;
            int bestIndex = -1;

            for (size_t i = 0; i < players_.size(); ++i) {
                const Player& candidatePlayer = players_[i];
                if (candidatePlayer.team != TeamSide::A || candidatePlayer.isGoalkeeper) {
                    continue;
                }

                const float distanceToBall = (candidatePlayer.position - ball_.position).length();
                if (distanceToBall < bestDistance) {
                    bestDistance = distanceToBall;
                    bestIndex = static_cast<int>(i);
                }
            }

            if (bestIndex >= 0) {
                controlledPlayerIndex_ = bestIndex;
            }
        }
    }
    requestSwitchPlayer_ = false;

    if (controlledPlayerIndex_ < 0 || controlledPlayerIndex_ >= static_cast<int>(players_.size())) {
        requestPass_ = false;
        requestShoot_ = false;
        shootReleased_ = false;
        return;
    }

    Player& controlledPlayer = players_[static_cast<size_t>(controlledPlayerIndex_)];
    const float hasBallRadius = controlledPlayer.radius + ball_.radius + 0.58f;
    const bool hasBall = (ball_.position - controlledPlayer.position).length() <= hasBallRadius;

    if (requestPass_ && hasBall && !gameOver_) {
        Vec2 passDir = controlledAimDirection_;
        if (passDir.lengthSquared() < 1e-6f) {
            passDir = Vec2(1.0f, 0.0f);
        }
        passDir = passDir.normalized();

        // Suggested target is used both for pass execution and ring highlight.
        const int passTargetIndex = pickBestPassTarget(passDir);
        suggestedPassTargetIndex_ = passTargetIndex;

        if (passTargetIndex >= 0) {
            const Vec2 teammatePos = players_[static_cast<size_t>(passTargetIndex)].position;
            Vec2 interceptionPoint(0.0f, 0.0f);
            const bool intercepted = computePassInterception(controlledPlayer.position, teammatePos, interceptionPoint);

            const Vec2 destination = intercepted ? interceptionPoint : teammatePos;
            const Vec2 passDirection = (destination - controlledPlayer.position).normalized();
            const float passSpeed = tuning_.passSpeed * (intercepted ? 0.86f : 1.0f);

            ball_.position = controlledPlayer.position + passDirection * (controlledPlayer.radius + ball_.radius + 0.28f);
            ball_.velocity = passDirection * passSpeed + Vec2(3.0f, 0.0f);
            ballControlCooldown_ = 0.26f;
            possessionActive_ = false;
            pendingKickSound_ = true;
            crowdExcitement_ = std::max(crowdExcitement_, intercepted ? 0.28f : 0.20f);

            if (!intercepted) {
                // Successful clean pass transfers player control to the receiver.
                controlledPlayerIndex_ = passTargetIndex;
                controlledAimDirection_ = passDirection;
                suggestedPassTargetIndex_ = -1;
            }
        }
    }
    requestPass_ = false;

    if ((shootReleased_ || requestShoot_) && !gameOver_) {
        if (hasBall) {
            const float charge = clampf(shotCharge_, 0.0f, 1.0f);
            const float baseSpeed = tuning_.shotMinSpeed + (tuning_.shotMaxSpeed - tuning_.shotMinSpeed) * charge;

            // Lower charge means more vertical target noise (less accurate shot).
            const float errorFactor = (1.0f - charge) * 2.4f;
            const float deterministicNoise = std::sin(matchTimeSeconds_ * 8.3f + static_cast<float>(controlledPlayerIndex_) * 0.61f);
            const float targetY = clampf(
                controlledPlayer.position.y + controlledAimDirection_.y * 5.0f + deterministicNoise * errorFactor,
                -GOAL_WIDTH * 0.47f,
                GOAL_WIDTH * 0.47f
            );
            const Vec2 goalTarget(FIELD_LENGTH * 0.5f + GOAL_DEPTH, targetY);
            const Vec2 shotDirection = (goalTarget - controlledPlayer.position).normalized();

            ball_.position = controlledPlayer.position + shotDirection * (controlledPlayer.radius + ball_.radius + 0.28f);
            ball_.velocity = shotDirection * baseSpeed + Vec2(9.5f, 0.0f);
            ballControlCooldown_ = 0.42f;
            possessionActive_ = false;
            pendingKickSound_ = true;
            crowdExcitement_ = std::max(crowdExcitement_, 0.45f + charge * 0.4f);
        }
        shotCharge_ = 0.0f;
    }

    requestShoot_ = false;
    shootReleased_ = false;
}


void Game::handleKeyDown(unsigned char key) {
    const unsigned char lower = static_cast<unsigned char>(std::tolower(key));
    keys_[lower] = true;

    if (lower == 'e') {
        requestPass_ = true;
    }
    if (key == ' ') {
        shootHeld_ = true;
    }
    if (key == '\t') {
        requestSwitchPlayer_ = true;
    }

    // Runtime difficulty switch for quick gameplay tuning checks.
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

    if (key == 27) {
        audio_.shutdown();
        std::exit(0);
    }
}

void Game::handleKeyUp(unsigned char key) {
    const unsigned char lower = static_cast<unsigned char>(std::tolower(key));
    keys_[lower] = false;

    if (key == ' ') {
        shootHeld_ = false;
        shootReleased_ = true;
        requestShoot_ = true;
    }
}

void Game::handleSpecialDown(int key) {
    if (key >= 0 && key < static_cast<int>(specialKeys_.size())) {
        specialKeys_[static_cast<size_t>(key)] = true;
    }
}

void Game::handleSpecialUp(int key) {
    if (key >= 0 && key < static_cast<int>(specialKeys_.size())) {
        specialKeys_[static_cast<size_t>(key)] = false;
    }
}

