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

#include "Source.h"
#include "Pool.h"
#include "Audio.h"

#include <emscripten.h>

// Helper macro for one-time warnings about unimplemented features
#define WEBAUDIO_WARN_ONCE(msg) \
	do { \
		static bool warned = false; \
		if (!warned) { \
			EM_ASM({ console.warn('[WebAudio] ' + UTF8ToString($0)); }, msg); \
			warned = true; \
		} \
	} while(0)

namespace love
{
namespace audio
{
namespace webaudio
{

int Source::nextSourceId = 1;

Source::Source(Pool *pool, love::sound::SoundData *soundData)
	: love::audio::Source(TYPE_STATIC)
	, pool(pool)
	, sourceId(nextSourceId++)
	, staticBuffer(soundData)
	, playing(false)
	, paused(false)
	, looping(false)
	, volume(1.0f)
	, pitch(1.0f)
	, channels(soundData->getChannelCount())
	, sampleRate(soundData->getSampleRate())
{
	// Upload audio data to Web Audio API
	const void* pcmData = soundData->getData();
	int samples = soundData->getSampleCount();
	int bitDepth = soundData->getBitDepth();

	EM_ASM({
		var ctx = Module.loveAudioContext;
		if (!ctx) {
			console.error('[Web Audio] AudioContext not initialized!');
			return;
		}

		try {
			// Create AudioBuffer
			var buffer = ctx.createBuffer($1, $2, $3);

			// Copy PCM data from WASM heap to AudioBuffer
			// soundData is interleaved, need to de-interleave for Web Audio
			var dataPtr = $5;
			var bytesPerSample = $4 / 8;
			var view;

			if (bytesPerSample == 2) {
				// 16-bit signed PCM
				view = new Int16Array(HEAPU8.buffer, dataPtr, $2 * $1);
			} else {
				// 8-bit unsigned PCM
				view = new Uint8Array(HEAPU8.buffer, dataPtr, $2 * $1);
			}

			// De-interleave and copy to AudioBuffer channels
			for (var ch = 0; ch < $1; ch++) {
				var channelData = buffer.getChannelData(ch);
				for (var i = 0; i < $2; i++) {
					var sample = view[i * $1 + ch];

					// Convert to float [-1, 1]
					if (bytesPerSample == 2) {
						channelData[i] = sample / 32768.0;
					} else {
						channelData[i] = (sample - 128) / 128.0;
					}
				}
			}

			Module.audioBuffers[$0] = buffer;
		} catch (e) {
			console.error('[Web Audio] Failed to create static source:', e);
		}
	}, this->sourceId, this->channels, samples, this->sampleRate, bitDepth, (int)pcmData);

	this->position[0] = this->position[1] = this->position[2] = 0.0f;
	this->velocity[0] = this->velocity[1] = this->velocity[2] = 0.0f;
	this->direction[0] = 0.0f; this->direction[1] = 0.0f; this->direction[2] = -1.0f;
}

Source::Source(Pool *pool, love::sound::Decoder *decoder)
	: love::audio::Source(TYPE_STREAM)
	, pool(pool)
	, sourceId(nextSourceId++)
	, decoder(decoder)
	, streamFilename("")
	, playing(false)
	, paused(false)
	, looping(false)
	, volume(1.0f)
	, pitch(1.0f)
	, channels(decoder->getChannelCount())
	, sampleRate(decoder->getSampleRate())
{
	this->position[0] = this->position[1] = this->position[2] = 0.0f;
	this->velocity[0] = this->velocity[1] = this->velocity[2] = 0.0f;
	this->direction[0] = 0.0f; this->direction[1] = 0.0f; this->direction[2] = -1.0f;

	// Create MediaElementSourceNode for streaming audio
	int srcId = this->sourceId;
	float volVal = this->volume;
	int loopVal = this->looping ? 1 : 0;

	EM_ASM({
		console.log('[Web Audio] Creating streaming source, ID:', $0);

		var ctx = Module.loveAudioContext;
		if (!ctx) {
			console.error('[Web Audio] Cannot create streaming source: no audio context');
			return;
		}

		// Initialize audio elements storage
		if (!Module.audioElements) {
			Module.audioElements = {};
		}

		// Create HTML5 Audio element
		// Note: We'll set the src when we have the filename (during play)
		var audio = new Audio();
		audio.loop = $1;
		audio.preload = 'auto';

		// Create MediaElementSourceNode
		var mediaSource = ctx.createMediaElementSource(audio);

		// Create gain node for volume control
		var gainNode = ctx.createGain();
		gainNode.gain.value = $2;

		// Connect: mediaSource -> gain -> master -> destination
		mediaSource.connect(gainNode);
		gainNode.connect(Module.audioMasterGain);

		// Store reference
		var elemInfo = {};
		elemInfo.audio = audio;
		elemInfo.source = mediaSource;
		elemInfo.gain = gainNode;
		Module.audioElements[$0] = elemInfo;

		console.log('[Web Audio] Streaming source created, ID:', $0, 'Elements count:', Object.keys(Module.audioElements).length);

	}, srcId, loopVal, volVal);
}

Source::Source(Pool *pool, int sampleRate, int bitDepth, int channels, int buffers)
	: love::audio::Source(TYPE_QUEUE)
	, pool(pool)
	, sourceId(nextSourceId++)
	, playing(false)
	, paused(false)
	, looping(false)
	, volume(1.0f)
	, pitch(1.0f)
	, channels(channels)
	, sampleRate(sampleRate)
{
	// TODO: Implement queueable source if needed
	printf("[Web Audio] Queue source created (not yet implemented), ID: %d\n", this->sourceId);

	this->position[0] = this->position[1] = this->position[2] = 0.0f;
	this->velocity[0] = this->velocity[1] = this->velocity[2] = 0.0f;
	this->direction[0] = 0.0f; this->direction[1] = 0.0f; this->direction[2] = -1.0f;
}

Source::Source(const Source &s)
	: love::audio::Source(s.sourceType)
	, pool(s.pool)
	, sourceId(nextSourceId++)
	, staticBuffer(s.staticBuffer)
	, decoder(s.decoder)
	, playing(false)
	, paused(false)
	, looping(s.looping)
	, volume(s.volume)
	, pitch(s.pitch)
	, channels(s.channels)
	, sampleRate(s.sampleRate)
{
	// Clone shares the same audio buffer but gets a new source ID
	if (this->sourceType == TYPE_STATIC && this->staticBuffer.get())
	{
		// Share the audio buffer by copying the buffer reference
		EM_ASM({
			// Share the buffer - clone's ID points to same buffer as original
			if (Module.audioBuffers[$1]) {
				Module.audioBuffers[$0] = Module.audioBuffers[$1];
			} else {
				console.error('[Web Audio] Original buffer not found for source', $1);
			}
		}, this->sourceId, s.sourceId);
	}

	this->position[0] = s.position[0]; this->position[1] = s.position[1]; this->position[2] = s.position[2];
	this->velocity[0] = s.velocity[0]; this->velocity[1] = s.velocity[1]; this->velocity[2] = s.velocity[2];
	this->direction[0] = s.direction[0]; this->direction[1] = s.direction[1]; this->direction[2] = s.direction[2];
}

Source::~Source()
{
	stop();

	// Cleanup audio buffer (only if we're the last reference)
	if (this->sourceType == TYPE_STATIC)
	{
		EM_ASM({
			// Note: We don't delete the buffer here because clones share it
			// The buffer will be cleaned up when the AudioContext is destroyed
		}, this->sourceId);
	}
}

void Source::reset()
{
	// Stop playback and reset state
	stop();
}

love::audio::Source *Source::clone()
{
	return new Source(*this);
}

bool Source::play()
{
	// Handle streaming sources
	if (this->sourceType == TYPE_STREAM)
	{
		this->playing = true;
		this->paused = false;

		this->pool->addPlayingSource(this);

		// Set audio source and play
		const char* filenameCStr = this->streamFilename.c_str();
		int srcId = this->sourceId;

		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				var filename = UTF8ToString($1);
				var ctx = Module.loveAudioContext;

				// Convert virtual filesystem path to HTTP URL
				// assets/audio/music.ogg → /assets/audio/music.ogg
				var url = filename.startsWith('/') ? filename : '/' + filename;

				console.log('[Web Audio] play() called - URL:', url);
				console.log('[Web Audio] play() called - AudioContext state:', ctx ? ctx.state : 'no context');
				console.log('[Web Audio] play() called - current paused state:', elem.audio.paused);

				// Set the src only if not paused (to allow resume from pause position)
				// or if the URL has changed
				var needsNewSrc = !elem.audio.paused || !elem.audio.src || elem.audio.src.indexOf(url) === -1;
				if (needsNewSrc) {
					elem.audio.src = url;
					console.log('[Web Audio] play() - src updated');
				} else {
					console.log('[Web Audio] play() - resuming from pause, keeping current position');
				}

				// Resume AudioContext if suspended, THEN play audio
				var playPromise;
				if (ctx && ctx.state === 'suspended') {
					console.log('[Web Audio] Resuming suspended AudioContext...');
					playPromise = ctx.resume().then(function() {
						console.log('[Web Audio] AudioContext resumed successfully, state:', ctx.state);
						return elem.audio.play();
					});
				} else {
					playPromise = elem.audio.play();
				}

				playPromise.then(function() {
					console.log('[Web Audio] play() - playback started successfully');
					console.log('[Web Audio] play() - AudioContext state after play:', ctx ? ctx.state : 'no context');
				}).catch(function(e) {
					console.error('[Web Audio] Failed to play streaming audio:', e);
				});
			} else {
				console.error('[Web Audio] play() - no audio element found for ID:', $0);
			}
		}, srcId, filenameCStr);

