#include "InputSystem.hpp"

namespace Engine::Core {
	std::unordered_map<int, int> InputSystem::KeyStates{};
	std::unordered_map<int, int> InputSystem::OldKeyStates{};

	MousePos InputSystem::MousePosition;
}