#pragma once
#include <AL/al.h>
#include <memory>
#include "AudioBuffer.h"

namespace eng::audio {

// RAII wrapper around an OpenAL source.
// A source pairs a buffer with playback state (position, volume, looping).
class AudioSource {
public:
    AudioSource();
    ~AudioSource();

    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;
    AudioSource(AudioSource&& o) noexcept;
    AudioSource& operator=(AudioSource&& o) noexcept;

    bool valid() const noexcept { return m_id != 0; }
    ALuint id()  const noexcept { return m_id; }

    void setBuffer(ALuint bufferId);
    void setLooping(bool loop);
    void setVolume(float v);   // [0..1]
    void setGain(float g);     // raw AL gain (allows > 1)
    void play();
    void stop();
    void pause();
    bool isPlaying() const;

private:
    ALuint m_id{0};
};

} // namespace eng::audio
