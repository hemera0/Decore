#include "CollisionBox.hpp"

namespace Engine::Core
{
	bool CollisionBox::Intersects( const CollisionBox& aabb ) const
	{
		const glm::vec3& mins1 = GetMins( ), & maxs1 = GetMaxs( );
		const glm::vec3& mins2 = aabb.GetMins( ), & maxs2 = aabb.GetMaxs( );

		return ( mins1.x < maxs2.x && maxs1.x > mins2.x ) &&
			( mins1.y < maxs2.y && maxs1.y > mins2.y ) &&
			( mins1.z < maxs2.z && maxs1.z > mins2.z );
	}

	bool CollisionBox::Intersects( const glm::vec3& mins, const glm::vec3& maxs ) const
	{
		const glm::vec3& mins1 = GetMins( ), & maxs1 = GetMaxs( );

		return ( mins1.x <= maxs.x && maxs1.x >= mins.x ) &&
			( mins1.y <= maxs.y && maxs1.y >= mins.y ) &&
			( mins1.z <= maxs.z && maxs1.z >= mins.z );
	}

	glm::vec3 CollisionBox::GetClosestPoint( const glm::vec3& target )
	{
		glm::vec3 result = target;

		result.x = glm::max( result.x, m_Mins.x );
		result.x = glm::min( result.x, m_Maxs.x );

		result.y = glm::max( result.y, m_Mins.y );
		result.y = glm::min( result.y, m_Maxs.y );

		result.z = glm::max( result.z, m_Mins.z );
		result.z = glm::min( result.z, m_Maxs.z );

		return result;
	}

	CollisionBox CollisionBox::Merge( const CollisionBox& b1, const CollisionBox& b2 )
	{
		const auto& mins = glm::min( b1.GetMins( ), b2.GetMins( ) );
		const auto& maxs = glm::max( b1.GetMaxs( ), b2.GetMaxs( ) );

		return CollisionBox( mins, maxs );
	}

	CollisionBox CollisionBox::MinkowskiDifference( const CollisionBox b1, const CollisionBox& b2 )
	{
		//const auto& mins = b1.GetMins( ) - b2.GetMaxs( );
		//const auto& maxs = 

		return CollisionBox( );
	}
}