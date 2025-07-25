#include "stdafx.h"
#include "InputManager.h"

void InputManager::Initialize(HWND hWnd)
{
	m_hWnd = hWnd;
	std::fill(m_eKeyStates.begin(), m_eKeyStates.end(), KEY_STATE::NONE);
}

void InputManager::Update()
{
	UpdateKeyboard();
	UpdateMouse();
}

bool InputManager::GetButtonDown(UCHAR key)
{
	return GetState(key) == KEY_STATE::DOWN;
}

bool InputManager::GetButtonPressed(UCHAR key)
{
	return GetState(key) == KEY_STATE::PRESS;
}

bool InputManager::GetButtonUp(UCHAR key)
{
	return GetState(key) == KEY_STATE::UP;
}

void InputManager::UpdateKeyboard()
{
	HWND hWnd = ::GetActiveWindow();
	if (m_hWnd != hWnd) {
		std::fill(m_eKeyStates.begin(), m_eKeyStates.end(), KEY_STATE::NONE);
		return;
	}

	std::array<BYTE, KEY_TYPE_COUNT> KeyBuffer;
	if (::GetKeyboardState(KeyBuffer.data()) == FALSE) {
		std::fill(m_eKeyStates.begin(), m_eKeyStates.end(), KEY_STATE::NONE);
		return;
	}

	for (int key = 0; key < KEY_TYPE_COUNT; ++key) {
		if (KeyBuffer[key] & 0xF0) {
			KEY_STATE& state = m_eKeyStates[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN) {
				state = KEY_STATE::PRESS;
			}
			else {
				state = KEY_STATE::DOWN;
			}

		}
		else {
			KEY_STATE& state = m_eKeyStates[key];

			if (state == KEY_STATE::PRESS || state == KEY_STATE::DOWN) {
				state = KEY_STATE::UP;
			}
			else {
				state = KEY_STATE::NONE;
			}
		}
	}
}

void InputManager::UpdateMouse()
{
	m_ptOldCursorPos = m_ptCurrentCursorPos;
	::GetCursorPos(&m_ptCurrentCursorPos);
	::ScreenToClient(m_hWnd, &m_ptCurrentCursorPos);
}
