#include "Pch.h"
#include "Timer.h"

Timer::Timer() noexcept
{
	auto countsPerSecond = SDL_GetPerformanceFrequency();
	m_SecondsPerCount = 1.0 / static_cast<double>(countsPerSecond);
	Reset();
}

void Timer::Start()
{
	auto startTime = SDL_GetPerformanceCounter();
	m_Active = true;

	if (m_Stopped)
	{
		m_PausedTime += (startTime - m_StopTime);

		m_PreviousTime = startTime;
		m_StopTime = 0;
		m_Stopped = false;
	}
}

void Timer::Stop()
{
	if (!m_Stopped)
	{
		auto currTime = SDL_GetPerformanceCounter();

		m_StopTime = currTime;
		m_Stopped = true;
	}
}

void Timer::Reset()
{
	auto currentTime = SDL_GetPerformanceCounter();

	m_BaseTime = currentTime;
	m_PreviousTime = currentTime;
	m_StopTime = 0;
	m_Stopped = false;
	m_Active = false;
}

void Timer::Tick()
{
	if (m_Stopped)
	{
		m_DeltaTime = 0.0;
		return;
	}

	m_CurrentTime = SDL_GetPerformanceCounter();

	// Time difference between this frame and the previous.
	m_DeltaTime =  (m_CurrentTime - m_PreviousTime) * m_SecondsPerCount;
	m_DeltaTime = std::max<double>(m_DeltaTime, 0.0);

	// Prepare for next frame.
	m_PreviousTime = m_CurrentTime;
}

double Timer::TotalTime() const
{
	if (m_Stopped)
	{
		return static_cast<double>(((m_StopTime - m_PausedTime) - m_BaseTime) * m_SecondsPerCount);
	}
	else
	{
		return static_cast<double>(((m_CurrentTime - m_PausedTime) - m_BaseTime) * m_SecondsPerCount);
	}
}
