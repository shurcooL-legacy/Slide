#include "../Globals.h"

InputManager * g_InputManager = nullptr;

InputManager * InputManager::m_pInstance = nullptr;

InputManager::InputManager()
	: m_Listeners(),
//	  m_MouseCursorPosition(),
	  m_MouseCursorVisible(true),
	  m_MouseIgnorePositionOnce(true),
	  m_MouseIgnorePositionAlways(false),
	  m_WindowDimensions()
{
	assert(nullptr == m_pInstance);		// m_pInstance wasn't == nullptr, means we have more than 1 instance of InputManager, not right
	m_pInstance = this;

	SetGlfwCallbacks();
}

InputManager::~InputManager()
{
	/*while (!m_Listeners.empty()) {
		delete *m_Listeners.rbegin();
		m_Listeners.pop_back();
	}*/

	RemoveGlfwCallbacks();

	SetMouseCursorVisibility(true);

	m_pInstance = nullptr;
}

void InputManager::RegisterListener(InputListener * pListener)
{
	m_Listeners.push_back(pListener);
}

void InputManager::UnregisterListener(InputListener * pListener)
{
	for (std::vector<InputListener *>::iterator it0 = m_Listeners.begin(); it0 != m_Listeners.end(); ++it0)
	{
		if ((*it0) == pListener)
		{
			m_Listeners.erase(it0);
			break;
		}
	}
}

/*const Vector2n InputManager::GetMouseCursorPosition() const
{
	return m_MouseCursorPosition;
}*/

void InputManager::SetMouseCursorVisibility(bool Visible)
{
	static int MouseCursorDesktopPositionX, MouseCursorDesktopPositionY;

	// Hide Mouse Cursor
	if (!Visible && m_MouseCursorVisible)
	{
		glfwPollEvents();
		m_MouseIgnorePositionAlways = true;
		glfwGetMousePos(&MouseCursorDesktopPositionX, &MouseCursorDesktopPositionY);
		glfwDisable(GLFW_MOUSE_CURSOR);
		m_MouseIgnorePositionOnce = true;
		m_MouseIgnorePositionAlways = false;

		m_MouseCursorVisible = false;
	}
	// Show Mouse Cursor
	else if (Visible && !m_MouseCursorVisible)
	{
		m_MouseIgnorePositionAlways = true;
		glfwEnable(GLFW_MOUSE_CURSOR);
		glfwSetMousePos(MouseCursorDesktopPositionX, MouseCursorDesktopPositionY);
		m_MouseIgnorePositionOnce = true;
		m_MouseIgnorePositionAlways = false;

		m_MouseCursorVisible = true;

		// Create a mouse position event
		//ProcessMousePos(m_MousePositionX, m_MousePositionY);
	}
}

bool InputManager::IsMouseCursorVisible()
{
	return m_MouseCursorVisible;
}

void InputManager::UpdateWindowDimensions(Vector2n WindowDimensions)
{
	m_WindowDimensions = WindowDimensions;
}

void InputManager::SetGlfwCallbacks()
{
	glfwSetKeyCallback(&InputManager::ProcessKey);
	glfwSetCharCallback(&InputManager::ProcessChar);
	glfwSetMouseButtonCallback(&InputManager::ProcessMouseButton);
	glfwSetMousePosCallback(&InputManager::ProcessMousePos);
	glfwSetMouseWheelCallback(&InputManager::ProcessMouseWheel);
#ifdef _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
	glfwSetTouchButtonCallback(&InputManager::ProcessTouchButton);
	glfwSetTouchPosCallback(&InputManager::ProcessTouchPos);
#endif // _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
}

void InputManager::RemoveGlfwCallbacks()
{
	glfwSetKeyCallback(nullptr);
	glfwSetCharCallback(nullptr);
	glfwSetMouseButtonCallback(nullptr);
	glfwSetMousePosCallback(nullptr);
	glfwSetMouseWheelCallback(nullptr);
#ifdef _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
	glfwSetTouchButtonCallback(nullptr);
	glfwSetTouchPosCallback(nullptr);
#endif // _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
}

void GLFWCALL InputManager::ProcessKey(int Key, int Action)
{
	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessButton(InputId(0 /* Keyboard */, Key), (GLFW_PRESS == Action));
	}
}

void GLFWCALL InputManager::ProcessChar(int Character, int Action)
{
	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessCharacter(Character, (GLFW_PRESS == Action));
	}
}

void GLFWCALL InputManager::ProcessMouseButton(int MouseButton, int Action)
{
	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessButton(InputId(1000 /* Mouse */, MouseButton), (GLFW_PRESS == Action));
	}
}

