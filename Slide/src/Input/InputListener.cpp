#include "../Globals.h"

InputListener::InputListener()
{
}

InputListener::~InputListener()
{
	if (nullptr != g_InputManager)
		g_InputManager->UnregisterListener(this);
}
