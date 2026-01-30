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

#ifndef LOVE_AUDIO_WEBAUDIO_AUDIO_H
#define LOVE_AUDIO_WEBAUDIO_AUDIO_H

// STD
#include <queue>
#include <map>
#include <vector>
#include <cmath>

// LOVE
#include "audio/Audio.h"
#include "audio/RecordingDevice.h"
#include "audio/Filter.h"
#include "common/config.h"
#include "sound/SoundData.h"

#include "Source.h"
#include "Pool.h"
#include "thread/threads.h"

namespace love
{
namespace audio
{
namespace webaudio
{

class Audio : public love::audio::Audio
{
public:

	Audio();
	~Audio();

	// Implements Module.
	const char *getName() const;

	// Implements Audio.
	love::audio::Source *newSource(love::sound::Decoder *decoder);
	love::audio::Source *newSource(love::sound::SoundData *soundData);
	love::audio::Source *newSource(int sampleRate, int bitDepth, int channels, int buffers);
	int getActiveSourceCount() const;
	int getMaxSources() const;
	bool play(love::audio::Source *source);
	bool play(const std::vector<love::audio::Source*> &sources);
	void stop(love::audio::Source *source);
	void stop(const std::vector<love::audio::Source*> &sources);
	void stop();
	void pause(love::audio::Source *source);
	void pause(const std::vector<love::audio::Source*> &sources);
	std::vector<love::audio::Source*> pause();
	void pauseContext();
	void resumeContext();
	void setVolume(float volume);
	float getVolume() const;

	void getPosition(float *v) const;
	void setPosition(float *v);
	void getOrientation(float *v) const;
	void setOrientation(float *v);
	void getVelocity(float *v) const;
	void setVelocity(float *v);

	void setDopplerScale(float scale);
	float getDopplerScale() const;

	const std::vector<love::audio::RecordingDevice*> &getRecordingDevices();

	DistanceModel getDistanceModel() const;
	void setDistanceModel(DistanceModel distanceModel);

	bool setEffect(const char *name, std::map<Effect::Parameter, float> &params);
	bool unsetEffect(const char *name);
	bool getEffect(const char *name, std::map<Effect::Parameter, float> &params);
	bool getActiveEffects(std::vector<std::string> &list) const;
	int getMaxSceneEffects() const;
	int getMaxSourceEffects() const;
	bool isEFXsupported() const;

	// The Pool.
	Pool *pool;

private:
	// Master volume
	float volume;

	// Empty recording devices vector
	std::vector<love::audio::RecordingDevice*> capture;

	// Distance model
	DistanceModel distanceModel;

}; // Audio

} // webaudio
} // audio
} // love

#endif // LOVE_AUDIO_WEBAUDIO_AUDIO_H
