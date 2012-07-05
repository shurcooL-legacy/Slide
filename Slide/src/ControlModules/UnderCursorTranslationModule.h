#pragma once
#ifndef __UnderCursorTranslationModule_H__
#define __UnderCursorTranslationModule_H__

class UnderCursorTranslationModule : public ControlModule
{
public:
	UnderCursorTranslationModule(Slide & Slide);
	virtual ~UnderCursorTranslationModule();

	virtual bool ShouldActivate() const;

	virtual void ProcessActivation(ControlModule * Previous);
	virtual void ProcessDeactivation(ControlModule * Next);

protected:
	virtual void ModuleProcessButton(InputManager::VirtualInputId ButtonId, bool Pressed);
	virtual void ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount);
	virtual void ModuleProcess2Axes(InputManager::VirtualInputId FirstAxisId, InputManager::AxisState AxisState[2]);

private:
	UnderCursorTranslationModule(const UnderCursorTranslationModule &);
	UnderCursorTranslationModule & operator =(const UnderCursorTranslationModule &);

	void PlayDepthPopSound(bool DepthPopSuccessful);

	bool			m_TranslationAllowed;

	Slide &			m_Slide;
};

#endif // __UnderCursorTranslationModule_H__
