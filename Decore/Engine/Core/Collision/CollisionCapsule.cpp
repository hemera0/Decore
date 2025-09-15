#include "CollisionCapsule.hpp"
#include "CollisionBox.hpp"

namespace Engine::Core {
	inline glm::vec3 
		ClosestPointOnLineSegment(
			const glm::vec3& A,
			const glm::vec3& B,
			const glm::vec3& Point)
	{
		glm::vec3 AB = B - A;
		float T = glm::dot(Point - A, AB) / glm::dot(AB, AB);
		return A + glm::clamp(T, 0.f, 1.f) * AB;
	}

	bool CollisionCapsule::Intersects(const CollisionCapsule& capsule) {
		// Capsule AABB's don't intersect.
		// if (!GetAABB().Intersects(capsule.GetAABB()))
		// 	return false;

		glm::vec3 aRadius = glm::vec3(m_Radius, m_Radius, m_Radius);
		glm::vec3 aNormal = glm::normalize(m_Top - m_Bottom);

		// Capsule 1.
		glm::vec3 aLineEnd = aNormal * aRadius;
		glm::vec3 a_A = m_Bottom + aLineEnd;
		glm::vec3 a_B = m_Top - aLineEnd;

		glm::vec3 bRadius = glm::vec3(capsule.m_Radius, capsule.m_Radius, capsule.m_Radius);
		glm::vec3 bNormal = glm::normalize(capsule.m_Top - capsule.m_Bottom);

		glm::vec3 bLineEnd = bNormal * bRadius;
		glm::vec3 b_A = capsule.m_Bottom + bLineEnd;
		glm::vec3 b_B = capsule.m_Top - bLineEnd;

		// Vectors between line endpoints.
		glm::vec3 v0 = b_A - a_A;
		glm::vec3 v1 = b_B - a_A;
		glm::vec3 v2 = b_A - a_B;
		glm::vec3 v3 = b_B - a_B;

		// Get squared lengths.
		float d0 = glm::dot(v0, v0);
		float d1 = glm::dot(v1, v1);
		float d2 = glm::dot(v2, v2);
		float d3 = glm::dot(v3, v3);

		glm::vec3 bestA = a_A;
		if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1)
			bestA = a_B;

		return true;
	}

	bool CollisionCapsule::Intersects(const CollisionBox& aabb) {
		return false;
	}

	bool CollisionCapsule::Intersects(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2) {
		glm::vec3 CapsuleNormal = glm::normalize(m_Top - m_Bottom);
		glm::vec3 LineEndOffset = CapsuleNormal * m_Radius;
		glm::vec3 A = m_Bottom + LineEndOffset;
		glm::vec3 B = m_Top - LineEndOffset;

		glm::vec3 N = glm::normalize(glm::cross(p1 - p0, p2 - p0)); // plane normal

		float t = glm::dot(N, (p0 - m_Bottom) / abs(dot(N, CapsuleNormal)));
		glm::vec3 line_plane_intersection = m_Bottom + CapsuleNormal * t;

		return false;
	}

	/*
	CollisionBox CollisionCapsule::GetAABB() const {
		//glm::vec3 halfWidth = glm::vec3(m_Radius, m_Radius, m_Radius);

		//CollisionBox bottomAABB{};
		//bottomAABB.m_Center = m_Bottom;
		//bottomAABB.m_Extents = halfWidth;
		//
		//CollisionBox topAABB = { m_Top, halfWidth };

		//return CollisionBox::Merge(topAABB, bottomAABB);
	
	
	}
	*/
}