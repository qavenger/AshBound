#include "Runtime/Core/HighPrecisionClock.h"

#include <Windows.h>

namespace
{
	int64_t QueryTicks()
	{
		LARGE_INTEGER value = {};
		QueryPerformanceCounter(&value);
		return value.QuadPart;
	}
}

HighPrecisionClock::HighPrecisionClock()
{
	Reset();
}

void HighPrecisionClock::Reset()
{
	m_startTicks = QueryTicks();
	m_lastTicks = m_startTicks;
	m_currentTicks = m_startTicks;
}

void HighPrecisionClock::Tick()
{
	m_lastTicks = m_currentTicks;
	m_currentTicks = QueryTicks();
}

double HighPrecisionClock::GetDeltaSeconds() const
{
	const int64_t deltaTicks = m_currentTicks - m_lastTicks;
	return TicksToSeconds(deltaTicks);
}

double HighPrecisionClock::GetTotalSeconds() const
{
	const int64_t totalTicks = m_currentTicks - m_startTicks;
	return TicksToSeconds(totalTicks);
}

double HighPrecisionClock::GetDeltaMilliseconds() const
{
	return GetDeltaSeconds() * 1000.0;
}

double HighPrecisionClock::GetTotalMilliseconds() const
{
	return GetTotalSeconds() * 1000.0;
}

int64_t HighPrecisionClock::GetFrequency()
{
	static int64_t frequency = 0;
	if (frequency == 0)
	{
		LARGE_INTEGER freq = {};
		QueryPerformanceFrequency(&freq);
		frequency = freq.QuadPart;
	}
	return frequency;
}

double HighPrecisionClock::TicksToSeconds(int64_t ticks)
{
	return static_cast<double>(ticks) / static_cast<double>(GetFrequency());
}
