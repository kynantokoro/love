/**
 * Copyright (c) 2006-2022 LOVE Development Team
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 **/

#ifndef LOVE_AUDIO_WEBAUDIO_SOURCE_H
#define LOVE_AUDIO_WEBAUDIO_SOURCE_H

// LOVE
#include "common/config.h"
#include "common/Object.h"
#include "audio/Source.h"
#include "audio/Filter.h"
#include "sound/SoundData.h"
#include "sound/Decoder.h"
#include "Audio.h"

// STL
#include <vector>

namespace love
{
namespace audio
{
namespace webaudio
{

class Audio;
class Pool;

class Source : public love::audio::Source
{
public:

	Source(Pool *pool, love::sound::SoundData *soundData);
	Source(Pool *pool, love::sound::Decoder *decoder);
	Source(Pool *pool, int sampleRate, int bitDepth, int channels, int buffers);
	Source(const Source &s);
	virtual ~Source();

	virtual love::audio::Source *clone();
	virtual bool play();
	virtual void stop();
	virtual void pause();
	virtual bool isPlaying() const;
	virtual bool isFinished() const;
	virtual bool update();
	virtual void setPitch(float pitch);
	virtual float getPitch() const;
	virtual void setVolume(float volume);
	virtual void setVolume(float volume, float rampTime);  // Web Audio-specific: volume with ramp
	virtual float getVolume() const;
	virtual void seek(double offset, Unit unit);
	virtual double tell(Unit unit);
	virtual double getDuration(Unit unit);
	virtual void setPosition(float *v);
	virtual void getPosition(float *v) const;
	virtual void setVelocity(float *v);
	virtual void getVelocity(float *v) const;
	virtual void setDirection(float *v);
	virtual void getDirection(float *v) const;
	virtual void setCone(float innerAngle, float outerAngle, float outerVolume, float outerHighGain);
	virtual void getCone(float &innerAngle, float &outerAngle, float &outerVolume, float &outerHighGain) const;
	virtual void setRelative(bool enable);
	virtual bool isRelative() const;
	void setLooping(bool looping);
	bool isLooping() const;
	virtual void setMinVolume(float volume);
	virtual float getMinVolume() const;
	virtual void setMaxVolume(float volume);
	virtual float getMaxVolume() const;
	virtual void setReferenceDistance(float distance);
	virtual float getReferenceDistance() const;
	virtual void setRolloffFactor(float factor);
	virtual float getRolloffFactor() const;
	virtual void setMaxDistance(float distance);
	virtual float getMaxDistance() const;
	virtual void setAirAbsorptionFactor(float factor);
	virtual float getAirAbsorptionFactor() const;
	virtual int getChannelCount() const;

	virtual bool setFilter(const std::map<Filter::Parameter, float> &params);
	virtual bool setFilter();
	virtual bool getFilter(std::map<Filter::Parameter, float> &params);

	virtual bool setEffect(const char *effect);
	virtual bool setEffect(const char *effect, const std::map<Filter::Parameter, float> &params);
	virtual bool unsetEffect(const char *effect);
	virtual bool getEffect(const char *effect, std::map<Filter::Parameter, float> &params);
	virtual bool getActiveEffects(std::vector<std::string> &list) const;

	virtual int getFreeBufferCount() const;
	virtual bool queue(void *data, size_t length, int dataSampleRate, int dataBitDepth, int dataChannels);

	// Set filename for streaming sources
	void setStreamFilename(const std::string &filename);

	// Check if streaming source is ready to play (buffered enough data)
	bool isReady() const;

private:

	void reset();

	Pool *pool;

	// Unique ID for this source instance
	int sourceId;

	// Static source data
	StrongRef<love::sound::SoundData> staticBuffer;

	// Streaming decoder (for TYPE_STREAM)
	StrongRef<love::sound::Decoder> decoder;

	// Filename for streaming sources (TYPE_STREAM)
	std::string streamFilename;

	// Playback state
	bool playing;
	bool paused;
	bool looping;

	// Audio properties
	float volume;
	float pitch;

	// Channel info
	int channels;
	int sampleRate;

	// Position/spatial (stubbed for bare minimum)
	float position[3];
	float velocity[3];
	float direction[3];

	// Unique source ID counter
	static int nextSourceId;

}; // Source

} // webaudio
} // audio
} // love

#endif // LOVE_AUDIO_WEBAUDIO_SOURCE_H
