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

#ifndef LOVE_AUDIO_WEBAUDIO_POOL_H
#define LOVE_AUDIO_WEBAUDIO_POOL_H

// STD
#include <queue>
#include <map>
#include <vector>
#include <set>

// LOVE
#include "common/config.h"
#include "common/Exception.h"
#include "thread/threads.h"
#include "audio/Source.h"

namespace love
{
namespace audio
{
namespace webaudio
{

class Source;

class Pool
{
public:

	Pool();
	~Pool();

	/**
	 * Checks whether a source slot is available.
	 * @return True if at least one is available, false otherwise.
	 **/
	bool isAvailable() const;

	/**
	 * Checks whether a Source is currently in the playing list.
	 **/
	bool isPlaying(Source *s);

	void update();

	int getActiveSourceCount() const;
	int getMaxSources() const;

	void addPlayingSource(Source *source);
	void removePlayingSource(Source *source);

	LOVE_WARN_UNUSED thread::Lock lock();
	std::vector<love::audio::Source*> getPlayingSources();

private:

	friend class Source;

	// Maximum possible number of simultaneous sources.
	static const int MAX_SOURCES = 64;

	// Set of currently playing sources
	std::set<Source*> playing;

	// Only one thread can access this object at the same time.
	love::thread::MutexRef mutex;

}; // Pool

} // webaudio
} // audio
} // love

#endif // LOVE_AUDIO_WEBAUDIO_POOL_H