void GLFWCALL InputManager::ProcessMousePos(int MousePositionX, int MousePositionY)
{
	MousePositionY = m_pInstance->m_WindowDimensions.Y() - 1 - MousePositionY;

#if 0
	static bool PreviousMousePositionSet = false;

	if (true == m_pInstance->IsMouseCursorVisible())
	{
		PreviousMousePositionSet = false;

		//m_pInstance->m_MousePositionX = MousePositionX;
		//m_pInstance->m_MousePositionY = MousePositionY;

		for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
		{
			//(*it0)->ProcessMousePosition(MousePositionX, MousePositionY);
			(*it0)->Process2Axes(InputId(1000 /* Mouse */, 0), MousePositionX, MousePositionY);
		}

		//printf("       pointer moved to = (%d, %d)\n", nMousePosX, nMousePosY);
	}
	else
	{
		static int		PreviousMousePositionX = MousePositionX;
		static int		PreviousMousePositionY = MousePositionY;

		if (false == PreviousMousePositionSet)
		{
			//printf("  initial mouse pos = (%d, %d)\n", nMousePosX, nMousePosY);
			PreviousMousePositionX = MousePositionX;
			PreviousMousePositionY = MousePositionY;
			PreviousMousePositionSet = true;
		}
		else
		{
			int MouseMovedX = MousePositionX - PreviousMousePositionX;
			int MouseMovedY = MousePositionY - PreviousMousePositionY;
			PreviousMousePositionX = MousePositionX;
			PreviousMousePositionY = MousePositionY;

			for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
			{
				(*it0)->ProcessSlider(InputId(1000 /* Mouse */, 0 /* Mouse X Axis */), static_cast<double>(MouseMovedX));
				(*it0)->ProcessSlider(InputId(1000 /* Mouse */, 1 /* Mouse Y Axis */), static_cast<double>(MouseMovedY));
			}
		}
	}
#else
	static int PreviousMousePositionX = MousePositionX;
	static int PreviousMousePositionY = MousePositionY;

	int MouseMovedX = MousePositionX - PreviousMousePositionX;
	int MouseMovedY = MousePositionY - PreviousMousePositionY;
	PreviousMousePositionX = MousePositionX;
	PreviousMousePositionY = MousePositionY;

	if (m_pInstance->m_MouseIgnorePositionAlways)
	{
		//printf("mm - %d,%d (Always)\n", MouseMovedX, MouseMovedY);
		return;
	}

	if (m_pInstance->m_MouseIgnorePositionOnce)
	{
		//printf("Mouse NOT moved by %d, %d to new pos %d, %d\n", MouseMovedX, MouseMovedY, MousePositionX, MousePositionY);
		m_pInstance->m_MouseIgnorePositionOnce = false;
		return;
	}

	if (true == m_pInstance->IsMouseCursorVisible())
	{
		//m_pInstance->m_MouseCursorPosition.X() = MousePositionX;
		//m_pInstance->m_MouseCursorPosition.Y() = MousePositionY;

		for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
		{
			//(*it0)->ProcessMousePosition(MousePositionX, MousePositionY);
			InputManager::AxisState AxisStates[2] = { InputManager::AxisState(MousePositionX, m_pInstance->m_WindowDimensions.X()), InputManager::AxisState(MousePositionY, m_pInstance->m_WindowDimensions.Y()) };
			(*it0)->Process2Axes(InputId(1000 /* Mouse */, 0), AxisStates);
		}
	}

	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessSlider(InputId(1000 /* Mouse */, 0 /* Mouse X Axis */), static_cast<double>(MouseMovedX));
		(*it0)->ProcessSlider(InputId(1000 /* Mouse */, 1 /* Mouse Y Axis */), static_cast<double>(MouseMovedY));
	}
#endif
}

void GLFWCALL InputManager::ProcessMouseWheel(int MouseWheelPosition)
{
	static int		PreviousMouseWheelPosition = MouseWheelPosition;

	int MouseWheelMoved = MouseWheelPosition - PreviousMouseWheelPosition;
	PreviousMouseWheelPosition = MouseWheelPosition;

	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessSlider(InputId(1000 /* Mouse */, 2 /* Mouse Wheel */), MouseWheelMoved);
		(*it0)->ProcessAxis(InputId(1000 /* Mouse */, 2 /* Mouse Wheel */), InputManager::AxisState(MouseWheelPosition, 0));
	}
}

#ifdef _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
void GLFWCALL InputManager::ProcessTouchButton(int TouchButton, int Action)
{
	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		(*it0)->ProcessButton(InputId(2000 /* Touch */, TouchButton), (GLFW_PRESS == Action));
	}
}

void GLFWCALL InputManager::ProcessTouchPos(int TouchPositionX, int TouchPositionY, int Action)
{
	TouchPositionY = m_pInstance->m_WindowDimensions.Y() - 1 - TouchPositionY;

	static int PreviousTouchPositionX;
	static int PreviousTouchPositionY;

	int TouchMovedX = TouchPositionX - PreviousTouchPositionX;
	int TouchMovedY = TouchPositionY - PreviousTouchPositionY;
	PreviousTouchPositionX = TouchPositionX;
	PreviousTouchPositionY = TouchPositionY;

	for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
	{
		InputManager::AxisState AxisStates[2] = { InputManager::AxisState(TouchPositionX, m_pInstance->m_WindowDimensions.X()), InputManager::AxisState(TouchPositionY, m_pInstance->m_WindowDimensions.Y()) };
		(*it0)->Process2Axes(InputId(2000 /* Touch */, 0), AxisStates);
	}

	if (0 == Action) {
		for (std::vector<InputListener *>::reverse_iterator it0 = m_pInstance->m_Listeners.rbegin(); it0 != m_pInstance->m_Listeners.rend(); ++it0)
		{
			(*it0)->ProcessSlider(InputId(2000 /* Touch */, 0 /* Touch X Axis */), static_cast<double>(TouchMovedX));
			(*it0)->ProcessSlider(InputId(2000 /* Touch */, 1 /* Touch Y Axis */), static_cast<double>(TouchMovedY));
		}
	}
}
#endif // _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
