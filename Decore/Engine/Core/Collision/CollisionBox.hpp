#pragma once
#include <glm.hpp>

namespace Engine::Core {
	class CollisionBox {
	public:
		glm::vec3 m_Mins{};
		glm::vec3 m_Maxs{};

		CollisionBox( ) = default;

		explicit CollisionBox( const glm::vec3& mins, const glm::vec3& maxs ) : m_Mins( mins ), m_Maxs( maxs ) { }

		// AABB to AABB
		bool Intersects( const CollisionBox& aabb ) const;

		bool Intersects( const glm::vec3& mins, const glm::vec3& maxs ) const;

		glm::vec3 GetClosestPoint( const glm::vec3& target );



		inline glm::vec3 GetMins( ) const { return m_Mins; }
		inline glm::vec3 GetMaxs( ) const { return m_Maxs; }

		static CollisionBox Merge( const CollisionBox& b1, const CollisionBox& b2 );
		static CollisionBox MinkowskiDifference( const CollisionBox b1, const CollisionBox& b2 );
	};
}