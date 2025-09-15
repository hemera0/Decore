#include "Camera.hpp"
#include "../Input/InputSystem.hpp"
#include "../Time/TimeSystem.hpp"
#include <gtc/quaternion.hpp>

namespace Engine::Core {
	Camera::Camera(CameraType type) : m_Type(type) {}

	void Camera::UpdateMouseControls(float xpos, float ypos) {
		float x = static_cast<float>(xpos);
		float y = static_cast<float>(ypos);

		static float oldX = x;
		static float oldY = y;

		float dx = oldX - x;
		float dy = oldY - y;

		if (m_Type != CameraType::LookAt) {
			const float Sensitivity = 0.01f;

			m_Yaw += dx * Sensitivity;
			m_Pitch += dy * Sensitivity;

			m_Pitch = glm::clamp( m_Pitch, glm::radians( -89.9f ), glm::radians( 89.9f ) );

			UpdateCameraVectors();

			oldX = x;
			oldY = y;
		}
	}

	void Camera::UpdateCameraVectors() {
		if (m_Type == CameraType::Observer || m_Type == CameraType::FlyCam) {
			glm::vec3 front;
			front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
			front.y = sin(glm::radians(m_Pitch));
			front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

			m_Front = glm::normalize(front);

			m_Right = glm::normalize(glm::cross(front, glm::vec3{ 0.f, 1.f, 0.f }));
			m_Up = glm::normalize(glm::cross(m_Right, front));
		}

		/*
		if (m_Type == CameraType::Attachment) {
			const float radius = 4.f;
			m_Position.x = m_Attachment.x - radius * glm::cos(glm::radians(m_Yaw));
			m_Position.y = 5.f;//m_Attachment.y + radius * glm::sin(glm::radians(m_Pitch));
			m_Position.z = m_Attachment.z - radius * glm::sin(glm::radians(m_Yaw));
		}
		*/
	}

	void Camera::UpdateMovementControls(float dt, float forwardMove, float sideMove) {
		if (m_Type == CameraType::LookAt || m_Type == CameraType::Attachment)
			return;

		float velocity = m_Speed * dt;
		m_Position += ((forwardMove * m_Front) + (sideMove * m_Right)) * velocity;
	}

	void Camera::Update() {
		if (m_Locked)
			return;

		float forwardMove{}, sideMove{};
		if (InputSystem::GetKeyHeld('W'))
			forwardMove = 1.f;
		if (InputSystem::GetKeyHeld('S'))
			forwardMove = -1.f;
		if (InputSystem::GetKeyHeld('A'))
			sideMove = 1.f;
		if (InputSystem::GetKeyHeld('D'))
			sideMove = -1.f;

		const auto& mousePos = InputSystem::GetMousePos();
		UpdateMouseControls(mousePos.x, mousePos.y);
		UpdateMovementControls(TimeSystem::GetDeltaTime(), forwardMove, sideMove);
	}

	glm::mat4 Camera::GetViewMatrix( ) const
	{
		if ( m_Type == CameraType::FlyCam || m_Type == CameraType::Observer )
			return glm::lookAtLH( m_Position, m_Position + m_Front, m_Up );
		if ( m_Type == CameraType::Attachment )
		{
			glm::mat4 T_back = glm::translate( glm::mat4( 1.0f ), glm::vec3( 0.0f, 0.0f, -m_AttachmentOffset ) );
			glm::mat4 R_pitch = glm::rotate( glm::mat4( 1.0f ), -m_Pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
			glm::mat4 R_yaw = glm::rotate( glm::mat4( 1.0f ), -m_Yaw, glm::vec3( 0.0f, 1.0f, 0.0f ) );
			glm::mat4 T_target = glm::translate( glm::mat4( 1.0f ), m_Attachment );

			glm::mat4 worldTransform = T_target * R_yaw * R_pitch * T_back;
			return glm::inverse( worldTransform );
		}

		return glm::mat4( 1.f );
	}

	void Camera::SetPerspective(float fovDegrees, float aspect, float zNear, float zFar) {
		m_Near = zNear;
		m_Far = zFar;

		m_ProjectionMatrix = glm::perspectiveLH(glm::radians(fovDegrees), aspect, zNear, zFar);
		m_ProjectionMatrix[1][1] *= -1;
	}

	glm::mat4 Camera::GetProjectionMatrix() const {
		return m_ProjectionMatrix;
	}

	std::array<glm::vec4, 8> Camera::CalculateFrustumCorners() const {
		std::array<glm::vec4, 8> corners = {
				glm::vec4(-1.0f,  1.0f, 0.0f, 1.f),
				glm::vec4(1.0f,  1.0f, 0.0f, 1.f),
				glm::vec4(1.0f, -1.0f, 0.0f, 1.f),
				glm::vec4(-1.0f, -1.0f, 0.0f, 1.f),
				glm::vec4(-1.0f,  1.0f,  1.0f, 1.f),
				glm::vec4(1.0f,  1.0f,  1.0f, 1.f),
				glm::vec4(1.0f, -1.0f,  1.0f, 1.f),
				glm::vec4(-1.0f, -1.0f,  1.0f, 1.f),
		};

		const glm::mat4& viewMatrix = GetViewMatrix();
		const glm::mat4& projectionMatrix = GetProjectionMatrix();

		const glm::mat4& invViewProjMatrix = glm::inverse(projectionMatrix * viewMatrix);

		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec4 invCorner = invViewProjMatrix * corners[i];
			corners[i] = invCorner / invCorner.w;
		}

		return corners;
	}

	glm::vec4 Camera::CalculateFrustumCenter() const {
		const auto& corners = CalculateFrustumCorners();

		glm::vec4 center = glm::vec4(0.0f);
		for (uint32_t i = 0; i < corners.size(); i++) {
			center += corners[i];
		}

		return center / 8.0f;
	}
}