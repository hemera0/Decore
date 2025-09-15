#pragma once
#include <array>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/rotate_vector.hpp>

namespace Engine::Core {
	enum class CameraType {
		Observer = 0, // FPS Where Y-axis movement is unaffected
		FlyCam,
		LookAt,
		Attachment // 3rd Person Camera.
	};

	class Camera {
	public:
		Camera() = default;
		Camera(CameraType cameraType);

		void UpdateMouseControls(float xpos, float ypos);
		void UpdateMovementControls(float dt, float forwardMove, float sideMove);

		void Update();

		void SetLockRotation(bool lock) { m_Locked = lock; }

		float GetNearClip() const { return m_Near; }
		float GetFarClip() const { return m_Far; }

		void SetPerspective(float fovDegrees, float aspect, float zNear, float zFar);
		void SetCameraType(CameraType type) { m_Type = type; }

		void SetAttachmentOffset( const float offset ) { m_AttachmentOffset = offset; }
		void SetAttachment(const glm::vec3& target) { m_Attachment = target; }
		void SetPosition(const glm::vec3& position) { m_Position = position; }
		void SetAngles(float yaw, float pitch) {
			m_Yaw = glm::radians(yaw);
			m_Pitch = glm::radians(pitch);
			UpdateCameraVectors();
		}

		glm::vec3 GetForward() const { return m_Front; }
		glm::vec3 GetRight() const { return m_Right; }
		glm::vec3 GetPosition() const { return m_Position; }
		glm::vec2 GetAngles() const { return { m_Yaw, m_Pitch }; }

		constexpr static float distanceFromTarget = -5.f;

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;

		std::array<glm::vec4, 8> CalculateFrustumCorners() const;
		glm::vec4 CalculateFrustumCenter() const;
	private:
		void UpdateCameraVectors();
		
		CameraType m_Type{};

		float m_Speed{ 10.f };
		float m_Near{}, m_Far{};

		glm::vec3 m_Front{}, m_Right{}, m_Up{ 0.f, 1.f, 0.f };
		glm::vec3 m_Position{};
		glm::vec3 m_Attachment{};

		glm::mat4 m_ProjectionMatrix{};

		float m_AttachmentOffset{ 4.f };
		float m_Yaw{}, m_Pitch{};
		bool m_Locked{};
	};
}