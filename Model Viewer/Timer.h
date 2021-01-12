#pragma once

// High resolution timer
class Timer
{
public:
	Timer() noexcept;
	virtual ~Timer() = default;
	Timer& operator=(const Timer&) = delete;
	Timer(const Timer&) = delete;

	// Start the timer
	void Start();

	// Stops the timer
	void Stop();

	// Resets the timer
	void Reset();

	// Advance the tick count. Called once per frame
	void Tick();

	// Current time since the last tick in ticks
	constexpr double DeltaTime() { return static_cast<double>(m_DeltaTime); }

	// Gets the total time elapsed since the timer was started in ticks
	double TotalTime() const;

	constexpr bool IsActive() { return m_Active; }

protected:
	double m_SecondsPerCount = 0.0;
	double m_DeltaTime = 0.0;

	__int64 m_BaseTime = 0;
	__int64 m_PausedTime = 0;
	__int64 m_StopTime = 0;
	__int64 m_PreviousTime = 0;
	__int64 m_CurrentTime = 0;

	bool m_Active = false;
	bool m_Stopped = false;
};