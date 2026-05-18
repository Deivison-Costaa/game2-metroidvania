#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "AudioBuffer.h"
#include "AudioSource.h"

namespace eng::audio {

// Singleton audio engine backed by OpenAL-soft.
// init() must be called before any play*() calls.
// shutdown() releases all OpenAL resources.
class AudioEngine {
public:
    static AudioEngine& instance();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Call once at app start; pass the directory that contains sfx/ and music/.
    void init(const std::string& assetRoot);
    void shutdown();

    // Play a one-shot SFX by filename (without extension), e.g. "attack".
    // Silently ignored if the file was not loaded or no free source is available.
    void playSfx(const std::string& name);

    // Load and start the music loop (stereo WAV).
    // Relative to assetRoot/music/. Stops any previously playing track.
    void playMusic(const std::string& filename);
    void stopMusic();
    void setMusicPaused(bool paused);

    // Volume controls [0..1].
    void setMasterVolume(float v);
    void setMusicVolume(float v);
    void setSfxVolume(float v);

    float masterVolume() const noexcept { return m_masterVol; }
    float musicVolume()  const noexcept { return m_musicVol;  }
    float sfxVolume()    const noexcept { return m_sfxVol;    }

private:
    AudioEngine() = default;
    ~AudioEngine() = default;

    void loadSfxDir(const std::string& sfxDir);
    void applyMusicGain();

    ALCdevice*  m_device  {nullptr};
    ALCcontext* m_context {nullptr};

    std::unordered_map<std::string, AudioBuffer> m_sfxBuffers;
    std::unique_ptr<AudioSource>                 m_musicSource;

    // Fixed pool of sources for concurrent SFX playback.
    static constexpr int kSfxPoolSize = 8;
    std::vector<AudioSource> m_sfxPool;
    int m_sfxNext{0};

    AudioBuffer m_musicBuffer;   // holds the currently-loaded music track
    std::string m_assetRoot;

    float m_masterVol{1.0f};
    float m_musicVol {0.7f};
    float m_sfxVol   {1.0f};

    bool m_ready{false};
};

} // namespace eng::audio
