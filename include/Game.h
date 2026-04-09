#pragma once

#include <array>
#include <string>
#include <vector>

#include "AudioSystem.h"
#include "Vec2.h"

enum class TeamSide {
    A,
    B
};

enum class Difficulty {
    Easy,
    Medium,
    Hard
};

struct GameTuning {
    float matchDurationSeconds;
    float ballFriction;
    float dribbleFollow;
    float dribbleCarrySpeed;
    float passSpeed;
    float shotMinSpeed;
    float shotMaxSpeed;
    float aiSpeedMultiplier;
    float dangerousShotSpeed;
};

struct Ball {
    Vec2 position;
    Vec2 velocity;
    float radius;
};

struct Player {
    Vec2 position;
    Vec2 homePosition;
    float radius;
    float speed;
    TeamSide team;
    bool isGoalkeeper;
    bool isDefender;
    float jitterSeed;
};

class Game {
public:
    Game();

    void run(int argc, char** argv);

private:
    static Game* instance_;

    static constexpr float FIELD_LENGTH = 105.0f;
    static constexpr float FIELD_WIDTH = 68.0f;
    static constexpr float PENALTY_AREA_DEPTH = 16.5f;
    static constexpr float PENALTY_AREA_WIDTH = 40.32f;
    static constexpr float GOAL_AREA_DEPTH = 5.5f;
    static constexpr float GOAL_AREA_WIDTH = 18.32f;
    static constexpr float CENTER_CIRCLE_RADIUS = 9.15f;
    static constexpr float CORNER_ARC_RADIUS = 1.0f;
    static constexpr float GOAL_WIDTH = 10.8f;
    static constexpr float GOAL_DEPTH = 2.4f;

    int windowWidth_;
    int windowHeight_;

    float viewLeft_;
    float viewRight_;
    float viewBottom_;
    float viewTop_;

    std::array<bool, 256> keys_;
    std::array<bool, 256> specialKeys_;

    std::vector<Player> players_;
    Ball ball_;
    AudioSystem audio_;

    int scoreTeamA_;
    int scoreTeamB_;

    float matchTimeSeconds_;
    float goalFlashTimer_;
    bool nightMode_;

    int previousTimeMs_;

    bool pendingKickSound_;
    int controlledPlayerIndex_;
    bool requestPass_;
    bool requestShoot_;
    bool requestSwitchPlayer_;
    bool shootHeld_;
    bool shootReleased_;

    float shotCharge_;
    float ballControlCooldown_;
    float crowdExcitement_;
    float matchTimeRemaining_;
    float possessionCooldown_;

    bool gameOver_;
    int sessionWinsA_;
    int sessionWinsB_;
    int sessionDraws_;

    TeamSide possessionTeam_;
    bool possessionActive_;
    Difficulty difficulty_;
    GameTuning tuning_;
    Vec2 controlledAimDirection_;
    int suggestedPassTargetIndex_;

    void initializeWorld();
    void initializePlayers();
    void resetAfterGoal();
    void resetMatch();
    void loadTuningFromConfigFile(const std::string& filePath);
    void applyDifficultyPreset(Difficulty difficulty);

    void update(float dt);
    void updateControlledPlayerFromInput(float dt);
    void handleControlledPlayerActions();
    void updateBallPhysics(float dt);
    void updatePlayersAI(float dt);
    void checkGoalDetection();
    void updateMatchState(float dt);
    void updatePossessionState(float dt);
    void updateCrowdExcitement(float dt);

    int pickBestPassTarget(const Vec2& passDirection) const;
    bool computePassInterception(const Vec2& passFrom, const Vec2& passTo, Vec2& interceptionPoint) const;
    float pointToSegmentDistance(const Vec2& point, const Vec2& a, const Vec2& b) const;
    const char* difficultyName() const;

    void render();
    void configureProjection();

    void drawBackgroundAndStands() const;
    void drawField() const;
    void drawGoals() const;
    void drawBall() const;
    void drawPlayers() const;
    void drawHud() const;
    void drawShadows() const;
    void drawBallSpotlight() const;
    void drawGameOverOverlay() const;

    void drawFilledCircle(const Vec2& center, float radius, int segments) const;
    void drawCircleOutline(const Vec2& center, float radius, int segments, float lineWidth) const;
    void drawArc(const Vec2& center, float radius, float angleStart, float angleEnd, int segments, float lineWidth) const;
    void drawText(float x, float y, const std::string& text) const;

    void handleKeyDown(unsigned char key);
    void handleKeyUp(unsigned char key);
    void handleSpecialDown(int key);
    void handleSpecialUp(int key);
    void handleResize(int width, int height);

    static void displayCallback();
    static void reshapeCallback(int width, int height);
    static void keyboardDownCallback(unsigned char key, int x, int y);
    static void keyboardUpCallback(unsigned char key, int x, int y);
    static void specialDownCallback(int key, int x, int y);
    static void specialUpCallback(int key, int x, int y);
    static void timerCallback(int value);
};
