#include "Collision.hpp"
#include <cstdarg>
#include <iostream>
namespace Engine::Core
{
	constexpr const JPH::uint MAX_RIGIDBODIES = 65536;

	static void TraceImpl( const char* inFMT, ... )
	{
		va_list list;
		va_start( list, inFMT );
		char buffer[ 1024 ];
		vsnprintf( buffer, sizeof( buffer ), inFMT, list );
		va_end( list );

		std::cout << buffer << std::endl;
	}

	void Physics::Initialize( )
	{
		JPH::RegisterDefaultAllocator( );
		JPH::Trace = TraceImpl;

		JPH::Factory::sInstance = new JPH::Factory( );
		JPH::RegisterTypes( );

		m_PhysicsSystem = std::make_unique<JPH::PhysicsSystem>( );
		m_PhysicsSystem->Init( MAX_RIGIDBODIES, 0, MAX_RIGIDBODIES, 10240, m_BroadPhaseLayers, m_ObjectVsBroadPhaseFilter, m_ObjectLayerPairFilter );

		m_TempAllocator = std::make_unique<JPH::TempAllocatorImpl>( 100 * 1024 * 1024 );
		m_JobSystem = std::make_unique<JPH::JobSystemThreadPool>( JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency( ) - 1 );

		printf( "Initialized Jolt\n" );
	}

	void Physics::Step( float dt )
	{

	}
}