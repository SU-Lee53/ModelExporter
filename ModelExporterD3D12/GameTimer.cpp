#include "stdafx.h"
#include "GameTimer.h"

float GameTimer::m_fTimeElapsed = 0.f;

GameTimer::GameTimer()
{
	::QueryPerformanceFrequency((LARGE_INTEGER*)&m_PerformanceFrequencePerSec);
	::QueryPerformanceCounter((LARGE_INTEGER*)&m_nLastPerformanceCounter);
	m_fTimeScale = 1.0 / (double)m_PerformanceFrequencePerSec;

	m_nBasePerformanceCounter = m_nLastPerformanceCounter;
}

void GameTimer::Tick(float fLockFPS)
{
	if (m_bStopped) {
		m_fTimeElapsed = 0.0f;
		return;
	}

	::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentPerformanceCounter);
	float fTimeElapsed = float((m_nCurrentPerformanceCounter - m_nLastPerformanceCounter) * m_fTimeScale);

	if (fLockFPS > 0.0f) {
		while (fTimeElapsed < (1.0f / fLockFPS)) {
			::QueryPerformanceCounter((LARGE_INTEGER*)&m_nCurrentPerformanceCounter);
			fTimeElapsed = float((m_nCurrentPerformanceCounter - m_nLastPerformanceCounter) * m_fTimeScale);
		}
	}

	m_nLastPerformanceCounter = m_nCurrentPerformanceCounter;

	if (fabsf(fTimeElapsed - m_fTimeElapsed) < 1.0f) {
		std::rotate(m_fFrameTime.rbegin(), m_fFrameTime.rbegin() + 1, m_fFrameTime.rend());
		m_fFrameTime[0] = fTimeElapsed;
		if (m_nSampleCount < MAX_SAMPLE_COUNT) m_nSampleCount++;
	}

	m_FramePerSecond++;
	m_fFPSTimeElapsed += fTimeElapsed;
	if (m_fFPSTimeElapsed > 1.0f) {
		m_nCurrentFrameRate = m_FramePerSecond;
		m_FramePerSecond = 0;
		m_fFPSTimeElapsed = 0.0f;
	}

	m_fTimeElapsed = 0.0f;
	std::for_each(m_fFrameTime.begin(), m_fFrameTime.end(), [this](const float& f) { this->m_fTimeElapsed += f; });
	if (m_nSampleCount > 0) m_fTimeElapsed /= m_nSampleCount;

}

void GameTimer::Start()
{
	INT64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	if (m_bStopped) {
		m_nPausedPerformanceCounter += (nPerformanceCounter - m_nStopPerformanceCounter);
		m_nLastPerformanceCounter = nPerformanceCounter;
		m_nStopPerformanceCounter = 0;
		m_bStopped = FALSE;
	}
}

void GameTimer::Stop()
{
	if (!m_bStopped) {
		::QueryPerformanceCounter((LARGE_INTEGER*)&m_nStopPerformanceCounter);
		m_bStopped = TRUE;
	}
}

void GameTimer::Reset()
{
	INT64 nPerformanceCounter;
	::QueryPerformanceCounter((LARGE_INTEGER*)&nPerformanceCounter);

	m_nBasePerformanceCounter = nPerformanceCounter;
	m_nLastPerformanceCounter = nPerformanceCounter;
	m_nStopPerformanceCounter = 0;
	m_bStopped = FALSE;
}

ULONG GameTimer::GetFrameRate(std::wstring_view wsvGameName, std::wstring& wstrString)
{
	wstrString = std::format(L"{} ({} FPS)", wsvGameName, m_nCurrentFrameRate);

	return m_nCurrentFrameRate;
}

float GameTimer::GetTotalTime()
{
	if (m_bStopped) {
		return float(((m_nStopPerformanceCounter - m_nPausedPerformanceCounter) - m_nBasePerformanceCounter) * m_fTimeScale);
	}

	return float(((m_nCurrentPerformanceCounter - m_nPausedPerformanceCounter) - m_nBasePerformanceCounter) * m_fTimeScale);;
}
