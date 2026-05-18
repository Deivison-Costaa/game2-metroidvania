#include "engine/audio/AudioEngine.h"
#include "engine/core/Log.h"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace eng::audio {

AudioEngine& AudioEngine::instance() {
    static AudioEngine s_inst;
    return s_inst;
}

void AudioEngine::init(const std::string& assetRoot) {
    m_assetRoot = assetRoot;

    m_device = alcOpenDevice(nullptr);
    if (!m_device) throw std::runtime_error("AudioEngine: cannot open AL device");

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context || !alcMakeContextCurrent(m_context))
        throw std::runtime_error("AudioEngine: cannot create AL context");

    m_sfxPool.resize(kSfxPoolSize);
    m_musicSource = std::make_unique<AudioSource>();

    loadSfxDir(assetRoot + "/sfx");

    m_ready = true;

    const ALCchar* devName = alcGetString(m_device, ALC_DEVICE_SPECIFIER);
    LOG_INFO("AudioEngine: device='{}' | master={:.1f} music={:.1f} sfx={:.1f}",
             devName ? devName : "default",
             m_masterVol, m_musicVol, m_sfxVol);
}

void AudioEngine::loadSfxDir(const std::string& sfxDir) {
    if (!fs::exists(sfxDir)) {
        LOG_WARN("AudioEngine: SFX directory not found: {}", sfxDir);
        return;
    }
    for (const auto& entry : fs::directory_iterator(sfxDir)) {
        if (!entry.is_regular_file()) continue;
        const auto& p = entry.path();
        if (p.extension() != ".wav") continue;
        try {
            m_sfxBuffers.emplace(p.stem().string(), AudioBuffer::fromFile(p.string()));
        } catch (const std::exception& ex) {
            LOG_WARN("AudioEngine: skip '{}': {}", p.string(), ex.what());
        }
    }
}

void AudioEngine::shutdown() {
    if (!m_ready) return;
    m_sfxPool.clear();
    m_musicSource.reset();
    m_sfxBuffers.clear();
    alcMakeContextCurrent(nullptr);
    if (m_context) { alcDestroyContext(m_context); m_context = nullptr; }
    if (m_device)  { alcCloseDevice(m_device);     m_device  = nullptr; }
    m_ready = false;
}

void AudioEngine::playSfx(const std::string& name) {
    if (!m_ready) return;
    auto it = m_sfxBuffers.find(name);
    if (it == m_sfxBuffers.end()) return;

    // Round-robin over the SFX pool
    for (int i = 0; i < kSfxPoolSize; ++i) {
        int idx = (m_sfxNext + i) % kSfxPoolSize;
        auto& src = m_sfxPool[idx];
        if (!src.isPlaying()) {
            src.setBuffer(it->second.id());
            src.setLooping(false);
            src.setGain(m_masterVol * m_sfxVol);
            src.play();
            m_sfxNext = (idx + 1) % kSfxPoolSize;
            return;
        }
    }
    // All sources busy — steal the oldest one (m_sfxNext)
    auto& src = m_sfxPool[m_sfxNext];
    src.stop();
    src.setBuffer(it->second.id());
    src.setLooping(false);
    src.setGain(m_masterVol * m_sfxVol);
    src.play();
    m_sfxNext = (m_sfxNext + 1) % kSfxPoolSize;
}

void AudioEngine::playMusic(const std::string& filename) {
    if (!m_ready || !m_musicSource) return;
    m_musicSource->stop();

    const std::string path = m_assetRoot + "/music/" + filename;
    try {
        m_musicBuffer = AudioBuffer::fromFile(path);
        m_musicSource->setBuffer(m_musicBuffer.id());
    } catch (const std::exception& ex) {
        LOG_WARN("AudioEngine: playMusic failed '{}': {}", path, ex.what());
        return;
    }

    m_musicSource->setLooping(true);
    applyMusicGain();
    m_musicSource->play();
}

void AudioEngine::stopMusic() {
    if (m_musicSource) m_musicSource->stop();
}

void AudioEngine::setMusicPaused(bool paused) {
    if (!m_musicSource) return;
    if (paused) m_musicSource->pause();
    else        m_musicSource->play();
}

void AudioEngine::applyMusicGain() {
    if (m_musicSource) m_musicSource->setGain(m_masterVol * m_musicVol);
}

void AudioEngine::setMasterVolume(float v) {
    m_masterVol = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    applyMusicGain();
    for (auto& s : m_sfxPool) s.setGain(m_masterVol * m_sfxVol);
}

void AudioEngine::setMusicVolume(float v) {
    m_musicVol = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
    applyMusicGain();
}

void AudioEngine::setSfxVolume(float v) {
    m_sfxVol = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
}

} // namespace eng::audio
