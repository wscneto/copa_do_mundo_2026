#pragma once

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool init();
    void shutdown();
    void update(float dt);

    void playGoal();
    void playKick();
    void playBigChance();
    void setCrowdExcitement(float excitement);

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
