#pragma once

const static int KEY_TYPE_COUNT = 256;

enum class KEY_STATE
{
	NONE,
	PRESS,
	DOWN,
	UP,
	END
};

class InputManager
{
public:
	void Initialize(HWND hWnd);
	void Update();

public:
	bool GetButtonDown(UCHAR key);
	bool GetButtonPressed(UCHAR key);
	bool GetButtonUp(UCHAR key);

private:
	void UpdateKeyboard();
	void UpdateMouse();

public:
	const POINT& GetCurrentCursorPos() { return m_ptCurrentCursorPos; }
	const POINT& GetOldCursorPos() { return m_ptOldCursorPos; }
	POINT GetCursorDeltaPos() { return POINT{ m_ptCurrentCursorPos.x - m_ptOldCursorPos.x, m_ptCurrentCursorPos.y - m_ptOldCursorPos.y }; }

	void ShowCursor() {
		if (!m_bShowCursor) {
			m_bShowCursor = TRUE;
			::ShowCursor(TRUE);
		}
	}

	void HideCursor() {
		if (m_bShowCursor) {
			m_bShowCursor = FALSE;
			::ShowCursor(FALSE);
		}
	}

	BOOL IsCursorShown() { return m_bShowCursor; }

private:
	inline KEY_STATE GetState(UCHAR key) { return m_eKeyStates[key]; }


private:
	HWND m_hWnd = NULL;
	std::array<KEY_STATE, KEY_TYPE_COUNT> m_eKeyStates = {};

	POINT m_ptCurrentCursorPos = {};
	POINT m_ptOldCursorPos = {};

	BOOL		m_bShowCursor = TRUE;
};

