#include "engine/audio/AudioBuffer.h"
#include "engine/core/Log.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace eng::audio {

// ── RIFF/WAV parser (16-bit PCM, mono or stereo) ─────────────────────────────

struct WavHeader {
    char     riff[4];
    uint32_t fileSize;
    char     wave[4];
};

struct ChunkHeader {
    char     id[4];
    uint32_t size;
};

struct FmtChunk {
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // 1 or 2
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample; // must be 16
};

static AudioBuffer loadWav(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("AudioBuffer: cannot open " + path);

    WavHeader hdr{};
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (std::strncmp(hdr.riff, "RIFF", 4) != 0 ||
        std::strncmp(hdr.wave, "WAVE", 4) != 0)
        throw std::runtime_error("AudioBuffer: not a WAV file: " + path);

    FmtChunk fmt{};
    bool fmtFound  = false;
    bool dataFound = false;
    std::vector<uint8_t> pcmData;

    while (f) {
        ChunkHeader ch{};
        f.read(reinterpret_cast<char*>(&ch), sizeof(ch));
        if (!f) break;

        if (std::strncmp(ch.id, "fmt ", 4) == 0) {
            f.read(reinterpret_cast<char*>(&fmt), sizeof(fmt));
            if (ch.size > sizeof(fmt))
                f.seekg(ch.size - sizeof(fmt), std::ios::cur);
            fmtFound = true;
        } else if (std::strncmp(ch.id, "data", 4) == 0) {
            pcmData.resize(ch.size);
            f.read(reinterpret_cast<char*>(pcmData.data()), ch.size);
            dataFound = true;
        } else {
            f.seekg(ch.size, std::ios::cur);
        }
    }

    if (!fmtFound || !dataFound)
        throw std::runtime_error("AudioBuffer: missing fmt/data chunk: " + path);
    if (fmt.audioFormat != 1)
        throw std::runtime_error("AudioBuffer: only PCM WAV supported: " + path);
    if (fmt.bitsPerSample != 16)
        throw std::runtime_error("AudioBuffer: only 16-bit WAV supported: " + path);

    ALenum format = (fmt.numChannels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    ALuint buf = 0;
    alGenBuffers(1, &buf);
    alBufferData(buf, format,
                 pcmData.data(), static_cast<ALsizei>(pcmData.size()),
                 static_cast<ALsizei>(fmt.sampleRate));

    if (alGetError() != AL_NO_ERROR) {
        alDeleteBuffers(1, &buf);
        throw std::runtime_error("AudioBuffer: alBufferData failed for: " + path);
    }

    LOG_INFO("AudioBuffer: loaded '{}' [{}ch @ {}Hz, {} bytes]",
             path, fmt.numChannels, fmt.sampleRate, pcmData.size());
    return AudioBuffer::adopt(buf);
}

// ── AudioBuffer impl ──────────────────────────────────────────────────────────

AudioBuffer::~AudioBuffer() {
    if (m_id) alDeleteBuffers(1, &m_id);
}

AudioBuffer::AudioBuffer(AudioBuffer&& o) noexcept : m_id(o.m_id) { o.m_id = 0; }

AudioBuffer& AudioBuffer::operator=(AudioBuffer&& o) noexcept {
    if (this != &o) {
        if (m_id) alDeleteBuffers(1, &m_id);
        m_id = o.m_id;
        o.m_id = 0;
    }
    return *this;
}

AudioBuffer AudioBuffer::fromFile(const std::string& path) {
    return loadWav(path);
}

} // namespace eng::audio
