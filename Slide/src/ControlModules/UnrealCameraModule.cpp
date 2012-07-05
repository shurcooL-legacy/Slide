#include "../Globals.h"

UnrealCameraModule::UnrealCameraModule()
	: ControlModule(2, 2, 0)
{
}

UnrealCameraModule::~UnrealCameraModule()
{
}

void UnrealCameraModule::ProcessActivation(ControlModule * Previous)
{
}

void UnrealCameraModule::ProcessDeactivation(ControlModule * Next)
{
}

void UnrealCameraModule::ModuleProcessSlider(InputManager::VirtualInputId SliderId, double MovedAmount)
{
	if (IsButtonPressed(0) && !IsButtonPressed(1)) {
		if (1 == SliderId) camera.x += 0.012 * MovedAmount * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
		if (1 == SliderId) camera.y += 0.012 * MovedAmount * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
		if (0 == SliderId) camera.rh += 0.15 * MovedAmount;
	} else if (IsButtonPressed(0) && IsButtonPressed(1)) {
		if (0 == SliderId) camera.x += 0.012 * MovedAmount * Wm5::Mathd::Cos(camera.rh * Wm5::Mathd::DEG_TO_RAD);
		if (0 == SliderId) camera.y += -0.012 * MovedAmount * Wm5::Mathd::Sin(camera.rh * Wm5::Mathd::DEG_TO_RAD);
		if (1 == SliderId) camera.z += 0.012 * MovedAmount;
	} else if (!IsButtonPressed(0) && IsButtonPressed(1)) {
		if (0 == SliderId) camera.rh += 0.15 * MovedAmount;
		if (1 == SliderId) camera.rv += 0.15 * MovedAmount;
	}
	while (camera.rh < 0) camera.rh += 360;
	while (camera.rh >= 360) camera.rh -= 360;
	if (camera.rv > 90) camera.rv = 90;
	if (camera.rv < -90) camera.rv = -90;
	//printf("Cam rot h = %f, v = %f\n", camera.rh, camera.rv);
}
