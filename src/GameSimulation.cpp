#include "Game.h"

#include <algorithm>
#include <cmath>

#include "GameInternal.h"

using namespace game_internal;

void Game::updateBallPhysics(float dt) {
    // Integrate position first, then apply damping and boundary constraints.
    ball_.position += ball_.velocity * dt;

    const float damping = std::pow(tuning_.ballFriction, dt * 60.0f);
    ball_.velocity *= damping;

    if (ball_.velocity.length() < 0.02f) {
        ball_.velocity = Vec2(0.0f, 0.0f);
    }

    const float halfLength = FIELD_LENGTH * 0.5f;
    const float halfWidth = FIELD_WIDTH * 0.5f;
    const float goalHalf = GOAL_WIDTH * 0.5f;

    if (ball_.position.y > halfWidth - ball_.radius) {
        ball_.position.y = halfWidth - ball_.radius;
        ball_.velocity.y *= -0.45f;
    } else if (ball_.position.y < -halfWidth + ball_.radius) {
        ball_.position.y = -halfWidth + ball_.radius;
        ball_.velocity.y *= -0.45f;
    }

    // Side walls are disabled inside goal mouth so the ball can enter the net.
    const bool insideGoalMouth = std::abs(ball_.position.y) <= goalHalf;

    if (!insideGoalMouth) {
        if (ball_.position.x > halfLength - ball_.radius) {
            ball_.position.x = halfLength - ball_.radius;
            ball_.velocity.x *= -0.45f;
        } else if (ball_.position.x < -halfLength + ball_.radius) {
            ball_.position.x = -halfLength + ball_.radius;
            ball_.velocity.x *= -0.45f;
        }
    } else {
        if (ball_.position.x > halfLength + GOAL_DEPTH - ball_.radius) {
            ball_.position.x = halfLength + GOAL_DEPTH - ball_.radius;
            ball_.velocity.x *= -0.30f;
        } else if (ball_.position.x < -halfLength - GOAL_DEPTH + ball_.radius) {
            ball_.position.x = -halfLength - GOAL_DEPTH + ball_.radius;
            ball_.velocity.x *= -0.30f;
        }
    }

    const Vec2 posts[4] = {
        Vec2(-halfLength, -goalHalf),
        Vec2(-halfLength, goalHalf),
        Vec2(halfLength, -goalHalf),
        Vec2(halfLength, goalHalf)
    };

    // Posts are treated as small circular bumpers for believable rebounds.
    for (const Vec2& post : posts) {
        const Vec2 delta = ball_.position - post;
        const float minDistance = ball_.radius + 0.30f;
        const float distance = delta.length();

        if (distance > 1e-5f && distance < minDistance) {
            const Vec2 normal = delta / distance;
            ball_.position = post + normal * minDistance;

            const float projection = dot2(ball_.velocity, normal);
            ball_.velocity -= normal * (2.0f * projection);
            ball_.velocity *= 0.78f;
            ball_.velocity += normal * 2.4f;

            pendingKickSound_ = true;
            crowdExcitement_ = std::max(crowdExcitement_, 0.55f);
        }
    }
}

