#pragma once
#ifndef __InputManager_H__
#define __InputManager_H__

class InputManager
{
public:
	InputManager();
	~InputManager();

	void RegisterListener(InputListener * pListener);
	void UnregisterListener(InputListener * pListener);

	void SetMouseCursorVisibility(bool Visible);
	bool IsMouseCursorVisible();

	//const Vector2n GetMouseCursorPosition() const;		// TODO: Replace with a more proper list of "cursors"

	void UpdateWindowDimensions(Vector2n WindowDimensions);

	static void GLFWCALL ProcessKey(int Key, int Action);
	static void GLFWCALL ProcessChar(int Character, int Action);
	static void GLFWCALL ProcessMouseButton(int MouseButton, int Action);
	static void GLFWCALL ProcessMousePos(int MousePositionX, int MousePositionY);
	static void GLFWCALL ProcessMouseWheel(int MouseWheelPosition);
#ifdef _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED
	static void GLFWCALL ProcessTouchButton(int TouchButton, int Action);
	static void GLFWCALL ProcessTouchPos(int TouchPositionX, int TouchPositionY, int Action);
#endif // _GLFW_DMITRI_WINDOWS_TOUCH_ENABLED

	struct InputId {
		uint16 Device;
		uint16 Id;

		InputId(uint16 Device, uint16 Id) {
			this->Device = Device;
			this->Id = Id;
		}

		bool operator <(const InputId & Other) const {
			uint32 ThisValue = (static_cast<uint32>(this->Device) << 16) | (this->Id);
			uint32 OtherValue = (static_cast<uint32>(Other.Device) << 16) | (Other.Id);
			return ThisValue < OtherValue;
		}
	};

	typedef uint16 VirtualInputId;

	class AxisState
	{
	public:
		AxisState(double Position, double Length)
			: m_Position(Position),
			  m_Length(Length)
		{
		}

		double GetPosition() const { return m_Position; }
		double GetLength() const { return m_Length; }

	private:
		double m_Position;
		double m_Length;
	};

private:
	InputManager(const InputManager &);
	InputManager & operator =(const InputManager &);

	void SetGlfwCallbacks();
	void RemoveGlfwCallbacks();

	std::vector<InputListener *>		m_Listeners;

	//Vector2n			m_MouseCursorPosition;		// TODO: Replace with a more proper list of "cursors"
	volatile bool		m_MouseCursorVisible;
	volatile bool		m_MouseIgnorePositionAlways;
	volatile bool		m_MouseIgnorePositionOnce;

	Vector2n			m_WindowDimensions;

	static InputManager *		m_pInstance;
};

#endif // __InputManager_H__
