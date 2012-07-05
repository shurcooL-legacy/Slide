#pragma once
#ifndef __ControlModuleMapping_H__
#define __ControlModuleMapping_H__

class ControlModuleMapping
	: public InputListener
{
public:
	ControlModuleMapping();
	~ControlModuleMapping();

	void AddMapping(ControlModule * ControlModule, sint32 Priority, std::vector<InputManager::InputId> ButtonMappings, std::vector<InputManager::InputId> SliderMappings, std::vector<InputManager::InputId> AxisMappings, std::vector<InputManager::InputId> PositiveConstraints, std::vector<InputManager::InputId> NegativeConstraints);
	void DoneAdding();
	void RemoveMapping(ControlModule * ControlModule);

	void ProcessButton(InputManager::InputId ButtonId, bool Pressed);
	void ProcessSlider(InputManager::InputId SliderId, double MovedAmount);
	void ProcessAxis(InputManager::InputId AxisId, InputManager::AxisState AxisState);
	void Process2Axes(InputManager::InputId FirstAxisId, InputManager::AxisState AxisState[2]);

	void ProcessCharacter(int Character, bool Pressed);
	
	void ProcessTimePassed(double TimePassed);

	ControlModule * GetActiveModule() { return m_ActiveModule; }

private:
	ControlModuleMapping(const ControlModuleMapping &);
	ControlModuleMapping & operator =(const ControlModuleMapping &);

	struct ControlModuleMappingEntry
	{
		ControlModuleMappingEntry(ControlModule * ControlModule, sint32 Priority, uint8 PositiveConstraintStateCount, uint8 NegativeConstraintStateCount)
			: m_ControlModule(ControlModule),
			  m_Priority(Priority),
			  m_PositiveConstraintStates(PositiveConstraintStateCount, false),
			  m_NegativeConstraintStates(NegativeConstraintStateCount, false)
		{}

		ControlModule *			m_ControlModule;
		sint32					m_Priority;
		std::vector<bool>		m_PositiveConstraintStates;
		std::vector<bool>		m_NegativeConstraintStates;
	};

	void RemoveFromMultimap(std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>> & Multimap, ControlModule * ControlModule);

	void ChangeActiveModule(ControlModule * ActiveModule);
	ControlModuleMappingEntry * GetActiveModuleEntry();
	ControlModuleMappingEntry * GetModuleEntry(ControlModule * Module);

	uint8 GetOverallConstraintState(std::vector<bool> & ConstraintStates, uint8 Default);

	//ControlModule *								m_HoverModule;
	ControlModule *								m_ActiveModule;
	std::vector<ControlModuleMappingEntry>		m_Entries;

	std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>>		m_ButtonMappings;
	std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>>		m_SliderMappings;
	std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>>		m_AxisMappings;

	std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>>		m_PositiveConstraintMappings;
	std::multimap<InputManager::InputId, std::pair<ControlModule *, uint8>>		m_NegativeConstraintMappings;
};

#endif // __ControlModuleMapping_H__