		return true;
	}

	// Handle static sources
	if (this->sourceType != TYPE_STATIC)
	{
		return false;
	}

	if (this->playing && !this->paused)
	{
		// Already playing
		return true;
	}

	if (this->paused)
	{
		// Pause acts as stop, so just clear the flag and continue
		this->paused = false;
	}

	// Start new playback
	this->playing = true;
	this->paused = false;

	this->pool->addPlayingSource(this);

	// Store parameters in local variables for EM_ASM
	int bufferId = this->sourceId;
	int srcId = this->sourceId;
	int isLooping = this->looping ? 1 : 0;
	float pitchVal = this->pitch;
	float volumeVal = this->volume;

	EM_ASM({
		var ctx = Module.loveAudioContext;
		var buffer = Module.audioBuffers[$0];

		if (!ctx || !buffer) {
			console.error('[Web Audio] Cannot play: context or buffer missing');
			return;
		}

		// Function to actually start playback
		var startPlayback = function() {
			// Stop existing playback if any
			if (Module.audioSources[$1]) {
				try {
					Module.audioSources[$1].source.stop();
				} catch(e) {}
				delete Module.audioSources[$1];
			}

			// Create new BufferSourceNode
			var source = ctx.createBufferSource();
			source.buffer = buffer;
			source.loop = $2;
			source.playbackRate.value = $3;

			// Volume control via GainNode
			var gainNode = ctx.createGain();
			gainNode.gain.value = $4;

			// Connect: source -> gain -> master -> destination
			source.connect(gainNode);
			gainNode.connect(Module.audioMasterGain);

			// Setup ended callback
			var srcId = $1;
			source.onended = function() {
				delete Module.audioSources[srcId];
			};

			// Start playback
			source.start(0);

			// Store reference
			var sourceInfo = {};
			sourceInfo.source = source;
			sourceInfo.gain = gainNode;
			sourceInfo.startTime = ctx.currentTime;
			Module.audioSources[$1] = sourceInfo;
		};

		// Resume AudioContext if suspended, THEN start playback
		if (ctx.state === 'suspended') {
			console.log('[Web Audio] Resuming suspended AudioContext for static source...');
			ctx.resume().then(function() {
				console.log('[Web Audio] AudioContext resumed for static source, state:', ctx.state);
				startPlayback();
			});
		} else {
			startPlayback();
		}
	}, bufferId, srcId, isLooping, pitchVal, volumeVal);

