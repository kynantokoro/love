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

#include "Audio.h"
#include "Source.h"
#include "Pool.h"

#include <emscripten.h>

namespace love
{
namespace audio
{
namespace webaudio
{

Audio::Audio()
	: volume(1.0f)
	, distanceModel(DISTANCE_INVERSE_CLAMPED)
{
	// Initialize Web Audio API context
	EM_ASM({
		Module.LOVE_AUDIO_BACKEND = 'webaudio';

		if (!Module.loveAudioContext) {
			var AudioContext = window.AudioContext || window.webkitAudioContext;
			try {
				Module.loveAudioContext = new AudioContext();
				Module.audioBuffers = {};  // Store AudioBuffers by ID
				Module.audioSources = {};  // Store active source nodes by ID
				Module.audioElements = {}; // Store HTML5 Audio elements for streaming

				// Master gain node for volume control
				Module.audioMasterGain = Module.loveAudioContext.createGain();
				Module.audioMasterGain.connect(Module.loveAudioContext.destination);
				Module.audioMasterGain.gain.value = 1.0;
			} catch (e) {
				console.error('[Web Audio] Failed to initialize AudioContext:', e);
			}
		}
	});

	// Create the Pool
	pool = new Pool();
}

Audio::~Audio()
{
	delete pool;

	// Cleanup Web Audio API context
	EM_ASM({
		if (Module.loveAudioContext) {
			// Stop all sources
			for (var id in Module.audioSources) {
				try {
					Module.audioSources[id].source.stop();
				} catch(e) {}
			}

			// Stop all HTML5 audio elements
			for (var id in Module.audioElements) {
				try {
					Module.audioElements[id].audio.pause();
				} catch(e) {}
			}

			Module.loveAudioContext.close();
			Module.loveAudioContext = null;
			Module.audioBuffers = null;
			Module.audioSources = null;
			Module.audioElements = null;
			Module.audioMasterGain = null;
		}
	});
}

const char *Audio::getName() const
{
	return "Web Audio API";
}

love::audio::Source *Audio::newSource(love::sound::Decoder *decoder)
{
	return new Source(pool, decoder);
}

love::audio::Source *Audio::newSource(love::sound::SoundData *soundData)
{
	return new Source(pool, soundData);
}

love::audio::Source *Audio::newSource(int sampleRate, int bitDepth, int channels, int buffers)
{
	return new Source(pool, sampleRate, bitDepth, channels, buffers);
}

int Audio::getActiveSourceCount() const
{
	return pool ? pool->getActiveSourceCount() : 0;
}

int Audio::getMaxSources() const
{
	return pool ? pool->getMaxSources() : 0;
}

bool Audio::play(love::audio::Source *source)
{
	return source->play();
}

bool Audio::play(const std::vector<love::audio::Source*> &sources)
{
	bool success = true;
	for (auto source : sources)
	{
		success = source->play() && success;
	}
	return success;
}

void Audio::stop(love::audio::Source *source)
{
	source->stop();
}

void Audio::stop(const std::vector<love::audio::Source*> &sources)
{
	for (auto source : sources)
	{
		source->stop();
	}
}

void Audio::stop()
{
	if (pool)
	{
		thread::Lock lock(pool->lock());
		std::vector<love::audio::Source*> sources = pool->getPlayingSources();
		for (auto source : sources)
		{
			source->stop();
		}
	}
}

void Audio::pause(love::audio::Source *source)
{
	source->pause();
}

void Audio::pause(const std::vector<love::audio::Source*> &sources)
{
	for (auto source : sources)
	{
		source->pause();
	}
}

std::vector<love::audio::Source*> Audio::pause()
{
	std::vector<love::audio::Source*> sources;

	if (pool)
	{
		thread::Lock lock(pool->lock());
		sources = pool->getPlayingSources();
		for (auto source : sources)
		{
			source->pause();
		}
	}

	return sources;
}

void Audio::pauseContext()
{
	EM_ASM({
		if (Module.loveAudioContext) {
			Module.loveAudioContext.suspend();
		}
	});
}

void Audio::resumeContext()
{
	EM_ASM({
		if (Module.loveAudioContext) {
			Module.loveAudioContext.resume();
		}
	});
}

void Audio::setVolume(float volume)
{
	this->volume = volume;

	EM_ASM({
		if (Module.audioMasterGain) {
			Module.audioMasterGain.gain.value = $0;
		}
	}, volume);
}

float Audio::getVolume() const
{
	return volume;
}

void Audio::getPosition(float *v) const
{
	// Stub - spatial audio not implemented in Phase 2
	v[0] = v[1] = v[2] = 0.0f;
}

void Audio::setPosition(float *v)
{
	// Stub - spatial audio not implemented in Phase 2
}

void Audio::getOrientation(float *v) const
{
	// Stub - spatial audio not implemented in Phase 2
	v[0] = 0.0f; v[1] = 0.0f; v[2] = -1.0f;
	v[3] = 0.0f; v[4] = 1.0f; v[5] = 0.0f;
}

void Audio::setOrientation(float *v)
{
	// Stub - spatial audio not implemented in Phase 2
}

void Audio::getVelocity(float *v) const
{
	// Stub - spatial audio not implemented in Phase 2
	v[0] = v[1] = v[2] = 0.0f;
}

void Audio::setVelocity(float *v)
{
	// Stub - spatial audio not implemented in Phase 2
}

void Audio::setDopplerScale(float scale)
{
	// Stub - spatial audio not implemented in Phase 2
}

float Audio::getDopplerScale() const
{
	// Stub - spatial audio not implemented in Phase 2
	return 1.0f;
}

const std::vector<love::audio::RecordingDevice*> &Audio::getRecordingDevices()
{
	// Return empty vector - recording not supported in Phase 2
	return capture;
}

Audio::DistanceModel Audio::getDistanceModel() const
{
	return distanceModel;
}

void Audio::setDistanceModel(DistanceModel distanceModel)
{
	this->distanceModel = distanceModel;
}

bool Audio::setEffect(const char *name, std::map<Effect::Parameter, float> &params)
{
	// Stub - effects not supported in Phase 2
	return false;
}

bool Audio::unsetEffect(const char *name)
{
	// Stub - effects not supported in Phase 2
	return false;
}

bool Audio::getEffect(const char *name, std::map<Effect::Parameter, float> &params)
{
	// Stub - effects not supported in Phase 2
	return false;
}

bool Audio::getActiveEffects(std::vector<std::string> &list) const
{
	// Stub - effects not supported in Phase 2
	return false;
}

int Audio::getMaxSceneEffects() const
{
	// Stub - effects not supported in Phase 2
	return 0;
}

int Audio::getMaxSourceEffects() const
{
	// Stub - effects not supported in Phase 2
	return 0;
}

bool Audio::isEFXsupported() const
{
	// Stub - effects not supported in Phase 2
	return false;
}

} // webaudio
} // audio
} // love
