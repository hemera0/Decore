#pragma once
#include <unordered_map>

namespace Engine::Core {
	struct MousePos {
		float x{}, y{};
	};

	class InputSystem {
	public:
		static std::unordered_map<int, int> KeyStates;
		static std::unordered_map<int, int> OldKeyStates;
		static MousePos MousePosition;

		static bool GetKeyPressed(int keyName) {
			return KeyStates[keyName] == 0 && KeyStates[keyName] != -1 && KeyStates[keyName] != OldKeyStates[keyName];
		}

		static bool GetKeyHeld(int keyName) {
			return KeyStates[keyName] != 0 && KeyStates[keyName] != -1;
		}

		static void SetKeyState(int keyName, int state) {
			KeyStates[keyName] = state;
		}

		static MousePos GetMousePos() {
			return MousePosition;
		}

		static void SetMousePos(float x, float y) {
			MousePosition = { x, y };
		}
	};
}