#pragma once
#include "CollisionBox.hpp"
#include "CollisionCapsule.hpp"
#include "CollisionSphere.hpp"
#include <array>
// Jolt Physics.
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/ObjectStream/ObjectStreamBinaryIn.h>
#include <Jolt/ObjectStream/ObjectStreamBinaryOut.h>
#include <Jolt/Physics/PhysicsSettings.h>
// #include <Jolt/Physics/StateRecorderImpl.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/SoftBody/SoftBodyShape.h>
#include <Jolt/Physics/SoftBody/SoftBodyCreationSettings.h>
#include <Jolt/Physics/SoftBody/SoftBodyMotionProperties.h>

namespace Engine::Core
{
	enum EPhysicsObjectLayers {
		NON_MOVING = 0,
		MOVING = 1,
		NUM_LAYERS = 2,
	};

	constexpr auto sPhysicsObjectLayersNames = std::array{
		"NON_MOVING",
		"MOVING",
		"NUM_LAYERS"
	};

	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NON_MOVING( 0 );
		static constexpr JPH::BroadPhaseLayer MOVING( 1 );
		static constexpr JPH::uint NUM_LAYERS( 2 );
	};

	class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
	public:
		BPLayerInterfaceImpl( )
		{
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[ EPhysicsObjectLayers::NON_MOVING ] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[ EPhysicsObjectLayers::MOVING ] = BroadPhaseLayers::MOVING;
		}

		virtual JPH::uint GetNumBroadPhaseLayers( ) const override
		{
			return BroadPhaseLayers::NUM_LAYERS;
		}

		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer( JPH::ObjectLayer inLayer ) const override
		{
			assert( inLayer < EPhysicsObjectLayers::NUM_LAYERS );
			return mObjectToBroadPhase[ inLayer ];
		}

		virtual const char* GetBroadPhaseLayerName( JPH::BroadPhaseLayer inLayer ) const { return "BroadPhaseLayerName"; }

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[ EPhysicsObjectLayers::NUM_LAYERS ];
	};

	class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
		bool ShouldCollide( JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2 ) const
		{
			switch ( inLayer1 )
			{
				case EPhysicsObjectLayers::NON_MOVING:
					return inLayer2 == BroadPhaseLayers::MOVING;
				case EPhysicsObjectLayers::MOVING:
					return true;
				default:
					JPH_ASSERT( false );
					return false;
			}
		}
	};

	class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
		bool ShouldCollide( JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2 ) const
		{
			switch ( inLayer1 )
			{
				case EPhysicsObjectLayers::NON_MOVING:
					return inLayer2 == EPhysicsObjectLayers::MOVING; // Non moving only collides with moving
				case EPhysicsObjectLayers::MOVING:
					return true; // Moving collides with everything
				default:
					JPH_ASSERT( false );
					return false;
			}
		}
	};


	class Physics {
	public:
		void Initialize( );

		void Step( float dt );
	private:
		std::unique_ptr<JPH::PhysicsSystem> m_PhysicsSystem = nullptr;
		std::unique_ptr<JPH::JobSystem> m_JobSystem = nullptr;
		std::unique_ptr<JPH::TempAllocator> m_TempAllocator = nullptr;
		BPLayerInterfaceImpl m_BroadPhaseLayers;
		ObjectLayerPairFilter m_ObjectLayerPairFilter;
		ObjectVsBroadPhaseLayerFilter m_ObjectVsBroadPhaseFilter;
	};
}