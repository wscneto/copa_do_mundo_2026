#include "Game.h"

#include <GL/freeglut.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include "GameInternal.h"

using namespace game_internal;

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

    // Draw order is important for readability and fake depth layering.
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

    // Expand one axis to preserve field proportions regardless of window aspect.
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

    // Crowd colors pulse on goals and dangerous moments to reinforce match intensity.
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
    // HUD anchors rely on dynamic view bounds so overlays remain stable on resize.
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

