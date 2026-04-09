#include "AudioSystem.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef HAS_OPENAL
#include <AL/al.h>
#include <AL/alc.h>
#endif

namespace {
constexpr int SAMPLE_RATE = 22050;
constexpr float PI = 3.14159265f;

struct WavPcmData {
    std::vector<short> samples;
    int sampleRate;
};

float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

void decayTimer(float& timer, float dt) {
    if (timer > 0.0f) {
        timer = std::max(0.0f, timer - dt);
    }
}

float pseudoNoise(int seed) {
    const float x = std::sin(static_cast<float>(seed) * 12.9898f) * 43758.5453f;
    return (x - std::floor(x)) * 2.0f - 1.0f;
}

bool hasExtension(const std::string& fileName, const char* ext) {
    const std::string extension(ext);
    if (fileName.size() < extension.size()) {
        return false;
    }

    const std::string tail = fileName.substr(fileName.size() - extension.size());
    std::string lowerTail = tail;
    std::transform(lowerTail.begin(), lowerTail.end(), lowerTail.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    return lowerTail == extension;
}

uint32_t readLe32(std::istream& input) {
    unsigned char bytes[4] = {0, 0, 0, 0};
    input.read(reinterpret_cast<char*>(bytes), 4);
    return static_cast<uint32_t>(bytes[0]) |
        (static_cast<uint32_t>(bytes[1]) << 8) |
        (static_cast<uint32_t>(bytes[2]) << 16) |
        (static_cast<uint32_t>(bytes[3]) << 24);
}

uint16_t readLe16(std::istream& input) {
    unsigned char bytes[2] = {0, 0};
    input.read(reinterpret_cast<char*>(bytes), 2);
    return static_cast<uint16_t>(bytes[0]) | (static_cast<uint16_t>(bytes[1]) << 8);
}

bool loadWavPcm16(const std::string& filePath, WavPcmData& outData) {
    std::ifstream input(filePath.c_str(), std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    char riff[4] = {};
    char wave[4] = {};
    input.read(riff, 4);
    (void)readLe32(input);
    input.read(wave, 4);

    if (!input.good() || std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE") {
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t channelCount = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    std::vector<unsigned char> rawData;

    while (input.good()) {
        char chunkId[4] = {};
        input.read(chunkId, 4);
        if (!input.good()) {
            break;
        }

        const uint32_t chunkSize = readLe32(input);
        if (!input.good()) {
            break;
        }

        const std::string id(chunkId, 4);
        if (id == "fmt ") {
            audioFormat = readLe16(input);
            channelCount = readLe16(input);
            sampleRate = readLe32(input);
            (void)readLe32(input);
            (void)readLe16(input);
            bitsPerSample = readLe16(input);

            if (chunkSize > 16) {
                input.seekg(static_cast<std::streamoff>(chunkSize - 16), std::ios::cur);
            }
        } else if (id == "data") {
            rawData.resize(chunkSize);
            if (chunkSize > 0) {
                input.read(reinterpret_cast<char*>(rawData.data()), static_cast<std::streamsize>(chunkSize));
            }
        } else {
            input.seekg(static_cast<std::streamoff>(chunkSize), std::ios::cur);
        }

        if (chunkSize % 2 != 0) {
            input.seekg(1, std::ios::cur);
        }
    }

    if (audioFormat != 1 || bitsPerSample != 16 || sampleRate == 0 || rawData.empty()) {
        return false;
    }

    std::vector<short> decoded;
    const size_t sampleCount = rawData.size() / sizeof(short);
    decoded.resize(sampleCount);

    for (size_t i = 0; i < sampleCount; ++i) {
        const unsigned char lo = rawData[i * 2];
        const unsigned char hi = rawData[i * 2 + 1];
        decoded[i] = static_cast<short>((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo));
    }

    if (channelCount == 2) {
        std::vector<short> mono;
        mono.reserve(decoded.size() / 2);
        for (size_t i = 0; i + 1 < decoded.size(); i += 2) {
            const int mixed = static_cast<int>(decoded[i]) + static_cast<int>(decoded[i + 1]);
            mono.push_back(static_cast<short>(mixed / 2));
        }
        decoded.swap(mono);
    } else if (channelCount != 1) {
        return false;
    }

    outData.samples = decoded;
    outData.sampleRate = static_cast<int>(sampleRate);
    return !outData.samples.empty();
}

bool ffmpegAvailable() {
    return std::system("ffmpeg -version >/dev/null 2>&1") == 0;
}

bool convertToWavWithFfmpeg(const std::string& inputPath, const std::string& outputPath) {
    if (!ffmpegAvailable()) {
        return false;
    }

    const std::string command =
        "ffmpeg -y -loglevel error -i \"" + inputPath + "\" -ac 1 -ar 22050 -sample_fmt s16 \"" + outputPath + "\"";
    return std::system(command.c_str()) == 0;
}

bool loadExternalCrowdAudio(WavPcmData& outData) {
    namespace fs = std::filesystem;

    const fs::path soundsDir("sounds");
    if (!fs::exists(soundsDir) || !fs::is_directory(soundsDir)) {
        return false;
    }

    std::vector<fs::path> candidates;
    for (const fs::directory_entry& entry : fs::directory_iterator(soundsDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::string pathStr = entry.path().string();
        if (hasExtension(pathStr, ".wav") ||
            hasExtension(pathStr, ".mp3") ||
            hasExtension(pathStr, ".ogg") ||
            hasExtension(pathStr, ".flac") ||
            hasExtension(pathStr, ".m4a")) {
            candidates.push_back(entry.path());
        }
    }

    std::sort(candidates.begin(), candidates.end());
    bool printedConversionHint = false;

    for (const fs::path& candidate : candidates) {
        WavPcmData loaded;
        const std::string candidatePath = candidate.string();

        if (hasExtension(candidatePath, ".wav") && loadWavPcm16(candidatePath, loaded)) {
            outData = loaded;
            std::cout << "Audio externo carregado: " << candidatePath << std::endl;
            return true;
        }

        const std::string tempWav = "/tmp/worldcup2026_external_crowd.wav";
        if (convertToWavWithFfmpeg(candidatePath, tempWav) && loadWavPcm16(tempWav, loaded)) {
            outData = loaded;
            std::cout << "Audio externo carregado: " << candidatePath << std::endl;
            return true;
        }

        if (!printedConversionHint && !hasExtension(candidatePath, ".wav")) {
            std::cerr
                << "Nao foi possivel converter audio externo (talvez ffmpeg ausente). "
                << "Use WAV PCM 16-bit em sounds/ para garantir carregamento."
                << std::endl;
            printedConversionHint = true;
        }
    }

    return false;
}

short toPCM16(float value) {
    const float clamped = std::max(-1.0f, std::min(1.0f, value));
    return static_cast<short>(clamped * 32767.0f);
}

std::vector<short> generateCrowdAmbience() {
    const int samples = SAMPLE_RATE * 2;
    std::vector<short> data(samples);

    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        const float low = std::sin(2.0f * PI * 63.0f * t) * 0.12f;
        const float mid = std::sin(2.0f * PI * 130.0f * t + std::sin(t * 0.3f) * 0.4f) * 0.10f;
        const float high = std::sin(2.0f * PI * 420.0f * t) * 0.03f;
        const float slowPulse = (std::sin(t * 2.0f) + 1.0f) * 0.5f;
        const float hiss = pseudoNoise(i * 7) * 0.05f;
        const float mixed = (low + mid + high + hiss) * (0.60f + 0.40f * slowPulse);
        data[i] = toPCM16(mixed);
    }

    return data;
}

std::vector<short> generateCrowdChant() {
    const int samples = SAMPLE_RATE * 3;
    std::vector<short> data(samples);

    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        const float beatCycle = std::fmod(t * 1.55f, 1.0f);

        const float peakA = std::exp(-44.0f * (beatCycle - 0.16f) * (beatCycle - 0.16f));
        const float peakB = std::exp(-44.0f * (beatCycle - 0.62f) * (beatCycle - 0.62f));
        const float chantEnvelope = clamp01((peakA + peakB) * 0.90f);

        const float vowelA = std::sin(2.0f * PI * 220.0f * t);
        const float vowelB = std::sin(2.0f * PI * 330.0f * t + std::sin(t * 1.8f) * 0.33f) * 0.66f;
        const float consonant = pseudoNoise(i * 11 + 3) * 0.25f;

        const float mixed = (vowelA + vowelB + consonant) * chantEnvelope * 0.45f;
        data[i] = toPCM16(mixed);
    }

    return data;
}

std::vector<short> generateCheerBurst() {
    const int samples = static_cast<int>(SAMPLE_RATE * 0.95f);
    std::vector<short> data(samples);

    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        const float u = t / 0.95f;
        const float envelope = std::exp(-2.6f * u);
        const float roarBand = std::sin(2.0f * PI * (170.0f + 210.0f * u) * t) * 0.55f;
        const float harmonic = std::sin(2.0f * PI * 480.0f * t + std::sin(t * 3.0f) * 0.8f) * 0.25f;
        const float noise = pseudoNoise(i * 13 + 19) * 0.42f;
        const float mixed = (roarBand + harmonic + noise) * envelope;
        data[i] = toPCM16(mixed * 0.88f);
    }

    return data;
}

std::vector<short> generateGoalTone() {
    const int samples = static_cast<int>(SAMPLE_RATE * 0.9f);
    std::vector<short> data(samples);

    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        const float u = t / 0.9f;
        const float frequency = 420.0f + 380.0f * u;
        const float envelope = std::exp(-2.5f * u);
        const float wobble = std::sin(2.0f * PI * 6.0f * t) * 0.07f;
        const float sample = std::sin(2.0f * PI * frequency * t + wobble) * envelope;
        data[i] = toPCM16(sample * 0.95f);
    }

    return data;
}

std::vector<short> generateKickThump() {
    const int samples = static_cast<int>(SAMPLE_RATE * 0.15f);
    std::vector<short> data(samples);

    for (int i = 0; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(SAMPLE_RATE);
        const float envelope = std::exp(-22.0f * t);
        const float low = std::sin(2.0f * PI * 90.0f * t);
        const float click = std::sin(2.0f * PI * 360.0f * t) * std::exp(-60.0f * t) * 0.3f;
        data[i] = toPCM16((low * 0.9f + click) * envelope);
    }

    return data;
}

#ifdef HAS_OPENAL
void ensureLoopingSourcePlaying(unsigned int source) {
    if (source == 0) {
        return;
    }

    ALint state = 0;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
    }
}
#endif
}  // namespace

