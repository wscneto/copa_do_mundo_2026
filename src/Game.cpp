#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace {
constexpr float PI = 3.14159265358979323846f;

float hash01(int a, int b) {
    const float v = std::sin(static_cast<float>(a * 127 + b * 311) * 0.131f) * 43758.5453f;
    return v - std::floor(v);
}

float degreesToRadians(float degrees) {
    return degrees * PI / 180.0f;
}

float dot2(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
}
}  // namespace

Game* Game::instance_ = nullptr;

Game::Game()
    : windowWidth_(1280),
      windowHeight_(720),
      viewLeft_(-80.0f),
      viewRight_(80.0f),
      viewBottom_(-45.0f),
      viewTop_(45.0f),
      keys_(),
      specialKeys_(),
      players_(),
      ball_{Vec2(0.0f, 0.0f), Vec2(0.0f, 0.0f), 0.8f},
      audio_(),
      scoreTeamA_(0),
      scoreTeamB_(0),
      matchTimeSeconds_(0.0f),
      goalFlashTimer_(0.0f),
      nightMode_(false),
      previousTimeMs_(0),
      pendingKickSound_(false),
      controlledPlayerIndex_(0),
      requestPass_(false),
      requestShoot_(false),
      requestSwitchPlayer_(false),
      shootHeld_(false),
      shootReleased_(false),
      shotCharge_(0.0f),
      ballControlCooldown_(0.0f),
      crowdExcitement_(0.0f),
      matchTimeRemaining_(180.0f),
      possessionCooldown_(0.0f),
      gameOver_(false),
      sessionWinsA_(0),
      sessionWinsB_(0),
      sessionDraws_(0),
      possessionTeam_(TeamSide::A),
    possessionActive_(false),
      difficulty_(Difficulty::Medium),
      tuning_{180.0f, 0.985f, 14.0f, 8.0f, 28.0f, 26.0f, 52.0f, 1.0f, 30.0f},
      controlledAimDirection_(1.0f, 0.0f),
      suggestedPassTargetIndex_(-1) {
}

void Game::run(int argc, char** argv) {
    instance_ = this;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowWidth_, windowHeight_);
    glutCreateWindow("World Cup 2026 - OpenGL 2.0 Football");

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardDownCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialDownCallback);
    glutSpecialUpFunc(specialUpCallback);
    glutIgnoreKeyRepeat(1);

    loadTuningFromConfigFile("config/game.cfg");
    applyDifficultyPreset(difficulty_);
    initializeWorld();
    configureProjection();
    audio_.init();

    previousTimeMs_ = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, timerCallback, 0);
    glutMainLoop();
}

void Game::initializeWorld() {
    resetMatch();
}

void Game::initializePlayers() {
    players_.clear();

    const std::vector<Vec2> teamAHome = {
        Vec2(-50.5f, 0.0f),
        Vec2(-34.0f, -12.0f),
        Vec2(-34.0f, 12.0f),
        Vec2(-20.0f, -18.0f),
        Vec2(-20.0f, 18.0f),
        Vec2(-8.0f, 0.0f)
    };

    const std::vector<Vec2> teamBHome = {
        Vec2(50.5f, 0.0f),
        Vec2(34.0f, -12.0f),
        Vec2(34.0f, 12.0f),
        Vec2(20.0f, -18.0f),
        Vec2(20.0f, 18.0f),
        Vec2(8.0f, 0.0f)
    };

    for (size_t i = 0; i < teamAHome.size(); ++i) {
        Player player;
        player.position = teamAHome[i];
        player.homePosition = teamAHome[i];
        player.radius = 1.1f;
        player.speed = (i == 0) ? 4.1f : (7.6f + static_cast<float>(i) * 0.35f);
        player.team = TeamSide::A;
        player.isGoalkeeper = (i == 0);
        player.isDefender = (i == 1 || i == 2);
        player.jitterSeed = static_cast<float>(i) * 0.97f;
        players_.push_back(player);
    }

    for (size_t i = 0; i < teamBHome.size(); ++i) {
        Player player;
        player.position = teamBHome[i];
        player.homePosition = teamBHome[i];
        player.radius = 1.1f;
        player.speed = (i == 0) ? 4.1f : (7.6f + static_cast<float>(i) * 0.35f);
        player.team = TeamSide::B;
        player.isGoalkeeper = (i == 0);
        player.isDefender = (i == 1 || i == 2);
        player.jitterSeed = static_cast<float>(i) * 1.11f;
        players_.push_back(player);
    }

    controlledPlayerIndex_ = 5;
    if (controlledPlayerIndex_ >= static_cast<int>(players_.size())) {
        controlledPlayerIndex_ = 0;
    }
}

void Game::resetAfterGoal() {
    ball_.position = Vec2(0.0f, 0.0f);
    ball_.velocity = Vec2(0.0f, 0.0f);
    ballControlCooldown_ = 0.45f;
    shotCharge_ = 0.0f;
    possessionActive_ = false;
    possessionCooldown_ = 0.0f;

    for (Player& player : players_) {
        player.position = player.homePosition;
    }
}