	return true;
}

void Source::stop()
{
	if (!this->playing)
		return;

	this->playing = false;
	this->paused = false;

	this->pool->removePlayingSource(this);

	if (this->sourceType == TYPE_STREAM)
	{
		// Stop streaming audio
		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				elem.audio.pause();
				elem.audio.currentTime = 0;
			}
		}, this->sourceId);
	}
	else
	{
		// Stop static audio
		EM_ASM({
			if (Module.audioSources[$0]) {
				try {
					Module.audioSources[$0].source.stop();
				} catch(e) {}
				delete Module.audioSources[$0];
			}
		}, this->sourceId);
	}
}

void Source::pause()
{
	if (!this->playing || this->paused)
		return;

	// Handle streaming sources
	if (this->sourceType == TYPE_STREAM)
	{
		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				elem.audio.pause();
				console.log('[Web Audio] pause() - streaming source paused');
			}
		}, this->sourceId);

		this->paused = true;
		this->playing = false;
		return;
	}

	// For static sources, still use stop (Web Audio API doesn't have pause for BufferSourceNode)
	WEBAUDIO_WARN_ONCE("pause() not fully implemented for static sources - use stop() instead");
	stop();
	this->paused = true;
}

bool Source::isPlaying() const
{
	if (!this->playing || this->paused)
		return false;

	// Check if the source is still playing in JavaScript
	if (this->sourceType == TYPE_STREAM)
	{
		// Check streaming audio element
		int stillPlaying = EM_ASM_INT({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				var isPaused = elem.audio.paused;
				var isEnded = elem.audio.ended;
				console.log('[Web Audio] isPlaying check - paused:', isPaused, 'ended:', isEnded);
				return !isPaused && !isEnded ? 1 : 0;
			}
			console.log('[Web Audio] isPlaying check - no audio element found');
			return 0;
		}, this->sourceId);
		return stillPlaying != 0;
	}
	else
	{
		// Check static audio source
		int stillPlaying = EM_ASM_INT({
			return Module.audioSources[$0] ? 1 : 0;
		}, this->sourceId);
		return stillPlaying != 0;
	}
}

