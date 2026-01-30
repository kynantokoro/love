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

#include "Pool.h"
#include "Source.h"

namespace love
{
namespace audio
{
namespace webaudio
{

Pool::Pool()
{
	// mutex is auto-initialized by MutexRef's default constructor
}

Pool::~Pool()
{
}

bool Pool::isAvailable() const
{
	// Web Audio API can handle up to MAX_SOURCES concurrent sources
	return playing.size() < MAX_SOURCES;
}

bool Pool::isPlaying(Source *s)
{
	thread::Lock lock(mutex);
	return playing.find(s) != playing.end();
}

void Pool::update()
{
	thread::Lock lock(mutex);

	// Update all playing sources
	std::set<Source*> toRemove;
	for (auto it = playing.begin(); it != playing.end(); ++it)
	{
		Source *source = *it;
		if (source->update() && source->isFinished())
		{
			toRemove.insert(source);
		}
	}

	// Remove finished sources
	for (auto it = toRemove.begin(); it != toRemove.end(); ++it)
	{
		playing.erase(*it);
	}
}

int Pool::getActiveSourceCount() const
{
	return (int)playing.size();
}

int Pool::getMaxSources() const
{
	return MAX_SOURCES;
}

void Pool::addPlayingSource(Source *source)
{
	thread::Lock lock(mutex);
	playing.insert(source);
}

void Pool::removePlayingSource(Source *source)
{
	thread::Lock lock(mutex);
	playing.erase(source);
}

thread::Lock Pool::lock()
{
	return thread::Lock(mutex);
}

std::vector<love::audio::Source*> Pool::getPlayingSources()
{
	std::vector<love::audio::Source*> sources;
	sources.reserve(playing.size());

	for (auto it = playing.begin(); it != playing.end(); ++it)
	{
		sources.push_back(*it);
	}

	return sources;
}

} // webaudio
} // audio
} // love