AudioSystem::AudioSystem()
    : initialized_(false),
    kickCooldown_(0.0f),
    crowdExcitement_(0.0f),
    cheerCooldown_(0.0f),
    celebrationBoostTimer_(0.0f),
    chanceBoostTimer_(0.0f),
    chantLfoPhase_(0.0f)
#ifdef HAS_OPENAL
      ,
      ambienceBuffer_(0),
    chantBuffer_(0),
    cheerBuffer_(0),
      goalBuffer_(0),
      kickBuffer_(0),
      ambienceSource_(0),
    chantSource_(0),
    cheerSource_(0),
      goalSource_(0),
      kickSource_(0),
      device_(nullptr),
      context_(nullptr)
#endif
{
}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    if (initialized_) {
        return true;
    }

#ifdef HAS_OPENAL
    if (!initOpenAL()) {
        std::cerr << "OpenAL nao inicializado. Jogo continua sem audio 3D." << std::endl;
        return false;
    }

    if (!createBuffers()) {
        std::cerr << "Falha ao criar buffers de audio." << std::endl;
        shutdown();
        return false;
    }
#endif

    initialized_ = true;
    return true;
}

void AudioSystem::shutdown() {
#ifdef HAS_OPENAL
    if (chantSource_ != 0) {
        alDeleteSources(1, &chantSource_);
        chantSource_ = 0;
    }
    if (cheerSource_ != 0) {
        alDeleteSources(1, &cheerSource_);
        cheerSource_ = 0;
    }
    if (ambienceSource_ != 0) {
        alDeleteSources(1, &ambienceSource_);
        ambienceSource_ = 0;
    }
    if (goalSource_ != 0) {
        alDeleteSources(1, &goalSource_);
        goalSource_ = 0;
    }
    if (kickSource_ != 0) {
        alDeleteSources(1, &kickSource_);
        kickSource_ = 0;
    }

    destroyBuffers();

    if (context_ != nullptr) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(reinterpret_cast<ALCcontext*>(context_));
        context_ = nullptr;
    }

    if (device_ != nullptr) {
        alcCloseDevice(reinterpret_cast<ALCdevice*>(device_));
        device_ = nullptr;
    }