bool Source::isFinished() const
{
	return !isPlaying();
}

bool Source::update()
{
	// Check if playback has finished
	if (this->playing && !isPlaying())
	{
		this->playing = false;
		this->paused = false;
		return true; // Signal that source finished
	}

	return false;
}

void Source::setPitch(float pitch)
{
	this->pitch = pitch;

	if (this->playing)
	{
		if (this->sourceType == TYPE_STATIC)
		{
			EM_ASM({
				if (Module.audioSources[$0]) {
					Module.audioSources[$0].source.playbackRate.value = $1;
				}
			}, this->sourceId, pitch);
		}
		else if (this->sourceType == TYPE_STREAM)
		{
			WEBAUDIO_WARN_ONCE("setPitch() not implemented for streaming sources");
		}
	}
}

float Source::getPitch() const
{
	return this->pitch;
}

void Source::setVolume(float volume)
{
	this->volume = volume;

	if (this->sourceType == TYPE_STREAM)
	{
		// Set volume for streaming audio
		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				Module.audioElements[$0].gain.gain.value = $1;
			}
		}, this->sourceId, volume);
	}
	else if (this->playing)
	{
		// Set volume for static audio
		EM_ASM({
			if (Module.audioSources[$0]) {
				Module.audioSources[$0].gain.gain.value = $1;
			}
		}, this->sourceId, volume);
	}
}

float Source::getVolume() const
{
	return this->volume;
}

void Source::seek(double offset, Unit unit)
{
	WEBAUDIO_WARN_ONCE("seek() not implemented");
}

