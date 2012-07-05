#pragma once
#ifndef __TwoAxisValuatorModule_H__
#define __TwoAxisValuatorModule_H__

class TwoAxisValuatorModule : public ControlModule
{
public:
	TwoAxisValuatorModule(Slide & Slide);
	virtual ~TwoAxisValuatorModule();

	virtual bool ShouldActivate() const;

	virtual bool ShouldMouseCursorVisible() { return false; }

	virtual void ProcessActivation(ControlModule * Previous);
	virtual void ProcessDeactivation(ControlModule * Next);

protected:
	virtual void ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount);

private:
	TwoAxisValuatorModule(const TwoAxisValuatorModule &);
	TwoAxisValuatorModule & operator =(const TwoAxisValuatorModule &);

	Slide &			m_Slide;
};

#endif // __TwoAxisValuatorModule_H__