#endif

    initialized_ = false;
}

void AudioSystem::update(float dt) {
    decayTimer(kickCooldown_, dt);
    decayTimer(cheerCooldown_, dt);
    decayTimer(celebrationBoostTimer_, dt);
    decayTimer(chanceBoostTimer_, dt);

    chantLfoPhase_ += dt * 1.75f;

#ifdef HAS_OPENAL
    if (initialized_) {
        ensureLoopingSourcePlaying(ambienceSource_);
        ensureLoopingSourcePlaying(chantSource_);
    }

    if (initialized_) {
        // Boost ambience and chants from match context (danger + celebration timers).
        const float celebrationNorm = clamp01(celebrationBoostTimer_ / 2.8f);
        const float chanceNorm = clamp01(chanceBoostTimer_ / 1.1f);

        const float ambienceGain = 0.70f + crowdExcitement_ * 0.60f + celebrationNorm * 0.25f + chanceNorm * 0.15f;
        alSourcef(ambienceSource_, AL_GAIN, std::min(1.4f, ambienceGain));

        const float chantLfo = 0.60f + 0.45f * std::sin(chantLfoPhase_);
        const float chantBase = std::max(0.0f, crowdExcitement_ - 0.01f) * 1.40f;
        const float chantGain = chantBase * chantLfo + celebrationNorm * 0.35f + chanceNorm * 0.20f;
        alSourcef(chantSource_, AL_GAIN, std::min(1.4f, chantGain));
    }
#endif
}

