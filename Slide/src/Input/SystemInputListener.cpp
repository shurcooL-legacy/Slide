#include "../Globals.h"

SystemInputListener::SystemInputListener()
	: InputListener()
{
}

SystemInputListener::~SystemInputListener()
{
}

void SystemInputListener::ProcessButton(InputManager::InputId ButtonId, bool Pressed)
{
	if (Pressed)
	{
		if (0 == ButtonId.Device)
		{
			switch (ButtonId.Id) {
			case GLFW_KEY_ESC:
				KeepRunning = false;
				break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case '0':
				Scene::InitializeScene(ButtonId.Id - '1' + 1 + ((glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT)) ? 10 : 0));
				break;
			case GLFW_KEY_F1: case GLFW_KEY_F2: case GLFW_KEY_F3: case GLFW_KEY_F4: case GLFW_KEY_F5: case GLFW_KEY_F6: case GLFW_KEY_F7: case GLFW_KEY_F8: case GLFW_KEY_F9: case GLFW_KEY_F10: case GLFW_KEY_F11: case GLFW_KEY_F12:
				m_DebugSwitches[ButtonId.Id - GLFW_KEY_F1] = !m_DebugSwitches[ButtonId.Id - GLFW_KEY_F1];
				break;
			case 'B':
				bTESTMode0Performed = true;
				break;
			case 'F':
				bTESTMode1Performed = true;
				break;
			case 'G':
				bTESTMode2Performed = true;
				break;
			case GLFW_KEY_KP_ADD:
				++m_DebugValues[0];
				printf("NumberDepthLayersToPeel = %d\n", m_DebugValues[0]);
				break;
			case GLFW_KEY_KP_SUBTRACT:
				if (m_DebugValues[0] > 0) {
					--m_DebugValues[0];
					printf("NumberDepthLayersToPeel = %d\n", m_DebugValues[0]);
				}
				break;
			case 'R':
				MySlideTools->ReloadShaders();
				break;
			case 'P':
				printf("--->>>>>>>>>>---\n");
				printf("std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin();\n");
				for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin() + 1; it0 != MyScene.m_Objects.end(); ++it0) {
					printf("++it0; (*it0)->ModifyPosition().X() = %f; (*it0)->ModifyPosition().Y() = %f; (*it0)->ModifyPosition().Z() = %f; (*it0)->ModifyRotation().W() = %f; (*it0)->ModifyRotation().X() = %f; (*it0)->ModifyRotation().Y() = %f; (*it0)->ModifyRotation().Z() = %f;\n",
						(*it0)->GetPosition().X(), (*it0)->GetPosition().Y(), (*it0)->GetPosition().Z(), (*it0)->ModifyRotation().W(), (*it0)->ModifyRotation().X(), (*it0)->ModifyRotation().Y(), (*it0)->ModifyRotation().Z());
				}
				printf("\n");
				printf("camera.x = %f; camera.y = %f; camera.z = %f; camera.rh = %f; camera.rv = %f;\n", camera.x, camera.y, camera.z, camera.rh, camera.rv);
				printf("---<<<<<<<<<<---\n");
				break;
			case 'O':
				printf("--->>>>>>>>>>---\n");
				for (std::vector<SceneObject *>::iterator it0 = MyScene.m_Objects.begin(); it0 != MyScene.m_Objects.end(); ++it0) {
					printf("Object %d:\n", (*it0)->GetId());
					printf("  m_ContainedObjects: { ");
						for (std::set<uint16>::iterator it1 = (*it0)->m_ContainedObjects.begin(); it1 != (*it0)->m_ContainedObjects.end(); ++it1) {
							printf("%d, ", *it1);
						}
						printf("}\n");
					printf("  m_ContainedByObject: %d\n", (*it0)->m_ContainedByObject);
				}
				printf("---<<<<<<<<<<---\n");
				break;
			default:
				break;
			}
		}
	}
}
