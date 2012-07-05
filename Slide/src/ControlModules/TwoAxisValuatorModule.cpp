#include "../Globals.h"

TwoAxisValuatorModule::TwoAxisValuatorModule(Slide & Slide)
	: ControlModule(1, 3, 0),
	  m_Slide(Slide)
{
}

TwoAxisValuatorModule::~TwoAxisValuatorModule()
{
}

bool TwoAxisValuatorModule::ShouldActivate() const
{
	return ControlModule::ShouldActivate() && (0 != m_Slide.GetSelectedObjectId());
}

void TwoAxisValuatorModule::ProcessActivation(ControlModule * Previous)
{
}

void TwoAxisValuatorModule::ProcessDeactivation(ControlModule * Next)
{
	// Snap to nearest rotation
	m_Slide.RotationalSnap();
}

void TwoAxisValuatorModule::ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount)
{
	if (IsButtonPressed(0))
	{
		if (0 != m_Slide.GetSelectedObjectId())
		{
			Wm5::Vector3d ToObject = m_Slide.GetSelectedObject().GetPosition() - Wm5::Vector3d(camera.x, camera.y, camera.z); ToObject.Normalize();

			Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);
			Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
			Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
			Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);

			Horizontal = Vertical.Cross(ToObject);
			Vertical = ToObject.Cross(Horizontal);

			if (0 == SliderId)
			{
				Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Vertical, -0.008 * MovedAmount);
				m_Slide.GetSelectedObject().ModifyRotation() = RotateBy * m_Slide.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
				m_Slide.GetSelectedObject().ModifyRotation().Normalize();
			}
			else if (1 == SliderId)
			{
				Wm5::Quaterniond RotateBy = Wm5::Quaterniond(Horizontal, -0.008 * MovedAmount);
				m_Slide.GetSelectedObject().ModifyRotation() = RotateBy * m_Slide.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
				m_Slide.GetSelectedObject().ModifyRotation().Normalize();
			}
			else if (2 == SliderId)
			{
				Wm5::Vector3d ToObject = m_Slide.GetSelectedObject().GetPosition() - Wm5::Vector3d(camera.x, camera.y, camera.z); ToObject.Normalize();
				Wm5::Quaterniond RotateBy = Wm5::Quaterniond(ToObject, -0.10 * MovedAmount);
				m_Slide.GetSelectedObject().ModifyRotation() = RotateBy * m_Slide.GetSelectedObject().ModifyRotation();		// RotateBy first gives us rotations around global axes; RotateBy second gives us rotations around local axes
				m_Slide.GetSelectedObject().ModifyRotation().Normalize();
			}

			m_Slide.m_ModelRotatedByUser = true;
		}
	}
}