void Game::resetMatch() {
    scoreTeamA_ = 0;
    scoreTeamB_ = 0;
    matchTimeSeconds_ = 0.0f;
    matchTimeRemaining_ = tuning_.matchDurationSeconds;
    goalFlashTimer_ = 0.0f;
    crowdExcitement_ = 0.0f;
    possessionCooldown_ = 0.0f;
    ballControlCooldown_ = 0.0f;
    shotCharge_ = 0.0f;
    suggestedPassTargetIndex_ = -1;
    controlledAimDirection_ = Vec2(1.0f, 0.0f);
    possessionTeam_ = TeamSide::A;
    possessionActive_ = false;
    gameOver_ = false;

    requestPass_ = false;
    requestShoot_ = false;
    requestSwitchPlayer_ = false;
    shootHeld_ = false;
    shootReleased_ = false;

    initializePlayers();
    ball_.position = Vec2(0.0f, 0.0f);
    ball_.velocity = Vec2(0.0f, 0.0f);
}

void Game::loadTuningFromConfigFile(const std::string& filePath) {
    std::ifstream input(filePath.c_str());
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        const size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, equalsPos);
        const std::string value = line.substr(equalsPos + 1);

        if (key == "matchDurationSeconds") {
            tuning_.matchDurationSeconds = std::max(30.0f, static_cast<float>(std::atof(value.c_str())));
        } else if (key == "ballFriction") {
            tuning_.ballFriction = clampf(static_cast<float>(std::atof(value.c_str())), 0.92f, 0.998f);
        } else if (key == "dribbleFollow") {
            tuning_.dribbleFollow = clampf(static_cast<float>(std::atof(value.c_str())), 3.0f, 30.0f);
        } else if (key == "dribbleCarrySpeed") {
            tuning_.dribbleCarrySpeed = clampf(static_cast<float>(std::atof(value.c_str())), 2.0f, 20.0f);
        } else if (key == "passSpeed") {
            tuning_.passSpeed = clampf(static_cast<float>(std::atof(value.c_str())), 10.0f, 42.0f);
        } else if (key == "shotMinSpeed") {
            tuning_.shotMinSpeed = clampf(static_cast<float>(std::atof(value.c_str())), 12.0f, 42.0f);
        } else if (key == "shotMaxSpeed") {
            tuning_.shotMaxSpeed = clampf(static_cast<float>(std::atof(value.c_str())), 24.0f, 70.0f);
        } else if (key == "dangerousShotSpeed") {
            tuning_.dangerousShotSpeed = clampf(static_cast<float>(std::atof(value.c_str())), 12.0f, 80.0f);
        } else if (key == "difficulty") {
            if (value == "easy") {
                difficulty_ = Difficulty::Easy;
            } else if (value == "hard") {
                difficulty_ = Difficulty::Hard;
            } else {
                difficulty_ = Difficulty::Medium;
            }
        }
    }
}

void Game::applyDifficultyPreset(Difficulty difficulty) {
    difficulty_ = difficulty;

    switch (difficulty_) {
        case Difficulty::Easy:
            tuning_.aiSpeedMultiplier = 0.86f;
            break;
        case Difficulty::Medium:
            tuning_.aiSpeedMultiplier = 1.00f;
            break;
        case Difficulty::Hard:
            tuning_.aiSpeedMultiplier = 1.14f;
            break;
    }
}

