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
    const float baseWidth = FIELD_LENGTH + 74.0f;
    const float baseHeight = FIELD_WIDTH + 80.0f;
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

    // Inner ellipse is sized to fully contain the rectangular field corners.
    const float innerRadiusX = halfLength + 17.5f;
    const float innerRadiusY = halfWidth + 18.0f;
    const float outerRadiusX = innerRadiusX + 18.0f;
    const float outerRadiusY = innerRadiusY + 18.0f;

    const int bowlSegments = 220;

    // Draw the stadium floor (track/outer grass) so the sky/clear gap doesn't show
    if (nightMode_) {
        glColor3f(0.08f, 0.18f, 0.10f);
    } else {
        glColor3f(0.11f, 0.44f, 0.16f);
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f);
    for (int i = 0; i <= bowlSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bowlSegments);
        const float angle = t * 2.0f * PI;
        glVertex2f(innerRadiusX * std::cos(angle), innerRadiusY * std::sin(angle));
    }
    glEnd();

    if (nightMode_) {
        glColor3f(0.17f, 0.19f, 0.23f);
    } else {
        glColor3f(0.78f, 0.78f, 0.76f);
    }

    // 2. Concrete Bowl with Radial and Directional Gradient for Fake Depth
    // We use vertex colors to create a smooth gradient. Inner edge is darker (ambient occlusion),
    // outer edge is lighter. We also apply a fake directional sun/stadium light.
    struct Colors { float inR, inG, inB, outR, outG, outB; };
    Colors bowlColors = nightMode_
        ? Colors{0.14f, 0.16f, 0.20f, 0.36f, 0.40f, 0.46f}
        : Colors{0.40f, 0.40f, 0.40f, 0.85f, 0.85f, 0.82f};

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= bowlSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bowlSegments);
        const float angle = t * 2.0f * PI;
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        // Directional light factor (light coming from top-left)
        const float dirLight = (c * -0.7f + s * 0.7f) * 0.5f + 0.5f;
        
        // Outer Vertex (Lighter, top of the stands)
        float lOut = 0.7f + 0.4f * dirLight;
        glColor3f(bowlColors.outR * lOut, bowlColors.outG * lOut, bowlColors.outB * lOut);
        glVertex2f(outerRadiusX * c, outerRadiusY * s);
        
        // Inner Vertex (Darker, field level depression)
        float lIn = 0.5f + 0.3f * dirLight;
        glColor3f(bowlColors.inR * lIn, bowlColors.inG * lIn, bowlColors.inB * lIn);
        glVertex2f(innerRadiusX * c, innerRadiusY * s);
    }
    glEnd();

    // 3. Pitch Inner Wall (Creates a 3D drop from the stands to the grass)
    Colors wallColors = nightMode_
        ? Colors{0.07f, 0.09f, 0.12f, 0.15f, 0.18f, 0.22f}
        : Colors{0.25f, 0.25f, 0.25f, 0.45f, 0.45f, 0.45f};

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= bowlSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bowlSegments);
        const float angle = t * 2.0f * PI;
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        // Inner pitch-level vertex (Darkest)
        glColor3f(wallColors.inR, wallColors.inG, wallColors.inB);
        glVertex2f((innerRadiusX - 1.8f) * c, (innerRadiusY - 1.8f) * s);

        // Outer stands-level vertex
        glColor3f(wallColors.outR, wallColors.outG, wallColors.outB);
        glVertex2f(innerRadiusX * c, innerRadiusY * s);
    }
    glEnd();

    // 4. Dynamic Crowd with Depth and Perspective
    // Crowd colors pulse on goals and dangerous moments to reinforce match intensity.
    const float crowdBoost = (goalFlashTimer_ > 0.0f ? (0.45f * goalFlashTimer_) : 0.0f) + crowdExcitement_ * 0.5f;

    const int rowCount = 14;
    const int seatsPerRow = 240;
    const float firstRowRadiusX = innerRadiusX + 1.0f;
    const float firstRowRadiusY = innerRadiusY + 1.0f;
    const float lastRowRadiusX = outerRadiusX - 2.5f;
    const float lastRowRadiusY = outerRadiusY - 2.5f;

    for (int row = 0; row < rowCount; ++row) {
        // Perspective faking: rows pack closer together farther out to fake FOV depth
        const float linearRow = static_cast<float>(row) / static_cast<float>(rowCount - 1);
        const float rowMix = linearRow * (1.8f - linearRow); // gentle ease-out curve
        
        const float seatRadiusX = firstRowRadiusX + (lastRowRadiusX - firstRowRadiusX) * rowMix;
        const float seatRadiusY = firstRowRadiusY + (lastRowRadiusY - firstRowRadiusY) * rowMix;

        // Distant seats look smaller
        const float seatWidth = 0.85f - linearRow * 0.25f;
        const float seatHeight = 0.55f - linearRow * 0.15f;

        for (int col = 0; col < seatsPerRow; ++col) {
            const float t = static_cast<float>(col) / static_cast<float>(seatsPerRow);
            const float angle = t * 2.0f * PI + static_cast<float>(row) * 0.025f; // stagger seats
            const float c = std::cos(angle);
            const float s = std::sin(angle);

            const Vec2 center(seatRadiusX * c, seatRadiusY * s);
            const Vec2 tangent(-s, c);
            const Vec2 normal(c, s);

            const Vec2 halfW = tangent * (seatWidth * 0.5f);
            const Vec2 halfH = normal * (seatHeight * 0.5f);

            const float n = hash01(row + 5, col + 37);
            const bool yellowSeat = n > 0.48f;

            float r, g, b;

            if (yellowSeat) {
                r = 0.88f + n * 0.08f + crowdBoost * 0.22f;
                g = 0.72f + (1.0f - n) * 0.20f + crowdBoost * 0.15f;
                b = 0.08f + std::sin(static_cast<float>(row + col) * 0.20f) * 0.04f + crowdBoost * 0.06f;
            } else {
                r = 0.10f + (1.0f - n) * 0.10f + crowdBoost * 0.08f;
                g = 0.34f + n * 0.14f + crowdBoost * 0.12f;
                b = 0.74f + n * 0.20f + crowdBoost * 0.26f;
            }

            // Combine depth shade (dark near pitch, bright near top) and directional light
            const float dirLight = (c * -0.7f + s * 0.7f) * 0.5f + 0.5f;
            const float depthShade = 0.50f + 0.50f * linearRow;
            const float finalLight = nightMode_
                ? (depthShade * (0.92f + 0.42f * dirLight) + 0.08f)
                : (depthShade * (0.75f + 0.35f * dirLight));
            
            r *= finalLight;
            g *= finalLight;
            b *= finalLight;

            glColor3f(clampf(r, 0.0f, 1.0f), clampf(g, 0.0f, 1.0f), clampf(b, 0.0f, 1.0f));
            glBegin(GL_QUADS);
            glVertex2f(center.x - halfW.x - halfH.x, center.y - halfW.y - halfH.y);
            glVertex2f(center.x + halfW.x - halfH.x, center.y + halfW.y - halfH.y);
            glVertex2f(center.x + halfW.x + halfH.x, center.y + halfW.y + halfH.y);
            glVertex2f(center.x - halfW.x + halfH.x, center.y - halfW.y + halfH.y);
            glEnd();
        }
    }

    // 5. Volumetric Roof Canopy Shadow
    // Draws a semi-transparent rim around the outer bowl to suggest an overarching roof.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_TRIANGLE_STRIP);
    const float shadowAlpha = nightMode_ ? 0.45f : 0.40f;
    for (int i = 0; i <= bowlSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(bowlSegments);
        const float angle = t * 2.0f * PI;
        const float c = std::cos(angle);
        const float s = std::sin(angle);

        glColor4f(0.0f, 0.0f, 0.0f, shadowAlpha);
        glVertex2f(outerRadiusX * c, outerRadiusY * s);
        glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        glVertex2f((outerRadiusX - 5.5f) * c, (outerRadiusY - 5.5f) * s);
    }
    glEnd();
    glDisable(GL_BLEND);
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

    }
}

