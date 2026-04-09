#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#include "GameInternal.h"

using namespace game_internal;

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

    // Load tunables before world reset so initial state already reflects config.
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

    // Team A and Team B share the same role slots to keep tactical symmetry.
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
    // Soft reset: keep match score/time and only reposition actors.
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
    // Full reset: scoreboard, timers, transient input flags and actors.
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

    // Simple key=value parser; invalid lines are ignored to keep startup resilient.
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
    // Frame update order: state/input -> simulation -> meta (crowd/audio).
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


void Game::checkGoalDetection() {
    if (gameOver_) {
        return;
    }

    const float halfLength = FIELD_LENGTH * 0.5f;
    const float goalHalf = GOAL_WIDTH * 0.5f;

    if (std::abs(ball_.position.y) > goalHalf) {
        return;
    }

    // Goal only counts if the ball crossed the line within the goal mouth.
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

    // Possession belongs to the closest player that is inside control radius.
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
    // Trigger dangerous chance reactions when a fast ball enters final third.
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

    // Weighted heuristic: openness + lane safety + direction + progression.
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

    // Pick the earliest opponent projection along the passing lane.
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

    // Clamp dt to avoid giant simulation steps after stalls or debugger pauses.
    dt = clampf(dt, 0.0f, 0.05f);
    instance_->update(dt);

    glutPostRedisplay();
    glutTimerFunc(16, timerCallback, 0);
}
