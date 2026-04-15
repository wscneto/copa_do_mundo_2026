#pragma once

#include <array>
#include <string>
#include <vector>

#include "AudioSystem.h"
#include "Vec2.h"

/**
 * @brief Identifies which side currently owns a player or possession.
 */
enum class TeamSide {
    A,
    B
};

/**
 * @brief Runtime difficulty presets that tune AI behavior.
 */
enum class Difficulty {
    Easy,
    Medium,
    Hard
};

/**
 * @brief Gameplay tuning values loaded from configuration.
 */
struct GameTuning {
    // Match and movement tuning loaded from config/game.cfg.
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

/**
 * @brief Physical state of the ball.
 */
struct Ball {
    Vec2 position;
    Vec2 velocity;
    float radius;
};

/**
 * @brief Runtime state for a player in either team.
 */
struct Player {
    Vec2 position;
    Vec2 homePosition;
    float radius;
    float speed;
    TeamSide team;
    // Team shape and AI behavior rely on these role flags.
    bool isGoalkeeper;
    bool isDefender;
    float jitterSeed;
};

class Game {
public:
    /**
     * @brief Creates a new match controller with default runtime state.
     */
    Game();

    /**
     * @brief Boots GLUT, loads configuration, and starts the main loop.
     * @param argc CLI argument count forwarded to GLUT.
     * @param argv CLI arguments forwarded to GLUT.
     */
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

    int previousTimeMs_; // Internal timer state
    bool pendingKickSound_;
    float shotCharge_;
    float ballControlCooldown_;
    float crowdExcitement_;
    float matchTimeRemaining_;
    float possessionCooldown_;
    bool gameOver_;
    Vec2 ballInputDirection_;
    bool ballShootHeld_;
    bool ballShootReleased_;
    bool ballRequestShoot_;
    int sessionWinsA_;
    int sessionWinsB_;
    int sessionDraws_;

    TeamSide possessionTeam_;
    bool possessionActive_;
    Difficulty difficulty_;
    GameTuning tuning_;

    void initializeWorld();
    void initializePlayers();
    void resetAfterGoal();
    void resetMatch();
    void loadTuningFromConfigFile(const std::string& filePath);
    void applyDifficultyPreset(Difficulty difficulty);

    void update(float dt);
    void updateBallControl(float dt);
    void updateBallPhysics(float dt);
    void updatePlayersAI(float dt);
    void checkGoalDetection();
    void updateMatchState(float dt);
    void updatePossessionState(float dt);
    void updateCrowdExcitement(float dt);

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
