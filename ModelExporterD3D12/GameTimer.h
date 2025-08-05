#pragma once

const ULONG MAX_SAMPLE_COUNT = 50;

class GameTimer {
public:
	GameTimer();

	void Tick(float fLockFPS = 0.0f);
	void Start();
	void Stop();
	void Reset();

	ULONG GetFrameRate(std::wstring_view wsvGameName, std::wstring& wstrString);
	float GetTotalTime();


public:
	static float		m_fTimeElapsed;

private:
	double		m_fTimeScale = 0.0;

	INT64		m_nBasePerformanceCounter = 0;
	INT64		m_nPausedPerformanceCounter = 0;
	INT64		m_nStopPerformanceCounter = 0;
	INT64		m_nCurrentPerformanceCounter = 0;
	INT64		m_nLastPerformanceCounter = 0;

	INT64		m_PerformanceFrequencePerSec = 0;

	std::array<float, MAX_SAMPLE_COUNT> m_fFrameTime = {};
	ULONG		m_nSampleCount = 0;

	ULONG		m_nCurrentFrameRate = 0;
	ULONG		m_FramePerSecond = 0;
	float		m_fFPSTimeElapsed = 0.f;

	BOOL		m_bStopped = FALSE;

};

