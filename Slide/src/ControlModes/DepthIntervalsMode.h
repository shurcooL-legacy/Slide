#pragma once
#ifndef __DepthIntervalsMode_H__
#define __DepthIntervalsMode_H__

class DepthIntervalsMode
{
public:
	DepthIntervalsMode();
	virtual ~DepthIntervalsMode();

	ControlModuleMapping & GetControlModuleMapping() { return m_ControlModuleMapping; }

private:
	DepthIntervalsMode(const DepthIntervalsMode &);
	DepthIntervalsMode & operator =(const DepthIntervalsMode &);

	UnderCursorTranslationModule	m_UnderCursorTranslationModule;
	TwoAxisValuatorModule			m_TwoAxisValuatorModule;
	UnrealCameraModule				m_UnrealCameraModule;

	ControlModuleMapping			m_ControlModuleMapping;
};

#endif // __DepthIntervalsMode_H__