void AudioSystem::playGoal() {
    celebrationBoostTimer_ = std::max(celebrationBoostTimer_, 2.8f);
    chanceBoostTimer_ = std::max(chanceBoostTimer_, 1.2f);

#ifdef HAS_OPENAL
    if (initialized_ && goalSource_ != 0) {
        alSourceRewind(goalSource_);
        alSourcePlay(goalSource_);

        if (cheerSource_ != 0) {
            alSourcef(cheerSource_, AL_GAIN, 0.42f);
            alSourceRewind(cheerSource_);
            alSourcePlay(cheerSource_);
        }
        return;
    }
#endif
    std::system("printf '\\a'");
}

void AudioSystem::playKick() {
    if (kickCooldown_ > 0.0f) {
        return;
    }

    kickCooldown_ = 0.09f;

#ifdef HAS_OPENAL
    if (initialized_ && kickSource_ != 0) {
        alSourceRewind(kickSource_);
        alSourcePlay(kickSource_);
        return;
    }
#endif
    std::system("printf '\\a'");
}

void AudioSystem::playBigChance() {
    if (cheerCooldown_ > 0.0f) {
        return;
    }

    cheerCooldown_ = 0.65f;
    chanceBoostTimer_ = std::max(chanceBoostTimer_, 1.0f);

#ifdef HAS_OPENAL
    if (initialized_ && cheerSource_ != 0) {
        alSourcef(cheerSource_, AL_GAIN, 0.30f);
        alSourceRewind(cheerSource_);
        alSourcePlay(cheerSource_);
        return;
    }
#endif
}

void AudioSystem::setCrowdExcitement(float excitement) {
    crowdExcitement_ = clamp01(excitement);
}

bool AudioSystem::isAvailable() const {
    return initialized_;
}

#ifdef HAS_OPENAL
bool AudioSystem::initOpenAL() {
    device_ = alcOpenDevice(nullptr);
    if (device_ == nullptr) {
        return false;
    }

    context_ = alcCreateContext(reinterpret_cast<ALCdevice*>(device_), nullptr);
    if (context_ == nullptr) {
        alcCloseDevice(reinterpret_cast<ALCdevice*>(device_));
        device_ = nullptr;
        return false;
    }

    if (!alcMakeContextCurrent(reinterpret_cast<ALCcontext*>(context_))) {
        alcDestroyContext(reinterpret_cast<ALCcontext*>(context_));
        context_ = nullptr;
        alcCloseDevice(reinterpret_cast<ALCdevice*>(device_));
        device_ = nullptr;
        return false;
    }

    return true;
}

