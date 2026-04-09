#pragma once

/**
 * @brief Runtime audio facade for ambience and gameplay sound effects.
 */
class AudioSystem {
public:
    /**
     * @brief Constructs an audio system in a non-initialized state.
     */
    AudioSystem();

    /**
     * @brief Releases audio resources if they were initialized.
     */
    ~AudioSystem();

    /**
     * @brief Initializes audio backend and sound buffers.
     * @return true when audio is available for playback.
     */
    bool init();

    /**
     * @brief Stops playback and destroys backend resources.
     */
    void shutdown();

    /**
     * @brief Updates cooldowns and adaptive crowd mix.
     * @param dt Frame delta time in seconds.
     */
    void update(float dt);

    /**
     * @brief Plays goal celebration cue.
     */
    void playGoal();

    /**
     * @brief Plays kick/contact cue with cooldown gating.
     */
    void playKick();

    /**
     * @brief Triggers crowd burst for dangerous attacks.
     */
    void playBigChance();

    /**
     * @brief Sets normalized crowd intensity.
     * @param excitement Expected range is [0, 1].
     */
    void setCrowdExcitement(float excitement);

    /**
     * @brief Indicates if audio backend is currently active.
     * @return true if audio was initialized and is ready.
     */
    bool isAvailable() const;

private:
    bool initialized_;
    float kickCooldown_;
    float crowdExcitement_;
    float cheerCooldown_;
    float celebrationBoostTimer_;
    float chanceBoostTimer_;
    float chantLfoPhase_;

#ifdef HAS_OPENAL
    unsigned int ambienceBuffer_;
    unsigned int chantBuffer_;
    unsigned int cheerBuffer_;
    unsigned int goalBuffer_;
    unsigned int kickBuffer_;

    unsigned int ambienceSource_;
    unsigned int chantSource_;
    unsigned int cheerSource_;
    unsigned int goalSource_;
    unsigned int kickSource_;

    void* device_;
    void* context_;

    bool initOpenAL();
    bool createBuffers();
    void destroyBuffers();
#endif
};
