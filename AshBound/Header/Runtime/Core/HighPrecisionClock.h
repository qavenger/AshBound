#pragma once

#include <cstdint>

class HighPrecisionClock
{
public:
	HighPrecisionClock();

	void Reset();
	void Tick();

	double GetDeltaSeconds() const;
	double GetTotalSeconds() const;
	double GetDeltaMilliseconds() const;
	double GetTotalMilliseconds() const;

private:
	static int64_t GetFrequency();
	static double TicksToSeconds(int64_t ticks);

	int64_t m_startTicks = 0;
	int64_t m_lastTicks = 0;
	int64_t m_currentTicks = 0;
};
