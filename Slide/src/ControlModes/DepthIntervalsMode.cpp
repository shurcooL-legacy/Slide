#include "../Globals.h"

DepthIntervalsMode::DepthIntervalsMode()
	: m_UnderCursorTranslationModule(*MySlide0),
	  m_TwoAxisValuatorModule(*MySlide0),
	  m_UnrealCameraModule(),
	  m_ControlModuleMapping()
{
	{
		std::vector<InputManager::InputId> ButtonMappings; ButtonMappings.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_LEFT)); ButtonMappings.push_back(InputManager::InputId(0, GLFW_KEY_LALT)); ButtonMappings.push_back(InputManager::InputId(0, GLFW_KEY_ESC));
		std::vector<InputManager::InputId> SliderMappings; SliderMappings.push_back(InputManager::InputId(1000, 2));
		std::vector<InputManager::InputId> AxisMappings; AxisMappings.push_back(InputManager::InputId(1000, 0)); AxisMappings.push_back(InputManager::InputId(1000, 1));
		std::vector<InputManager::InputId> PositiveConstraints;
		std::vector<InputManager::InputId> NegativeConstraints; NegativeConstraints.push_back(InputManager::InputId(0, GLFW_KEY_LCTRL));// NegativeConstraints.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_RIGHT));
		m_ControlModuleMapping.AddMapping(&m_UnderCursorTranslationModule, 0, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}

	{
		std::vector<InputManager::InputId> ButtonMappings; ButtonMappings.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_RIGHT));
		std::vector<InputManager::InputId> SliderMappings; SliderMappings.push_back(InputManager::InputId(1000, 0)); SliderMappings.push_back(InputManager::InputId(1000, 1)); SliderMappings.push_back(InputManager::InputId(1000, 2));
		std::vector<InputManager::InputId> AxisMappings;
		std::vector<InputManager::InputId> PositiveConstraints;
		std::vector<InputManager::InputId> NegativeConstraints; NegativeConstraints.push_back(InputManager::InputId(0, GLFW_KEY_LCTRL));// NegativeConstraints.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_LEFT));
		m_ControlModuleMapping.AddMapping(&m_TwoAxisValuatorModule, 0, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}

#ifndef SLIDE_USE_BAMBOO
	{
		std::vector<InputManager::InputId> ButtonMappings; ButtonMappings.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_LEFT)); ButtonMappings.push_back(InputManager::InputId(1000, GLFW_MOUSE_BUTTON_RIGHT));
		std::vector<InputManager::InputId> SliderMappings; SliderMappings.push_back(InputManager::InputId(1000, 0)); SliderMappings.push_back(InputManager::InputId(1000, 1));
		std::vector<InputManager::InputId> AxisMappings;
		std::vector<InputManager::InputId> PositiveConstraints; PositiveConstraints.push_back(InputManager::InputId(0, GLFW_KEY_LCTRL));
		std::vector<InputManager::InputId> NegativeConstraints;
		m_ControlModuleMapping.AddMapping(&m_UnrealCameraModule, 1, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}
#else
	{
		std::vector<InputManager::InputId> ButtonMappings; ButtonMappings.push_back(InputManager::InputId(2000, 0)); ButtonMappings.push_back(InputManager::InputId(2000, 1));
		std::vector<InputManager::InputId> SliderMappings; SliderMappings.push_back(InputManager::InputId(2000, 0)); SliderMappings.push_back(InputManager::InputId(2000, 1));
		std::vector<InputManager::InputId> AxisMappings;
		std::vector<InputManager::InputId> PositiveConstraints;
		std::vector<InputManager::InputId> NegativeConstraints;
		m_ControlModuleMapping.AddMapping(&m_UnrealCameraModule, 1, ButtonMappings, SliderMappings, AxisMappings, PositiveConstraints, NegativeConstraints);
	}
#endif

	m_ControlModuleMapping.DoneAdding();

	g_InputManager->RegisterListener(&m_ControlModuleMapping);
}

DepthIntervalsMode::~DepthIntervalsMode()
{
}