double Source::tell(Unit unit)
{
	WEBAUDIO_WARN_ONCE("tell() not implemented - playback position tracking not supported");
	return 0.0;
}

double Source::getDuration(Unit unit)
{
	if (this->sourceType == TYPE_STATIC && this->staticBuffer.get())
	{
		int samples = this->staticBuffer->getSampleCount();
		int rate = this->staticBuffer->getSampleRate();

		if (unit == UNIT_SAMPLES)
			return samples;
		else
			return (double)samples / (double)rate;
	}
	else if (this->sourceType == TYPE_STREAM)
	{
		WEBAUDIO_WARN_ONCE("getDuration() not implemented for streaming sources");
		return 0.0;
	}

	return 0.0;
}

void Source::setPosition(float *v)
{
	WEBAUDIO_WARN_ONCE("setPosition() not implemented - 3D spatial audio not supported");

	this->position[0] = v[0];
	this->position[1] = v[1];
	this->position[2] = v[2];
}

void Source::getPosition(float *v) const
{
	v[0] = this->position[0];
	v[1] = this->position[1];
	v[2] = this->position[2];
}

void Source::setVelocity(float *v)
{
	WEBAUDIO_WARN_ONCE("setVelocity() not implemented - Doppler effect not supported");

	this->velocity[0] = v[0];
	this->velocity[1] = v[1];
	this->velocity[2] = v[2];
}

void Source::getVelocity(float *v) const
{
	v[0] = this->velocity[0];
	v[1] = this->velocity[1];
	v[2] = this->velocity[2];
}

void Source::setDirection(float *v)
{
	WEBAUDIO_WARN_ONCE("setDirection() not implemented - directional audio not supported");

	this->direction[0] = v[0];
	this->direction[1] = v[1];
	this->direction[2] = v[2];
}

void Source::getDirection(float *v) const
{
	v[0] = this->direction[0];
	v[1] = this->direction[1];
	v[2] = this->direction[2];
}

void Source::setCone(float innerAngle, float outerAngle, float outerVolume, float outerHighGain)
{
	WEBAUDIO_WARN_ONCE("setCone() not implemented - audio cone shaping not supported");
}

void Source::getCone(float &innerAngle, float &outerAngle, float &outerVolume, float &outerHighGain) const
{
	innerAngle = 360.0f;
	outerAngle = 360.0f;
	outerVolume = 0.0f;
	outerHighGain = 1.0f;
}

void Source::setRelative(bool enable)
{
	WEBAUDIO_WARN_ONCE("setRelative() not implemented - relative positioning not supported");
}

bool Source::isRelative() const
{
	return false;
}

void Source::setLooping(bool looping)
{
	this->looping = looping;

	if (this->sourceType == TYPE_STREAM)
	{
		// Set looping for streaming audio
		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				Module.audioElements[$0].audio.loop = $1;
			}
		}, this->sourceId, looping);
	}
	else if (this->playing)
	{
		// Set looping for static audio
		EM_ASM({
			if (Module.audioSources[$0]) {
				Module.audioSources[$0].source.loop = $1;
			}
		}, this->sourceId, looping);
	}
}

bool Source::isLooping() const
{
	return this->looping;
}

void Source::setMinVolume(float volume)
{
	WEBAUDIO_WARN_ONCE("setMinVolume() not implemented - volume clamping not supported");
}

float Source::getMinVolume() const
{
	return 0.0f;
}

void Source::setMaxVolume(float volume)
{
	WEBAUDIO_WARN_ONCE("setMaxVolume() not implemented - volume clamping not supported");
}

float Source::getMaxVolume() const
{
	return 1.0f;
}

void Source::setReferenceDistance(float distance)
{
	WEBAUDIO_WARN_ONCE("setReferenceDistance() not implemented - distance attenuation not supported");
}

float Source::getReferenceDistance() const
{
	return 1.0f;
}

void Source::setRolloffFactor(float factor)
{
	WEBAUDIO_WARN_ONCE("setRolloffFactor() not implemented - distance rolloff not supported");
}

