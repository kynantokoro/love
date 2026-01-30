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

#ifndef LOVE_AUDIO_WEBAUDIO_RECORDING_DEVICE_H
#define LOVE_AUDIO_WEBAUDIO_RECORDING_DEVICE_H

// LOVE
#include "audio/RecordingDevice.h"

namespace love
{
namespace audio
{
namespace webaudio
{

// Stub implementation - recording not supported in Phase 2
class RecordingDevice : public love::audio::RecordingDevice
{
public:
	RecordingDevice(const char *name) : love::audio::RecordingDevice(name) {}
	virtual ~RecordingDevice() {}

	virtual bool start(int samples, int sampleRate, int bitDepth, int channels) { return false; }
	virtual void stop() {}
	virtual love::sound::SoundData *getData() { return nullptr; }
	virtual const char *getName() const { return "Web Audio (Recording Not Supported)"; }
	virtual int getMaxSamples() const { return 0; }
	virtual int getSampleCount() const { return 0; }
	virtual int getSampleRate() const { return 0; }
	virtual int getBitDepth() const { return 0; }
	virtual int getChannelCount() const { return 0; }
	virtual bool isRecording() const { return false; }
};

} // webaudio
} // audio
} // love

#endif // LOVE_AUDIO_WEBAUDIO_RECORDING_DEVICE_H
