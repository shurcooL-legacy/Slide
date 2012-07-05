#include "../Globals.h"

UnderCursorTranslationModule::UnderCursorTranslationModule(Slide & Slide)
	: ControlModule(2, 1, 2),
	  m_TranslationAllowed(false),
	  m_Slide(Slide)
{
}

UnderCursorTranslationModule::~UnderCursorTranslationModule()
{
}

bool UnderCursorTranslationModule::ShouldActivate() const
{
	// Only the first button activates the module internally, the second one is just a modifier key
	return IsButtonPressed(0);
}

void UnderCursorTranslationModule::ProcessActivation(ControlModule * Previous)
{
}

void UnderCursorTranslationModule::ProcessDeactivation(ControlModule * Next)
{
	m_TranslationAllowed = false;
}

void UnderCursorTranslationModule::ModuleProcessButton(InputManager::VirtualInputId ButtonId, bool Pressed)
{
	if (0 == ButtonId && Pressed)
	{
		m_Slide.PickModelAndComputeUnderCursorPositionAndRayDirection(static_cast<int16>(GetAxisState(0).GetPosition()), static_cast<int16>(GetAxisState(1).GetPosition()));
		m_TranslationAllowed = true;
	}
}

void UnderCursorTranslationModule::PlayDepthPopSound(bool DepthPopSuccessful)
{
#ifdef WIN32
	if (DepthPopSuccessful)
		SoundEngine->play2D("../Sounds/click.wav");
	else
		SoundEngine->play2D("../Sounds/beep.wav");
#endif
}

void UnderCursorTranslationModule::ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount)
{
	if (IsButtonPressed(0) && m_TranslationAllowed)
	{
		if (0 != m_Slide.GetSelectedObjectId())
		{
			if (!IsButtonPressed(1) ^ GetDebugSwitch(DebugSwitch::DEBUG_SWITCH_SLIDE_RESOLUTION_OFF))
			{
				if (0 < MovedAmount)
				{
					//PlayDepthPopSound(Wm5::Vector3d::ZERO != m_Slide.SnapOrtho(Slide::Back, Slide::NonColliding, 2));
					PlayDepthPopSound(m_Slide.NewSnapC(Slide::BackNextLevel));
				}
				else if (0 > MovedAmount)
				{
					//PlayDepthPopSound(Wm5::Vector3d::ZERO != m_Slide.SnapOrtho(Slide::Front, Slide::NonColliding, 1));
					PlayDepthPopSound(m_Slide.NewSnapB(Slide::Front, Slide::NonColliding));
				}
			}
			else
			{
				m_Slide.MoveSelectedObject(m_Slide.ZoomSelectedModel(0.50 * MovedAmount, m_Slide.m_CursorRayDirection));
			}
		}
	}
}

void UnderCursorTranslationModule::ModuleProcess2Axes(InputManager::VirtualInputId FirstAxisId, InputManager::AxisState AxisState[2])
{
	if (IsButtonPressed(0) && m_TranslationAllowed)
	{
		// Find the new cursor ray
		m_Slide.ComputeCursorRayDirection(static_cast<int16>(AxisState[0].GetPosition()), static_cast<int16>(AxisState[1].GetPosition()));

		Wm5::Vector3d oDisplacement(Wm5::Vector3d::ZERO);

		if (0 != m_Slide.GetSelectedObjectId())
		{
			while (true)
			{
				Wm5::Vector3d Camera(camera.x, camera.y, camera.z);
				Wm5::Line3d Line(Camera, m_Slide.m_CursorRayDirection);

				Wm5::Vector3d Normal;
				if (Wm5::Vector3d::ZERO == m_Slide.m_SlidingConstraint.Normal)
				{
					Wm5::Vector3d Horizontal(Wm5::Vector3d::ZERO), Vertical(Wm5::Vector3d::ZERO);

					Horizontal.X() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					Horizontal.Y() += -Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
					Vertical.X() += Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					Vertical.Y() += Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD) * Wm5::Mathd::Sin(camera.rv * Wm5::Mathd::DEG_TO_RAD);
					Vertical.Z() += -Wm5::Mathd::Cos(camera.rv * Wm5::Mathd::DEG_TO_RAD);

					Normal = Horizontal.Cross(Vertical);
				}
				else
				{
					Normal = m_Slide.m_SlidingConstraint.Normal;
				}
				Wm5::Plane3d MovementPlane(Normal, m_Slide.m_UnderCursorPosition);

				Wm5::Vector3d IntersectionPoint;
				{
					Wm5::IntrLine3Plane3d Intersection(Line, MovementPlane);
					if (Intersection.Find())
						IntersectionPoint = Intersection.GetLine().Origin + Intersection.GetLine().Direction * Intersection.GetLineParameter();
					else
						printf("Error: No intersection!\n");
				}

				oDisplacement = IntersectionPoint - m_Slide.m_UnderCursorPosition;
				//m_UnderCursorPosition = Intersection;

				if (Wm5::Vector3d::ZERO != m_Slide.m_SlidingConstraint.Normal)
				{
					// If the primary constraint is not valid at the new position, remove the constraint and redo the displacement calculation
					auto Valid = !m_Slide.CheckForCollisionTemporary(oDisplacement) && m_Slide.CheckForCollisionTemporary(oDisplacement + m_Slide.m_SlidingConstraint.Normal * -2 * m_Slide.m_kBufferDistance);
					if (!Valid)
					{
						m_Slide.m_SlidingConstraint.Normal = Wm5::Vector3d::ZERO;
						continue;
					}
				}

				break;
			}

			// Move the Selected Object(s)
			m_Slide.MoveSelectedObject(oDisplacement);

			if (oDisplacement != Wm5::Vector3d::ZERO)
			{
				m_Slide.m_ModelMovedByUser = true;
				//printf("m_SecondConstraint.Normal.Length() == %f, %d == m_StickToPlaneModePossible\n", m_SecondConstraint.Normal.Length(), m_StickToPlaneModePossible);
			}
		}
	}
}