bool AudioSystem::createBuffers() {
    WavPcmData externalCrowd;
    const bool usingExternalCrowd = loadExternalCrowdAudio(externalCrowd);

    const std::vector<short> ambience = usingExternalCrowd ? externalCrowd.samples : generateCrowdAmbience();
    const int ambienceSampleRate = usingExternalCrowd ? externalCrowd.sampleRate : SAMPLE_RATE;

    const std::vector<short> chant = generateCrowdChant();
    const std::vector<short> cheer = generateCheerBurst();
    const std::vector<short> goal = generateGoalTone();
    const std::vector<short> kick = generateKickThump();

    alGenBuffers(1, &ambienceBuffer_);
    alGenBuffers(1, &chantBuffer_);
    alGenBuffers(1, &cheerBuffer_);
    alGenBuffers(1, &goalBuffer_);
    alGenBuffers(1, &kickBuffer_);

    alBufferData(
        ambienceBuffer_,
        AL_FORMAT_MONO16,
        ambience.data(),
        static_cast<ALsizei>(ambience.size() * sizeof(short)),
        static_cast<ALsizei>(ambienceSampleRate)
    );

    alBufferData(
        chantBuffer_,
        AL_FORMAT_MONO16,
        chant.data(),
        static_cast<ALsizei>(chant.size() * sizeof(short)),
        SAMPLE_RATE
    );

    alBufferData(
        cheerBuffer_,
        AL_FORMAT_MONO16,
        cheer.data(),
        static_cast<ALsizei>(cheer.size() * sizeof(short)),
        SAMPLE_RATE
    );

    alBufferData(
        goalBuffer_,
        AL_FORMAT_MONO16,
        goal.data(),
        static_cast<ALsizei>(goal.size() * sizeof(short)),
        SAMPLE_RATE
    );

    alBufferData(
        kickBuffer_,
        AL_FORMAT_MONO16,
        kick.data(),
        static_cast<ALsizei>(kick.size() * sizeof(short)),
        SAMPLE_RATE
    );

    alGenSources(1, &ambienceSource_);
    alGenSources(1, &chantSource_);
    alGenSources(1, &cheerSource_);
    alGenSources(1, &goalSource_);
    alGenSources(1, &kickSource_);

    alSourcei(ambienceSource_, AL_BUFFER, static_cast<ALint>(ambienceBuffer_));
    alSourcei(ambienceSource_, AL_LOOPING, AL_TRUE);
    alSourcef(ambienceSource_, AL_GAIN, 0.85f);

    alSourcei(chantSource_, AL_BUFFER, static_cast<ALint>(chantBuffer_));
    alSourcei(chantSource_, AL_LOOPING, AL_TRUE);
    alSourcef(chantSource_, AL_GAIN, 0.60f);

    alSourcei(cheerSource_, AL_BUFFER, static_cast<ALint>(cheerBuffer_));
    alSourcef(cheerSource_, AL_GAIN, 0.28f);

    alSourcei(goalSource_, AL_BUFFER, static_cast<ALint>(goalBuffer_));
    alSourcef(goalSource_, AL_GAIN, 0.48f);

    alSourcei(kickSource_, AL_BUFFER, static_cast<ALint>(kickBuffer_));
    alSourcef(kickSource_, AL_GAIN, 0.18f);

    alSourcePlay(ambienceSource_);
    alSourcePlay(chantSource_);

    return alGetError() == AL_NO_ERROR;
}

void AudioSystem::destroyBuffers() {
    if (ambienceBuffer_ != 0) {
        alDeleteBuffers(1, &ambienceBuffer_);
        ambienceBuffer_ = 0;
    }
    if (chantBuffer_ != 0) {
        alDeleteBuffers(1, &chantBuffer_);
        chantBuffer_ = 0;
    }
    if (cheerBuffer_ != 0) {
        alDeleteBuffers(1, &cheerBuffer_);
        cheerBuffer_ = 0;
    }
    if (goalBuffer_ != 0) {
        alDeleteBuffers(1, &goalBuffer_);
        goalBuffer_ = 0;
    }
    if (kickBuffer_ != 0) {
        alDeleteBuffers(1, &kickBuffer_);
        kickBuffer_ = 0;
    }
}
#endif
