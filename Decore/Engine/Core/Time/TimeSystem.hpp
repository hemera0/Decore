#pragma once

namespace Engine::Core {
	class TimeSystem {
		static float m_DeltaTime;
	public:
		static float GetDeltaTime() { return m_DeltaTime; }
		static void SetDeltaTime(float value) { m_DeltaTime = value; }
	};
}