void Game::update(float dt) {
    matchTimeSeconds_ += dt;

    if (goalFlashTimer_ > 0.0f) {
        goalFlashTimer_ = std::max(0.0f, goalFlashTimer_ - dt);
    }

    updateMatchState(dt);

    if (!gameOver_) {
        updateControlledPlayerFromInput(dt);
        handleControlledPlayerActions();
        updatePlayersAI(dt);
        updateBallPhysics(dt);
        updatePossessionState(dt);
        checkGoalDetection();
    }

    updateCrowdExcitement(dt);
    audio_.setCrowdExcitement(crowdExcitement_);
    audio_.update(dt);

    if (pendingKickSound_) {
        audio_.playKick();
        pendingKickSound_ = false;
    }
}

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
    const bool controllingBall = hasBall || keepPossessionWithoutDribble;

    if (controllingBall && ballControlCooldown_ <= 0.0f) {
        possessionTeam_ = TeamSide::A;
        possessionActive_ = true;
        possessionCooldown_ = 0.20f;

        if (shootHeld_) {
            shotCharge_ = clampf(shotCharge_ + dt * 1.15f, 0.0f, 1.0f);
        } else {
            shotCharge_ = std::max(0.0f, shotCharge_ - dt * 0.7f);

            Vec2 followDir = controlledAimDirection_;
            if (followDir.lengthSquared() < 1e-6f) {
                followDir = Vec2(1.0f, 0.0f);
            }

            const Vec2 desiredBallPos = controlledPlayer.position + followDir * (controlledPlayer.radius + ball_.radius + 0.25f);
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

void Game::updateBallPhysics(float dt) {
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

void Game::checkGoalDetection() {
    if (gameOver_) {
        return;
    }

    const float halfLength = FIELD_LENGTH * 0.5f;
    const float goalHalf = GOAL_WIDTH * 0.5f;

    if (std::abs(ball_.position.y) > goalHalf) {
        return;
    }

    if (ball_.position.x > halfLength + 0.45f) {
        ++scoreTeamA_;
        goalFlashTimer_ = 0.9f;
        crowdExcitement_ = std::max(crowdExcitement_, 0.9f);
        audio_.playGoal();
        resetAfterGoal();
    } else if (ball_.position.x < -halfLength - 0.45f) {
        ++scoreTeamB_;
        goalFlashTimer_ = 0.9f;
        crowdExcitement_ = std::max(crowdExcitement_, 0.9f);
        audio_.playGoal();
        resetAfterGoal();
    }
}

void Game::updateMatchState(float dt) {
    if (gameOver_) {
        return;
    }

    matchTimeRemaining_ = std::max(0.0f, matchTimeRemaining_ - dt);
    if (matchTimeRemaining_ <= 0.0f) {
        gameOver_ = true;
        ball_.velocity = Vec2(0.0f, 0.0f);

        if (scoreTeamA_ > scoreTeamB_) {
            ++sessionWinsA_;
        } else if (scoreTeamB_ > scoreTeamA_) {
            ++sessionWinsB_;
        } else {
            ++sessionDraws_;
        }
    }
}

void Game::updatePossessionState(float dt) {
    if (possessionCooldown_ > 0.0f) {
        possessionCooldown_ = std::max(0.0f, possessionCooldown_ - dt);
    }

    int controllingPlayerIndex = -1;
    float bestControlDistance = 1e9f;

    for (size_t i = 0; i < players_.size(); ++i) {
        const Player& player = players_[i];
        const float distance = (player.position - ball_.position).length();

        float controlRadius = player.radius + ball_.radius + 0.58f;
        if (player.isGoalkeeper) {
            controlRadius += 0.14f;
        }

        if (distance <= controlRadius && distance < bestControlDistance) {
            bestControlDistance = distance;
            controllingPlayerIndex = static_cast<int>(i);
        }
    }

    if (controllingPlayerIndex < 0) {
        possessionActive_ = false;
        return;
    }

    possessionTeam_ = players_[static_cast<size_t>(controllingPlayerIndex)].team;
    possessionActive_ = true;
}

void Game::updateCrowdExcitement(float dt) {
    if (ball_.velocity.length() > tuning_.dangerousShotSpeed && std::abs(ball_.position.y) < GOAL_WIDTH * 0.6f) {
        if (std::abs(ball_.position.x) > FIELD_LENGTH * 0.5f - 28.0f) {
            crowdExcitement_ = std::max(crowdExcitement_, 0.45f);
            audio_.playBigChance();
        }
    }

    crowdExcitement_ = std::max(0.0f, crowdExcitement_ - dt * 0.45f);
}

int Game::pickBestPassTarget(const Vec2& passDirection) const {
    if (controlledPlayerIndex_ < 0 || controlledPlayerIndex_ >= static_cast<int>(players_.size())) {
        return -1;
    }

    const Player& controlled = players_[static_cast<size_t>(controlledPlayerIndex_)];
    Vec2 forwardDir = passDirection;
    if (forwardDir.lengthSquared() < 1e-6f) {
        forwardDir = Vec2(1.0f, 0.0f);
    }
    forwardDir = forwardDir.normalized();

    float bestScore = -1e9f;
    int bestIndex = -1;

    for (size_t i = 0; i < players_.size(); ++i) {
        if (static_cast<int>(i) == controlledPlayerIndex_) {
            continue;
        }

        const Player& teammate = players_[i];
        if (teammate.team != TeamSide::A || teammate.isGoalkeeper) {
            continue;
        }

        Vec2 toTeammate = teammate.position - controlled.position;
        const float distance = std::max(0.001f, toTeammate.length());
        const Vec2 passDirNorm = toTeammate / distance;

        const float directionalAlignment = dot2(passDirNorm, forwardDir);
        const float directionScore = clampf((directionalAlignment + 1.0f) * 0.5f, 0.0f, 1.0f);

        float nearestOpponentDist = 1e9f;
        int nearbyOpponents = 0;
        float laneRisk = 0.0f;

        for (const Player& opponent : players_) {
            if (opponent.team != TeamSide::B) {
                continue;
            }

            const float distToReceiver = (opponent.position - teammate.position).length();
            nearestOpponentDist = std::min(nearestOpponentDist, distToReceiver);
            if (distToReceiver < 8.5f) {
                ++nearbyOpponents;
            }

            const float laneDistance = pointToSegmentDistance(opponent.position, controlled.position, teammate.position);
            const float projection = dot2(opponent.position - controlled.position, toTeammate) / (distance * distance);
            if (projection > 0.12f && projection < 0.96f && laneDistance < 4.0f) {
                laneRisk += (1.0f - laneDistance / 4.0f) * 0.40f;
            }
        }

        const float openness = clampf((nearestOpponentDist - 2.0f) / 14.0f, 0.0f, 1.0f) - nearbyOpponents * 0.05f;
        const float opennessScore = clampf(openness, 0.0f, 1.0f);
        const float laneSafety = 1.0f - clampf(laneRisk, 0.0f, 1.0f);

        const float preferredDistance = 14.0f;
        const float distanceScore = clampf(1.0f - std::abs(distance - preferredDistance) / 16.0f, 0.0f, 1.0f);

        const float progress = teammate.position.x - controlled.position.x;
        const float progressScore = clampf((progress + 12.0f) / 28.0f, 0.0f, 1.0f);

        // Pass prioritizes open teammates but still follows player aiming direction.
        float score =
            opennessScore * 3.0f +
            directionScore * 2.2f +
            laneSafety * 2.4f +
            distanceScore * 0.8f +
            progressScore * 0.6f;

        if (directionalAlignment < -0.35f) {
            score -= 1.1f;
        }

        if (score > bestScore) {
            bestScore = score;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}

bool Game::computePassInterception(const Vec2& passFrom, const Vec2& passTo, Vec2& interceptionPoint) const {
    const Vec2 segment = passTo - passFrom;
    const float segmentLengthSq = segment.lengthSquared();
    if (segmentLengthSq < 1e-5f) {
        return false;
    }

    float closestProjection = 2.0f;
    bool intercepted = false;

    for (const Player& opponent : players_) {
        if (opponent.team != TeamSide::B) {
            continue;
        }

        const float distanceToLane = pointToSegmentDistance(opponent.position, passFrom, passTo);
        if (distanceToLane > 1.45f) {
            continue;
        }

        const float projection = dot2(opponent.position - passFrom, segment) / segmentLengthSq;
        if (projection < 0.18f || projection > 0.95f) {
            continue;
        }

        if (projection < closestProjection) {
            closestProjection = projection;
            const Vec2 lanePoint = passFrom + segment * projection;
            interceptionPoint = lanePoint + (opponent.position - lanePoint) * 0.35f;
            intercepted = true;
        }
    }

    return intercepted;
}

float Game::pointToSegmentDistance(const Vec2& point, const Vec2& a, const Vec2& b) const {
    const Vec2 ab = b - a;
    const float abLenSq = ab.lengthSquared();
    if (abLenSq < 1e-6f) {
        return (point - a).length();
    }

    const float t = clampf(dot2(point - a, ab) / abLenSq, 0.0f, 1.0f);
    const Vec2 projection = a + ab * t;
    return (point - projection).length();
}

const char* Game::difficultyName() const {
    switch (difficulty_) {
        case Difficulty::Easy:
            return "EASY";
        case Difficulty::Medium:
            return "MEDIUM";
        case Difficulty::Hard:
            return "HARD";
    }
    return "MEDIUM";
}

void Game::render() {
    if (nightMode_) {
        glClearColor(0.03f, 0.05f, 0.08f, 1.0f);
    } else {
        glClearColor(0.65f, 0.80f, 0.95f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    configureProjection();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawBackgroundAndStands();
    drawField();
    drawBallSpotlight();
    drawShadows();
    drawGoals();
    drawPlayers();
    drawBall();
    drawHud();

    if (goalFlashTimer_ > 0.0f) {
        const float alpha = std::min(0.35f, goalFlashTimer_ * 0.45f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glBegin(GL_QUADS);
        glVertex2f(viewLeft_, viewBottom_);
        glVertex2f(viewRight_, viewBottom_);
        glVertex2f(viewRight_, viewTop_);
        glVertex2f(viewLeft_, viewTop_);
        glEnd();
        glDisable(GL_BLEND);
    }

    if (gameOver_) {
        drawGameOverOverlay();
    }

    glutSwapBuffers();
}

void Game::configureProjection() {
    const float baseWidth = FIELD_LENGTH + 46.0f;
    const float baseHeight = FIELD_WIDTH + 36.0f;
    const float desiredAspect = baseWidth / baseHeight;
    const float currentAspect = static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_);

    float width = baseWidth;
    float height = baseHeight;

    if (currentAspect > desiredAspect) {
        width = baseHeight * currentAspect;
    } else {
        height = baseWidth / currentAspect;
    }

    viewLeft_ = -width * 0.5f;
    viewRight_ = width * 0.5f;
    viewBottom_ = -height * 0.5f;
    viewTop_ = height * 0.5f;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewLeft_, viewRight_, viewBottom_, viewTop_, -1.0, 1.0);
}

void Game::drawBackgroundAndStands() const {
    const float halfLength = FIELD_LENGTH * 0.5f;
    const float halfWidth = FIELD_WIDTH * 0.5f;

    const float stadiumLeft = -halfLength - 13.0f;
    const float stadiumRight = halfLength + 13.0f;
    const float stadiumBottom = -halfWidth - 13.0f;
    const float stadiumTop = halfWidth + 13.0f;

    if (nightMode_) {
        glColor3f(0.17f, 0.19f, 0.23f);
    } else {
        glColor3f(0.78f, 0.78f, 0.76f);
    }

    glBegin(GL_QUADS);
    glVertex2f(stadiumLeft, stadiumBottom);
    glVertex2f(stadiumRight, stadiumBottom);
    glVertex2f(stadiumRight, stadiumTop);
    glVertex2f(stadiumLeft, stadiumTop);
    glEnd();

    const float crowdBoost = (goalFlashTimer_ > 0.0f ? (0.45f * goalFlashTimer_) : 0.0f) + crowdExcitement_ * 0.5f;

    for (int row = 0; row < 6; ++row) {
        const float yBottom = halfWidth + 3.0f + static_cast<float>(row) * 1.35f;
        const float yTop = yBottom + 1.0f;

        for (int col = 0; col < 96; ++col) {
            const float xLeft = -halfLength - 6.0f + static_cast<float>(col) * 1.25f;
            const float xRight = xLeft + 0.95f;

            const float n = hash01(row, col);
            const bool yellowSeat = n > 0.46f;

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (yellowSeat) {
                r = 0.88f + n * 0.08f + crowdBoost * 0.22f;
                g = 0.72f + (1.0f - n) * 0.20f + crowdBoost * 0.15f;
                b = 0.08f + std::sin(static_cast<float>(row + col) * 0.20f) * 0.04f + crowdBoost * 0.06f;
            } else {
                r = 0.10f + (1.0f - n) * 0.10f + crowdBoost * 0.08f;
                g = 0.34f + n * 0.14f + crowdBoost * 0.12f;
                b = 0.74f + n * 0.20f + crowdBoost * 0.26f;
            }

            glColor3f(clampf(r, 0.0f, 1.0f), clampf(g, 0.0f, 1.0f), clampf(b, 0.0f, 1.0f));
            glBegin(GL_QUADS);
            glVertex2f(xLeft, yBottom);
            glVertex2f(xRight, yBottom);
            glVertex2f(xRight, yTop);
            glVertex2f(xLeft, yTop);
            glEnd();

            glBegin(GL_QUADS);
            glVertex2f(xLeft, -yBottom);
            glVertex2f(xRight, -yBottom);
            glVertex2f(xRight, -yTop);
            glVertex2f(xLeft, -yTop);
            glEnd();
        }
    }

    for (int row = 0; row < 6; ++row) {
        const float xLeftBand = halfLength + 3.0f + static_cast<float>(row) * 1.35f;
        const float xRightBand = xLeftBand + 1.0f;

        for (int col = 0; col < 60; ++col) {
            const float yBottom = -halfWidth - 4.0f + static_cast<float>(col) * 1.25f;
            const float yTop = yBottom + 0.95f;

            const float n = hash01(row + 13, col + 77);
            const bool yellowSeat = n > 0.52f;

            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;

            if (yellowSeat) {
                r = 0.86f + n * 0.10f + crowdBoost * 0.20f;
                g = 0.74f + (1.0f - n) * 0.18f + crowdBoost * 0.14f;
                b = 0.10f + std::sin(static_cast<float>(row - col) * 0.21f) * 0.04f + crowdBoost * 0.06f;
            } else {
                r = 0.11f + (1.0f - n) * 0.10f + crowdBoost * 0.08f;
                g = 0.33f + n * 0.15f + crowdBoost * 0.12f;
                b = 0.73f + n * 0.19f + crowdBoost * 0.25f;
            }

            glColor3f(clampf(r, 0.0f, 1.0f), clampf(g, 0.0f, 1.0f), clampf(b, 0.0f, 1.0f));

            glBegin(GL_QUADS);
            glVertex2f(xLeftBand, yBottom);
            glVertex2f(xRightBand, yBottom);
            glVertex2f(xRightBand, yTop);
            glVertex2f(xLeftBand, yTop);
            glEnd();

            glBegin(GL_QUADS);
            glVertex2f(-xLeftBand, yBottom);
            glVertex2f(-xRightBand, yBottom);
            glVertex2f(-xRightBand, yTop);
            glVertex2f(-xLeftBand, yTop);
            glEnd();
        }
    }
}

void Game::drawField() const {
    const float halfLength = FIELD_LENGTH * 0.5f;
    const float halfWidth = FIELD_WIDTH * 0.5f;

    for (int stripe = 0; stripe < 12; ++stripe) {
        const float x0 = -halfLength + static_cast<float>(stripe) * (FIELD_LENGTH / 12.0f);
        const float x1 = x0 + (FIELD_LENGTH / 12.0f);

        if (nightMode_) {
            glColor3f((stripe % 2 == 0) ? 0.07f : 0.09f, (stripe % 2 == 0) ? 0.34f : 0.38f, 0.11f);
        } else {
            glColor3f((stripe % 2 == 0) ? 0.13f : 0.17f, (stripe % 2 == 0) ? 0.57f : 0.62f, 0.19f);
        }

        glBegin(GL_QUADS);
        glVertex2f(x0, -halfWidth);
        glVertex2f(x1, -halfWidth);
        glVertex2f(x1, halfWidth);
        glVertex2f(x0, halfWidth);
        glEnd();
    }

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.4f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(-halfLength, -halfWidth);
    glVertex2f(halfLength, -halfWidth);
    glVertex2f(halfLength, halfWidth);
    glVertex2f(-halfLength, halfWidth);
    glEnd();

    glBegin(GL_LINES);
    glVertex2f(0.0f, -halfWidth);
    glVertex2f(0.0f, halfWidth);
    glEnd();

    drawCircleOutline(Vec2(0.0f, 0.0f), CENTER_CIRCLE_RADIUS, 64, 2.4f);
    drawFilledCircle(Vec2(0.0f, 0.0f), 0.27f, 24);

    const float penaltyHalf = PENALTY_AREA_WIDTH * 0.5f;
    const float goalAreaHalf = GOAL_AREA_WIDTH * 0.5f;

    glBegin(GL_LINE_LOOP);
    glVertex2f(-halfLength, -penaltyHalf);
    glVertex2f(-halfLength + PENALTY_AREA_DEPTH, -penaltyHalf);
    glVertex2f(-halfLength + PENALTY_AREA_DEPTH, penaltyHalf);
    glVertex2f(-halfLength, penaltyHalf);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(halfLength, -penaltyHalf);
    glVertex2f(halfLength - PENALTY_AREA_DEPTH, -penaltyHalf);
    glVertex2f(halfLength - PENALTY_AREA_DEPTH, penaltyHalf);
    glVertex2f(halfLength, penaltyHalf);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(-halfLength, -goalAreaHalf);
    glVertex2f(-halfLength + GOAL_AREA_DEPTH, -goalAreaHalf);
    glVertex2f(-halfLength + GOAL_AREA_DEPTH, goalAreaHalf);
    glVertex2f(-halfLength, goalAreaHalf);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(halfLength, -goalAreaHalf);
    glVertex2f(halfLength - GOAL_AREA_DEPTH, -goalAreaHalf);
    glVertex2f(halfLength - GOAL_AREA_DEPTH, goalAreaHalf);
    glVertex2f(halfLength, goalAreaHalf);
    glEnd();

    drawFilledCircle(Vec2(-halfLength + 11.0f, 0.0f), 0.24f, 20);
    drawFilledCircle(Vec2(halfLength - 11.0f, 0.0f), 0.24f, 20);

    drawArc(Vec2(-halfLength + 11.0f, 0.0f), CENTER_CIRCLE_RADIUS, -52.0f, 52.0f, 36, 2.0f);
    drawArc(Vec2(halfLength - 11.0f, 0.0f), CENTER_CIRCLE_RADIUS, 128.0f, 232.0f, 36, 2.0f);

    drawArc(Vec2(-halfLength, halfWidth), CORNER_ARC_RADIUS, 270.0f, 360.0f, 20, 2.0f);
    drawArc(Vec2(halfLength, halfWidth), CORNER_ARC_RADIUS, 180.0f, 270.0f, 20, 2.0f);
    drawArc(Vec2(-halfLength, -halfWidth), CORNER_ARC_RADIUS, 0.0f, 90.0f, 20, 2.0f);
    drawArc(Vec2(halfLength, -halfWidth), CORNER_ARC_RADIUS, 90.0f, 180.0f, 20, 2.0f);
}

void Game::drawGoals() const {
    const float halfLength = FIELD_LENGTH * 0.5f;
    const float goalHalf = GOAL_WIDTH * 0.5f;

    glColor3f(0.97f, 0.97f, 0.97f);
    glLineWidth(2.0f);

    glBegin(GL_LINE_LOOP);
    glVertex2f(-halfLength, -goalHalf);
    glVertex2f(-halfLength - GOAL_DEPTH, -goalHalf);
    glVertex2f(-halfLength - GOAL_DEPTH, goalHalf);
    glVertex2f(-halfLength, goalHalf);
    glEnd();

    glBegin(GL_LINE_LOOP);
    glVertex2f(halfLength, -goalHalf);
    glVertex2f(halfLength + GOAL_DEPTH, -goalHalf);
    glVertex2f(halfLength + GOAL_DEPTH, goalHalf);
    glVertex2f(halfLength, goalHalf);
    glEnd();

    for (int i = 1; i < 8; ++i) {
        const float t = static_cast<float>(i) / 8.0f;
        const float y = -goalHalf + t * GOAL_WIDTH;

        glBegin(GL_LINES);
        glVertex2f(-halfLength, y);
        glVertex2f(-halfLength - GOAL_DEPTH, y);
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(halfLength, y);
        glVertex2f(halfLength + GOAL_DEPTH, y);
        glEnd();
    }

    for (int i = 1; i < 4; ++i) {
        const float t = static_cast<float>(i) / 4.0f;
        const float xL = -halfLength - t * GOAL_DEPTH;
        const float xR = halfLength + t * GOAL_DEPTH;

        glBegin(GL_LINES);
        glVertex2f(xL, -goalHalf);
        glVertex2f(xL, goalHalf);
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(xR, -goalHalf);
        glVertex2f(xR, goalHalf);
        glEnd();
    }
}

void Game::drawBall() const {
    glColor3f(0.97f, 0.97f, 0.97f);
    drawFilledCircle(ball_.position, ball_.radius, 40);

    glColor3f(0.05f, 0.05f, 0.05f);
    drawCircleOutline(ball_.position, ball_.radius, 40, 1.5f);

    auto drawPatch = [&](const Vec2& center, float radius) {
        glBegin(GL_POLYGON);
        for (int i = 0; i < 5; ++i) {
            const float angle = degreesToRadians(72.0f * static_cast<float>(i) + 18.0f);
            glVertex2f(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
        }
        glEnd();
    };

    glColor3f(0.08f, 0.08f, 0.08f);
    drawPatch(ball_.position, ball_.radius * 0.22f);

    for (int i = 0; i < 5; ++i) {
        const float angle = matchTimeSeconds_ * 45.0f + static_cast<float>(i) * 72.0f;
        const float rad = degreesToRadians(angle);
        const Vec2 center(
            ball_.position.x + std::cos(rad) * ball_.radius * 0.48f,
            ball_.position.y + std::sin(rad) * ball_.radius * 0.48f
        );
        drawPatch(center, ball_.radius * 0.12f);
    }
}

void Game::drawPlayers() const {
    auto drawStripedKit = [&](const Vec2& center, float radius, bool teamBrazil) {
        const float baseR = teamBrazil ? 0.96f : 0.94f;
        const float baseG = teamBrazil ? 0.84f : 0.96f;
        const float baseB = teamBrazil ? 0.12f : 0.98f;

        const float stripeR = teamBrazil ? 0.05f : 0.42f;
        const float stripeG = teamBrazil ? 0.58f : 0.76f;
        const float stripeB = teamBrazil ? 0.22f : 0.99f;

        glColor3f(baseR, baseG, baseB);
        drawFilledCircle(center, radius, 24);

        glColor3f(stripeR, stripeG, stripeB);
        const int stripeCount = 8;
        const float stripeWidth = (radius * 2.0f) / static_cast<float>(stripeCount);

        for (int stripe = 0; stripe < stripeCount; ++stripe) {
            if (stripe % 2 != 0) {
                continue;
            }

            const float x0 = center.x - radius + stripeWidth * static_cast<float>(stripe);
            const float x1 = x0 + stripeWidth * 0.86f;

            glBegin(GL_QUAD_STRIP);
            for (int step = 0; step <= 10; ++step) {
                const float t = static_cast<float>(step) / 10.0f;
                const float x = x0 + (x1 - x0) * t;
                const float dx = x - center.x;
                if (std::abs(dx) > radius) {
                    continue;
                }

                const float yExtent = std::sqrt(std::max(0.0f, radius * radius - dx * dx));
                glVertex2f(x, center.y - yExtent);
                glVertex2f(x, center.y + yExtent);
            }
            glEnd();
        }
    };

    for (size_t i = 0; i < players_.size(); ++i) {
        const Player& player = players_[i];

        const bool teamBrazil = (player.team == TeamSide::A);
        drawStripedKit(player.position, player.radius, teamBrazil);

        Vec2 faceDir = (ball_.position - player.position).normalized();
        if (faceDir.lengthSquared() < 1e-4f) {
            faceDir = Vec2(1.0f, 0.0f);
        }

        const Vec2 headCenter = player.position + faceDir * (player.radius * 0.62f);
        glColor3f(0.98f, 0.82f, 0.67f);
        drawFilledCircle(headCenter, player.radius * 0.34f, 18);

        glColor3f(0.10f, 0.10f, 0.10f);
        glLineWidth(1.8f);
        glBegin(GL_LINES);
        glVertex2f(player.position.x, player.position.y);
        glVertex2f(player.position.x + faceDir.x * player.radius * 1.25f, player.position.y + faceDir.y * player.radius * 1.25f);
        glEnd();

        if (static_cast<int>(i) == controlledPlayerIndex_) {
            glColor3f(1.0f, 0.95f, 0.18f);
            drawCircleOutline(player.position, player.radius + 0.30f, 30, 2.2f);
        }

        if (static_cast<int>(i) == suggestedPassTargetIndex_) {
            glColor3f(0.95f, 1.0f, 0.40f);
            drawCircleOutline(player.position, player.radius + 0.48f, 28, 1.8f);
        }
    }
}

void Game::drawHud() const {
    glColor3f(1.0f, 1.0f, 1.0f);

    std::ostringstream scoreStream;
    scoreStream << "TEAM A " << scoreTeamA_ << "  x  " << scoreTeamB_ << " TEAM B";
    drawText(-13.5f, viewTop_ - 5.4f, scoreStream.str());

    const int total = static_cast<int>(std::max(0.0f, matchTimeRemaining_));
    const int minutes = total / 60;
    const int seconds = total % 60;

    std::ostringstream clockStream;
    clockStream << std::setfill('0') << std::setw(2) << minutes << ":" << std::setw(2) << seconds;
    drawText(-2.0f, viewTop_ - 9.4f, "TIME " + clockStream.str());

    drawText(viewLeft_ + 2.0f, viewTop_ - 5.4f, "WASD / Arrows: mover jogador");
    drawText(viewLeft_ + 2.0f, viewTop_ - 8.0f, "Hold SPACE: carregar chute | soltar: chutar");
    drawText(viewLeft_ + 2.0f, viewTop_ - 10.6f, "E: passe | TAB: mais perto (sem posse) | 1/2/3: dificuldade");
    drawText(viewLeft_ + 2.0f, viewTop_ - 13.2f, "N: day/night | R: reiniciar partida | ESC: sair");

    const std::string possessionText =
        possessionActive_ ?
        ((possessionTeam_ == TeamSide::A) ? "POSSESSION: TEAM A" : "POSSESSION: TEAM B") :
        "POSSESSION: LOOSE BALL";
    drawText(viewRight_ - 38.0f, viewTop_ - 5.4f, possessionText);

    drawText(viewRight_ - 38.0f, viewTop_ - 8.0f, std::string("DIFFICULTY: ") + difficultyName());
    drawText(viewRight_ - 38.0f, viewTop_ - 10.6f, nightMode_ ? "MODE: NIGHT" : "MODE: DAY");

    std::ostringstream sessionStream;
    sessionStream << "SESSION WINS A/B/D: " << sessionWinsA_ << "/" << sessionWinsB_ << "/" << sessionDraws_;
    drawText(viewRight_ - 50.0f, viewTop_ - 13.2f, sessionStream.str());

    const float barX = -17.5f;
    const float barY = viewTop_ - 13.1f;
    const float barW = 35.0f;
    const float barH = 1.8f;

    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + barW, barY);
    glVertex2f(barX + barW, barY + barH);
    glVertex2f(barX, barY + barH);
    glEnd();

    glColor3f(0.96f, 0.88f, 0.20f);
    glBegin(GL_QUADS);
    glVertex2f(barX + 0.2f, barY + 0.2f);
    glVertex2f(barX + 0.2f + (barW - 0.4f) * shotCharge_, barY + 0.2f);
    glVertex2f(barX + 0.2f + (barW - 0.4f) * shotCharge_, barY + barH - 0.2f);
    glVertex2f(barX + 0.2f, barY + barH - 0.2f);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.2f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(barX, barY);
    glVertex2f(barX + barW, barY);
    glVertex2f(barX + barW, barY + barH);
    glVertex2f(barX, barY + barH);
    glEnd();

    drawText(-6.5f, barY - 1.7f, "SHOT POWER");

    if (goalFlashTimer_ > 0.0f) {
        glColor3f(1.0f, 0.95f, 0.2f);
        drawText(-7.8f, viewTop_ - 16.4f, "GOOOOOL!");
    }
}

void Game::drawShadows() const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.20f);
    for (const Player& player : players_) {
        const Vec2 shadowCenter = player.position + Vec2(0.28f, -0.35f);
        drawFilledCircle(shadowCenter, player.radius * 0.92f, 24);
    }

    glColor4f(0.0f, 0.0f, 0.0f, 0.24f);
    drawFilledCircle(ball_.position + Vec2(0.22f, -0.26f), ball_.radius * 0.95f, 30);

    glDisable(GL_BLEND);
}

void Game::drawBallSpotlight() const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float baseAlpha = nightMode_ ? 0.10f : 0.07f;
    glColor4f(1.0f, 1.0f, 0.75f, baseAlpha);
    drawFilledCircle(ball_.position, 6.5f, 40);
    glColor4f(1.0f, 1.0f, 0.75f, baseAlpha * 0.6f);
    drawFilledCircle(ball_.position, 9.5f, 40);

    glDisable(GL_BLEND);
}

void Game::drawGameOverOverlay() const {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.02f, 0.04f, 0.06f, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(-34.0f, -10.0f);
    glVertex2f(34.0f, -10.0f);
    glVertex2f(34.0f, 10.0f);
    glVertex2f(-34.0f, 10.0f);
    glEnd();

    glDisable(GL_BLEND);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(-9.0f, 5.0f, "FULL TIME");

    if (scoreTeamA_ > scoreTeamB_) {
        drawText(-14.0f, 1.5f, "TEAM A WINS THE MATCH");
    } else if (scoreTeamB_ > scoreTeamA_) {
        drawText(-14.0f, 1.5f, "TEAM B WINS THE MATCH");
    } else {
        drawText(-8.0f, 1.5f, "DRAW GAME");
    }

    drawText(-17.5f, -2.2f, "Press R to start a new match");
}

void Game::drawFilledCircle(const Vec2& center, float radius, int segments) const {
    glBegin(GL_POLYGON);
    for (int i = 0; i < segments; ++i) {
        const float angle = (2.0f * PI * static_cast<float>(i)) / static_cast<float>(segments);
        glVertex2f(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
    }
    glEnd();
}

void Game::drawCircleOutline(const Vec2& center, float radius, int segments, float lineWidth) const {
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        const float angle = (2.0f * PI * static_cast<float>(i)) / static_cast<float>(segments);
        glVertex2f(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
    }
    glEnd();
}

void Game::drawArc(const Vec2& center, float radius, float angleStart, float angleEnd, int segments, float lineWidth) const {
    glLineWidth(lineWidth);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = degreesToRadians(angleStart + (angleEnd - angleStart) * t);
        glVertex2f(center.x + std::cos(angle) * radius, center.y + std::sin(angle) * radius);
    }
    glEnd();
}

void Game::drawText(float x, float y, const std::string& text) const {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
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

void Game::handleResize(int width, int height) {
    windowWidth_ = std::max(width, 1);
    windowHeight_ = std::max(height, 1);
    glViewport(0, 0, windowWidth_, windowHeight_);
    configureProjection();
}

void Game::displayCallback() {
    if (instance_ != nullptr) {
        instance_->render();
    }
}

void Game::reshapeCallback(int width, int height) {
    if (instance_ != nullptr) {
        instance_->handleResize(width, height);
    }
}

void Game::keyboardDownCallback(unsigned char key, int, int) {
    if (instance_ != nullptr) {
        instance_->handleKeyDown(key);
    }
}

void Game::keyboardUpCallback(unsigned char key, int, int) {
    if (instance_ != nullptr) {
        instance_->handleKeyUp(key);
    }
}

void Game::specialDownCallback(int key, int, int) {
    if (instance_ != nullptr) {
        instance_->handleSpecialDown(key);
    }
}

void Game::specialUpCallback(int key, int, int) {
    if (instance_ != nullptr) {
        instance_->handleSpecialUp(key);
    }
}

void Game::timerCallback(int) {
    if (instance_ == nullptr) {
        return;
    }

    const int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(now - instance_->previousTimeMs_) / 1000.0f;
    instance_->previousTimeMs_ = now;

    dt = clampf(dt, 0.0f, 0.05f);
    instance_->update(dt);

    glutPostRedisplay();
    glutTimerFunc(16, timerCallback, 0);
}