float Source::getRolloffFactor() const
{
	return 1.0f;
}

void Source::setMaxDistance(float distance)
{
	WEBAUDIO_WARN_ONCE("setMaxDistance() not implemented - distance clamping not supported");
}

float Source::getMaxDistance() const
{
	return 1000000.0f;
}

void Source::setAirAbsorptionFactor(float factor)
{
	WEBAUDIO_WARN_ONCE("setAirAbsorptionFactor() not implemented - air absorption not supported");
}

float Source::getAirAbsorptionFactor() const
{
	return 0.0f;
}

int Source::getChannelCount() const
{
	return this->channels;
}

bool Source::setFilter(const std::map<Filter::Parameter, float> &params)
{
	WEBAUDIO_WARN_ONCE("setFilter() not implemented - audio filters not supported");
	return false;
}

bool Source::setFilter()
{
	return false;
}

bool Source::getFilter(std::map<Filter::Parameter, float> &params)
{
	return false;
}

bool Source::setEffect(const char *effect)
{
	WEBAUDIO_WARN_ONCE("setEffect() not implemented - audio effects not supported");
	return false;
}

bool Source::setEffect(const char *effect, const std::map<Filter::Parameter, float> &params)
{
	WEBAUDIO_WARN_ONCE("setEffect() not implemented - audio effects not supported");
	return false;
}

bool Source::unsetEffect(const char *effect)
{
	return false;
}

bool Source::getEffect(const char *effect, std::map<Filter::Parameter, float> &params)
{
	return false;
}

bool Source::getActiveEffects(std::vector<std::string> &list) const
{
	return false;
}

int Source::getFreeBufferCount() const
{
	// For queueable sources
	return 0;
}

bool Source::queue(void *data, size_t length, int dataSampleRate, int dataBitDepth, int dataChannels)
{
	WEBAUDIO_WARN_ONCE("queue() not implemented - queueable sources not supported");
	return false;
}

void Source::setStreamFilename(const std::string &filename)
{
	this->streamFilename = filename;

	// Set audio.src immediately to allow browser preloading
	// This eliminates the delay when play() is called
	if (this->sourceType == TYPE_STREAM && !filename.empty())
	{
		int srcId = this->sourceId;
		EM_ASM({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				var filename = UTF8ToString($1);

				// Convert virtual filesystem path to HTTP URL (same as in play())
				// assets/audio/music.ogg → /assets/audio/music.ogg
				var url = filename.startsWith('/') ? filename : '/' + filename;

				// Set src to start preloading
				elem.audio.src = url;
				console.log('[Web Audio] setStreamFilename - preloading started:', url);
			} else {
				console.warn('[Web Audio] setStreamFilename - audio element not found for ID:', $0);
			}
		}, srcId, filename.c_str());
	}
}

bool Source::isReady() const
{
	if (this->sourceType == TYPE_STREAM)
	{
		// Check HTML5 Audio readyState
		// readyState values:
		// 0 = HAVE_NOTHING - no data
		// 1 = HAVE_METADATA - metadata loaded
		// 2 = HAVE_CURRENT_DATA - data for current position
		// 3 = HAVE_FUTURE_DATA - enough data to play a bit
		// 4 = HAVE_ENOUGH_DATA - enough data to play through
		int srcId = this->sourceId;
		int ready = EM_ASM_INT({
			if (Module.audioElements && Module.audioElements[$0]) {
				var elem = Module.audioElements[$0];
				// Consider ready if we have enough data to play through (readyState >= 4)
				// or if we at least have future data (readyState >= 3)
				var state = elem.audio.readyState;
				console.log('[Web Audio] isReady check - readyState:', state);
				return state >= 3 ? 1 : 0;
			}
			return 0;
		}, srcId);
		return ready != 0;
	}
	else if (this->sourceType == TYPE_STATIC)
	{
		// Static sources are always ready once created
		return true;
	}

	// Queue sources are always ready
	return true;
}

} // webaudio
} // audio
} // love