void Game::drawHud() const {
    const float topPadding = 3.0f;
    const float topHudY = viewTop_ - topPadding - 1.5f;     // Row 1
    const float midHudY = viewTop_ - topPadding - 4.5f;     // Row 2
    const float lowHudY = viewTop_ - topPadding - 7.5f;     // Row 3
    const float botHudY = viewTop_ - topPadding - 10.5f;    // Row 4
    
    const float leftHudX = viewLeft_ + 2.0f;
    const float rightHudX = viewRight_ - 30.0f;

    const float worldPerPixelX = (viewRight_ - viewLeft_) / static_cast<float>(windowWidth_);
    const float worldPerPixelY = (viewTop_ - viewBottom_) / static_cast<float>(windowHeight_);
    const float textHeightWorld = 18.0f * worldPerPixelY;
    auto textWidthWorld = [&](const std::string& text) {
        int widthPx = 0;
        for (char c : text) {
            widthPx += glutBitmapWidth(GLUT_BITMAP_HELVETICA_18, c);
        }
        return static_cast<float>(widthPx) * worldPerPixelX;
    };

    std::ostringstream scoreAStream;
    scoreAStream << scoreTeamA_;
    std::ostringstream scoreBStream;
    scoreBStream << scoreTeamB_;

    const std::string teamLeft = "BRASIL";
    const std::string teamRight = "ARGENTINA";
    const std::string scoreText = scoreAStream.str() + " - " + scoreBStream.str();

    const float leftTeamWidth = textWidthWorld(teamLeft);
    const float rightTeamWidth = textWidthWorld(teamRight);
    const float scoreTextWidth = textWidthWorld(scoreText);

    // ------------------------------------------------------------------
    // TV-Style Scoreboard (Center Top)
    // ------------------------------------------------------------------
    const float indicatorWidth = 1.7f;
    const float sbPadX = 1.8f;
    const float sbPadY = 0.85f;
    const float teamToScoreGap = 1.7f;
    const float scoreBoxPadX = 1.3f;

    const float scoreBoxWidth = std::max(10.8f, scoreTextWidth + scoreBoxPadX * 2.0f);
    const float sbWidth = (indicatorWidth * 2.0f) + (sbPadX * 2.0f) + leftTeamWidth + rightTeamWidth + scoreBoxWidth + (teamToScoreGap * 2.0f);
    const float sbHeight = textHeightWorld + sbPadY * 2.0f;
    const float sbX = -sbWidth / 2.0f;
    const float sbY = viewTop_ - 1.0f; 
    const float sbTextY = sbY - sbPadY - textHeightWorld * 0.88f;

    // Main dark background for the whole scoreboard
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(sbX, sbY);
    glVertex2f(sbX + sbWidth, sbY);
    glVertex2f(sbX + sbWidth, sbY - sbHeight);
    glVertex2f(sbX, sbY - sbHeight);
    glEnd();

    // Brazil Team Color Indicator (Green)
    glColor3f(0.0f, 0.6f, 0.2f);
    glBegin(GL_QUADS);
    glVertex2f(sbX, sbY);
    glVertex2f(sbX + indicatorWidth, sbY);
    glVertex2f(sbX + indicatorWidth, sbY - sbHeight);
    glVertex2f(sbX, sbY - sbHeight);
    glEnd();

    // Argentina Team Color Indicator (Light Blue)
    glColor3f(0.4f, 0.7f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(sbX + sbWidth - indicatorWidth, sbY);
    glVertex2f(sbX + sbWidth, sbY);
    glVertex2f(sbX + sbWidth, sbY - sbHeight);
    glVertex2f(sbX + sbWidth - indicatorWidth, sbY - sbHeight);
    glEnd();

    // Score box background (darker) in the center
    const float scoreBoxX = sbX + (sbWidth - scoreBoxWidth) / 2.0f;
    glColor3f(0.05f, 0.05f, 0.05f);
    glBegin(GL_QUADS);
    glVertex2f(scoreBoxX, sbY);
    glVertex2f(scoreBoxX + scoreBoxWidth, sbY);
    glVertex2f(scoreBoxX + scoreBoxWidth, sbY - sbHeight);
    glVertex2f(scoreBoxX, sbY - sbHeight);
    glEnd();

    // Border around scoreboard
    glColor3f(0.8f, 0.8f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(sbX, sbY);
    glVertex2f(sbX + sbWidth, sbY);
    glVertex2f(sbX + sbWidth, sbY - sbHeight);
    glVertex2f(sbX, sbY - sbHeight);
    glEnd();

    // Score texts
    glColor3f(1.0f, 1.0f, 1.0f);

    // Team names and score are aligned with dynamic padding so text never overflows.
    const float leftNameX = sbX + indicatorWidth + sbPadX;
    const float rightNameX = sbX + sbWidth - indicatorWidth - sbPadX - rightTeamWidth;
    const float scoreTextX = scoreBoxX + (scoreBoxWidth - scoreTextWidth) * 0.5f;

    drawText(leftNameX, sbTextY, teamLeft);
    drawText(rightNameX, sbTextY, teamRight);
    drawText(scoreTextX, sbTextY, scoreText);

    // ------------------------------------------------------------------
    // TV-Style Match Clock
    // ------------------------------------------------------------------
    const int total = static_cast<int>(std::max(0.0f, matchTimeRemaining_));
    const int minutes = total / 60;
    const int seconds = total % 60;

    std::ostringstream clockStream;
    clockStream << std::setfill('0') << std::setw(2) << minutes << ":" << std::setw(2) << seconds;

    const float timePadX = 1.1f;
    const float timePadY = 0.55f;
    const float timeGap = 0.65f;
    const float timeBoxWidth = std::max(11.6f, textWidthWorld(clockStream.str()) + timePadX * 2.0f);
    const float timeBoxHeight = textHeightWorld + timePadY * 2.0f;
    const float timeBoxY = sbY - sbHeight - timeGap;
    const float timeBoxX = sbX + (sbWidth - timeBoxWidth) / 2.0f;
    const float timeTextY = timeBoxY - timePadY - textHeightWorld * 0.88f;

    // Time box background
    glColor3f(0.85f, 0.85f, 0.85f); // light grey box for time
    glBegin(GL_QUADS);
    glVertex2f(timeBoxX, timeBoxY);
    glVertex2f(timeBoxX + timeBoxWidth, timeBoxY);
    glVertex2f(timeBoxX + timeBoxWidth, timeBoxY - timeBoxHeight);
    glVertex2f(timeBoxX, timeBoxY - timeBoxHeight);
    glEnd();
    
    // Time box border
    glColor3f(0.1f, 0.1f, 0.1f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(timeBoxX, timeBoxY);
    glVertex2f(timeBoxX + timeBoxWidth, timeBoxY);
    glVertex2f(timeBoxX + timeBoxWidth, timeBoxY - timeBoxHeight);
    glVertex2f(timeBoxX, timeBoxY - timeBoxHeight);
    glEnd();

    // Time text
    glColor3f(0.0f, 0.0f, 0.0f); // dark text for time
    drawText(timeBoxX + (timeBoxWidth - textWidthWorld(clockStream.str())) * 0.5f, timeTextY, clockStream.str());

    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(leftHudX, topHudY, "Mover bola: WASD / setas");
    drawText(leftHudX, midHudY, "Chute: segure SPACE e solte");
    drawText(leftHudX, lowHudY, "Dificuldade: 1 / 2 / 3");
    drawText(leftHudX, botHudY, "N: dia/noite | R: reiniciar | ESC: sair");

    const std::string possessionText =
        possessionActive_ ?
        ((possessionTeam_ == TeamSide::A) ? "POSSE: BRASIL" : "POSSE: ARGENTINA") :
        "POSSE: BOLA LIVRE";
    drawText(rightHudX, topHudY, possessionText);

    drawText(rightHudX, midHudY, std::string("NIVEL: ") + difficultyName());
    drawText(rightHudX, lowHudY, nightMode_ ? "MODO: NOITE" : "MODO: DIA");

    std::ostringstream sessionStream;
    sessionStream << "VITORIAS SESSAO A/B/E: " << sessionWinsA_ << "/" << sessionWinsB_ << "/" << sessionDraws_;
    drawText(rightHudX, botHudY, sessionStream.str());

    const float barX = -8.5f;
    const float barY = lowHudY - 2.0f;
    const float barW = 17.0f;
    const float barH = 1.4f;

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

    drawText(-6.5f, barY + 2.0f, "POTENCIA DO CHUTE");

    if (goalFlashTimer_ > 0.0f) {
        glColor3f(1.0f, 0.95f, 0.2f);
        drawText(-4.8f, barY - 2.5f, "GOOOOOL!");
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
    drawText(-9.0f, 5.0f, "FIM DO TEMPO");

    if (scoreTeamA_ > scoreTeamB_) {
        drawText(-14.0f, 1.5f, "BRASIL VENCE A PARTIDA");
    } else if (scoreTeamB_ > scoreTeamA_) {
        drawText(-14.0f, 1.5f, "ARGENTINA VENCE A PARTIDA");
    } else {
        drawText(-8.0f, 1.5f, "JOGO EMPATADO");
    }

    drawText(-17.5f, -2.2f, "Aperte R para iniciar nova partida");
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
