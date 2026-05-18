#include "engine/audio/AudioSource.h"
#include <AL/al.h>
#include <utility>

namespace eng::audio {

AudioSource::AudioSource() {
    alGenSources(1, &m_id);
    if (alGetError() != AL_NO_ERROR) m_id = 0;
}

AudioSource::~AudioSource() {
    if (m_id) {
        alSourceStop(m_id);
        alDeleteSources(1, &m_id);
    }
}

AudioSource::AudioSource(AudioSource&& o) noexcept : m_id(o.m_id) { o.m_id = 0; }

AudioSource& AudioSource::operator=(AudioSource&& o) noexcept {
    if (this != &o) {
        if (m_id) { alSourceStop(m_id); alDeleteSources(1, &m_id); }
        m_id = o.m_id;
        o.m_id = 0;
    }
    return *this;
}

void AudioSource::setBuffer(ALuint bufferId) {
    if (m_id) alSourcei(m_id, AL_BUFFER, static_cast<ALint>(bufferId));
}

void AudioSource::setLooping(bool loop) {
    if (m_id) alSourcei(m_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
}

void AudioSource::setVolume(float v) {
    if (m_id) alSourcef(m_id, AL_GAIN, v < 0.f ? 0.f : v);
}

void AudioSource::setGain(float g) {
    if (m_id) alSourcef(m_id, AL_GAIN, g < 0.f ? 0.f : g);
}

void AudioSource::play() {
    if (m_id) alSourcePlay(m_id);
}

void AudioSource::stop() {
    if (m_id) alSourceStop(m_id);
}

void AudioSource::pause() {
    if (m_id) alSourcePause(m_id);
}

bool AudioSource::isPlaying() const {
    if (!m_id) return false;
    ALint state = AL_STOPPED;
    alGetSourcei(m_id, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

} // namespace eng::audio