void Game::updatePlayersAI(float dt) {
    float keeperReactionChance = 0.63f;
    float keeperCoverage = 0.74f;
    float keeperSpeedMul = 0.78f;

    // Difficulty mostly affects goalkeeper reliability and global AI pace.
    if (difficulty_ == Difficulty::Easy) {
        keeperReactionChance = 0.48f;
        keeperCoverage = 0.64f;
        keeperSpeedMul = 0.66f;
    } else if (difficulty_ == Difficulty::Hard) {
        keeperReactionChance = 0.82f;
        keeperCoverage = 0.89f;
        keeperSpeedMul = 0.90f;
    }

    const float halfLength = FIELD_LENGTH * 0.5f;
    const float halfWidth = FIELD_WIDTH * 0.5f;
    const float goalHalf = GOAL_WIDTH * 0.5f;

    const TeamSide teamInPossession = possessionTeam_;
    const bool hasKnownPossession = possessionActive_;

    for (size_t i = 0; i < players_.size(); ++i) {
        if (static_cast<int>(i) == controlledPlayerIndex_) {
            continue;
        }

        Player& player = players_[i];
        const int roleSlot = static_cast<int>(i % 6);
        const float teamDirection = (player.team == TeamSide::A) ? 1.0f : -1.0f;
        const bool attackingPhase = hasKnownPossession && (player.team == teamInPossession);

        Vec2 target = ball_.position;
        const float wobbleTime = matchTimeSeconds_ + player.jitterSeed;
        target.x += std::sin(wobbleTime * 1.8f) * 1.25f;
        target.y += std::cos(wobbleTime * 1.2f) * 1.1f;

        float effectiveSpeed = player.speed * tuning_.aiSpeedMultiplier;

        if (player.isGoalkeeper) {
            // Keeper movement is constrained near line with partial ball tracking.
            target.x = (player.team == TeamSide::A) ? (-halfLength + 1.7f) : (halfLength - 1.7f);
            const float keeperLag = std::sin(wobbleTime * 1.4f) * 1.9f;
            target.y = clampf(ball_.position.y * keeperCoverage + keeperLag, -goalHalf * keeperCoverage, goalHalf * keeperCoverage);
            effectiveSpeed *= keeperSpeedMul;
        } else {
            float zoneXLocal = -6.0f;
            float zoneY = 0.0f;
            float zoneHalfX = 14.0f;
            float zoneHalfY = 10.0f;

            if (roleSlot == 1) {
                zoneXLocal = -34.0f;
                zoneY = -14.0f;
                zoneHalfX = 15.0f;
                zoneHalfY = 11.5f;
            } else if (roleSlot == 2) {
                zoneXLocal = -30.0f;
                zoneY = 14.0f;
                zoneHalfX = 15.0f;
                zoneHalfY = 11.5f;
            } else if (roleSlot == 3) {
                zoneXLocal = -12.0f;
                zoneY = -8.0f;
                zoneHalfX = 16.0f;
                zoneHalfY = 10.0f;
            } else if (roleSlot == 4) {
                zoneXLocal = -4.0f;
                zoneY = 8.0f;
                zoneHalfX = 16.0f;
                zoneHalfY = 10.0f;
            } else {
                zoneXLocal = 14.0f;
                zoneY = 0.0f;
                zoneHalfX = 18.0f;
                zoneHalfY = 14.0f;
            }

            Vec2 zoneCenter(
                zoneXLocal * teamDirection + (attackingPhase ? 8.5f * teamDirection : -2.5f * teamDirection),
                zoneY
            );

            const float ballProgress = clampf((ball_.position.x * teamDirection + halfLength) / FIELD_LENGTH, 0.0f, 1.0f);
            zoneCenter.x += (ballProgress - 0.5f) * (attackingPhase ? 9.0f : 5.0f) * teamDirection;

            if (attackingPhase) {
                // In possession: spread shape and push role-based support runs.
                if (roleSlot == 1 || roleSlot == 2) {
                    target.x = ball_.position.x - teamDirection * 14.0f;
                    target.y = zoneY + ball_.position.y * 0.26f;
                } else if (roleSlot == 3 || roleSlot == 4) {
                    target.x = ball_.position.x - teamDirection * 3.8f + ((roleSlot == 3) ? -1.6f : 1.6f);
                    target.y = ball_.position.y + ((roleSlot == 3) ? -7.4f : 7.4f) + std::sin(wobbleTime * 1.3f) * 1.2f;
                } else {
                    target.x = ball_.position.x + teamDirection * 10.5f;
                    target.y = ball_.position.y * 0.45f + std::sin(wobbleTime * 1.7f) * 4.6f;
                }

                target = target * 0.68f + zoneCenter * 0.32f;

                if ((target - ball_.position).length() < 4.0f) {
                    target.y += ((roleSlot % 2 == 0) ? 1.0f : -1.0f) * 3.2f;
                }
            } else {
                // Out of possession: stay compact, then bias to nearest threat.
                const Vec2 ballFromZone = ball_.position - zoneCenter;
                const bool ballInsideZone =
                    std::abs(ballFromZone.x) <= zoneHalfX &&
                    std::abs(ballFromZone.y) <= zoneHalfY;

                const float ballWeight = ballInsideZone ? 0.66f : 0.20f;
                target = zoneCenter * (1.0f - ballWeight) + ball_.position * ballWeight;

                float closestOpponentDistSq = 1e9f;
                int closestOpponent = -1;
                for (size_t j = 0; j < players_.size(); ++j) {
                    const Player& opponent = players_[j];
                    if (opponent.team == player.team || opponent.isGoalkeeper) {
                        continue;
                    }

                    if (std::abs(opponent.position.x - zoneCenter.x) > zoneHalfX + 4.0f ||
                        std::abs(opponent.position.y - zoneCenter.y) > zoneHalfY + 4.0f) {
                        continue;
                    }

                    const float distSq = (opponent.position - player.position).lengthSquared();
                    if (distSq < closestOpponentDistSq) {
                        closestOpponentDistSq = distSq;
                        closestOpponent = static_cast<int>(j);
                    }
                }

                if (closestOpponent >= 0) {
                    const Player& opponent = players_[static_cast<size_t>(closestOpponent)];
                    const Vec2 ownGoal((player.team == TeamSide::A) ? -halfLength : halfLength, 0.0f);
                    const Vec2 coverDir = (ownGoal - opponent.position).normalized();
                    const Vec2 markingPoint = opponent.position + coverDir * 1.9f;
                    target = target * 0.44f + markingPoint * 0.56f;
                }

                if (roleSlot == 5) {
                    target = target * 0.65f + ball_.position * 0.35f;
                }
            }

            effectiveSpeed *= attackingPhase ? 1.03f : 1.06f;
        }

        Vec2 delta = target - player.position;
        const float distance = delta.length();
        if (distance > 0.01f) {
            const Vec2 velocity = delta.normalized() * effectiveSpeed;
            const float maxStep = effectiveSpeed * dt;
            if (distance <= maxStep) {
                player.position = target;
            } else {
                player.position += velocity * dt;
            }
        }

        player.position.x = clampf(player.position.x, -halfLength + 0.8f, halfLength - 0.8f);
        player.position.y = clampf(player.position.y, -halfWidth + 0.8f, halfWidth - 0.8f);

        // Contact impulse approximates tackles/clearances without full collision solver.
        const Vec2 toBall = ball_.position - player.position;
        const float hitDistance = player.radius + ball_.radius + 0.15f;
        if (toBall.length() <= hitDistance) {
            if (player.isGoalkeeper) {
                const float reaction = hash01(static_cast<int>(matchTimeSeconds_ * 1000.0f), static_cast<int>(i * 17));
                if (reaction > keeperReactionChance) {
                    continue;
                }
            }

            Vec2 pushDirection = toBall.lengthSquared() > 1e-5f ? toBall.normalized() : Vec2(teamDirection, 0.0f);
            pushDirection = (pushDirection * 0.60f + Vec2(teamDirection, 0.0f) * 0.70f).normalized();

            const float pushStrength = player.isGoalkeeper
                ? 4.9f
                : ((attackingPhase ? 12.6f : 15.2f) * tuning_.aiSpeedMultiplier);
            ball_.velocity += pushDirection * pushStrength;
            possessionTeam_ = player.team;
            possessionActive_ = true;
            pendingKickSound_ = true;
        }
    }
}

