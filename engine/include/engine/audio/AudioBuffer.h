#pragma once
#include <AL/al.h>
#include <string>

namespace eng::audio {

// RAII wrapper around an OpenAL buffer.
// Supports mono/stereo 16-bit PCM WAV files only (RIFF/WAVE format).
class AudioBuffer {
public:
    AudioBuffer() = default;
    ~AudioBuffer();

    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;
    AudioBuffer(AudioBuffer&& o) noexcept;
    AudioBuffer& operator=(AudioBuffer&& o) noexcept;

    // Load from a 16-bit PCM WAV file (mono or stereo, any sample rate).
    static AudioBuffer fromFile(const std::string& path);

    ALuint id() const noexcept { return m_id; }
    bool   valid() const noexcept { return m_id != 0; }

    // Package-private factory called by fromFile implementation.
    static AudioBuffer adopt(ALuint id) { return AudioBuffer{id}; }

private:
    explicit AudioBuffer(ALuint id) : m_id(id) {}
    ALuint m_id{0};
};

} // namespace eng::audio
