#pragma once
#ifndef __ControlModule_H__
#define __ControlModule_H__

class ControlModule
	: public InputListener
{
public:
	ControlModule(uint32 Buttons, uint32 Sliders, uint32 Axes);
	virtual ~ControlModule();

	void VerifyMapping(uint32 Buttons, uint32 Sliders, uint32 Axes);

	virtual bool ShouldActivate() const;
	virtual bool ShouldDeactivate() const;

	virtual bool ShouldMouseCursorVisible() { return true; }

	virtual void ProcessActivation(ControlModule * Previous) {}
	virtual void ProcessDeactivation(ControlModule * Next) {}

	void ProcessButton(InputManager::VirtualInputId ButtonId, bool Pressed);
	void ProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount);
	void ProcessAxis(InputManager::VirtualInputId AxisId, InputManager::AxisState AxisState);
	void Process2Axes(InputManager::VirtualInputId FirstAxisId, InputManager::AxisState AxisState[2]);

	void ProcessCharacter(int Character, bool Pressed);

	void ProcessTimePassed(double TimePassed);

	void SetActiveModulePointer(ControlModule ** ActiveModulePointer) { m_ActiveModulePointer = ActiveModulePointer; }

protected:
	bool IsButtonPressed(uint32 Button) const;
	InputManager::AxisState GetAxisState(uint32 Axis) const;

	virtual void ModuleProcessButton(InputManager::VirtualInputId ButtonId, bool Pressed) {}
	virtual void ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount) {}
	virtual void ModuleProcessAxis(InputManager::VirtualInputId AxisId, InputManager::AxisState AxisState) {}
	virtual void ModuleProcess2Axes(InputManager::VirtualInputId FirstAxisId, InputManager::AxisState AxisState[2]) {}

	virtual void ModuleProcessCharacter(int Character, bool Pressed) {}

	virtual void ModuleProcessTimePassed(double TimePassed) {}

	ControlModule ** m_ActiveModulePointer;

private:
	ControlModule(const ControlModule &);
	ControlModule & operator =(const ControlModule &);

	bool IsActiveExternally() { return (this == *m_ActiveModulePointer); }

	const uint32		m_Buttons;
	const uint32		m_Sliders;
	const uint32		m_Axes;

	std::vector<bool>							m_ButtonsPressed;
	std::vector<InputManager::AxisState>		m_AxisStates;
};

#endif // __ControlModule_H__
