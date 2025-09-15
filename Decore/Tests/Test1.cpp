#include "../Engine/Renderer/Vulkan/VulkanRenderer.hpp"
#include "../Engine/Renderer/Debug/DebugRenderer.hpp"

#include "../Engine/Renderer/Environment/EnvironmentInfo.hpp"
#include "../Engine/Renderer/Shadows/ShadowMapPass.hpp"

#include "../Engine/Core/Core.hpp"
#include "../Engine/Assets/Assets.hpp"

#include "Test1.hpp"

#include "../../Dependencies/imgui/imgui.h"
#include "../../Dependencies/imgui/backends/imgui_impl_vulkan.h"

#include "../Engine/Renderer/Scene/Scene.hpp"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/matrix_decompose.hpp>

using namespace Engine::Core;
using namespace Engine::Renderer;
using namespace Engine::Assets;

namespace Engine::Tests
{
	static bool ThirdPersonMovement( const float dt, const Core::Camera& camera, glm::vec3& targetPos, glm::mat4* outMatrix )
	{
		const float moveSpeed = 5.f;
		float xdir = float( InputSystem::GetKeyHeld( 'W' ) ) - float( InputSystem::GetKeyHeld( 'S' ) );
		float zdir = float( InputSystem::GetKeyHeld( 'D' ) ) - float( InputSystem::GetKeyHeld( 'A' ) );

		bool isMoving = xdir != 0.f || zdir != 0.f;

		glm::mat4 cameraWorldMatrix = glm::inverse( camera.GetViewMatrix( ) );
		glm::vec3 cameraRight = cameraWorldMatrix[ 0 ];
		glm::vec3 cameraRightXZ = glm::vec3( cameraRight.x, 0.f, cameraRight.z );

		glm::vec3 cameraForward = cameraWorldMatrix[ 2 ];
		glm::vec3 cameraForwardXZ = glm::vec3( cameraForward.x, 0.f, cameraForward.z );
		glm::vec3 cameraPos = cameraWorldMatrix[ 3 ];

		glm::vec3 deltaForward = glm::normalize( cameraPos - targetPos );
		glm::vec3 deltaForwardXZ = glm::normalize( glm::vec3( deltaForward.x, 0.0f, deltaForward.z ) );
		float angle = atan2( zdir, xdir ) + atan2( deltaForwardXZ.x, deltaForwardXZ.z ); // Yaw

		static float oldAngle = angle;

		glm::mat4 localRotation = glm::rotate( glm::mat4( 1.f ), oldAngle, glm::vec3( 0.f, 1.f, 0.f ) );

		if ( isMoving )
			oldAngle = angle;

		glm::vec3 inputStates = glm::vec3( xdir, 0.f, zdir );

		static glm::vec3 oldInput = inputStates;

		glm::vec3 movementInput = glm::mix( oldInput, inputStates, dt );

		if ( glm::length( movementInput ) > 1.f )
			movementInput = glm::normalize( movementInput );

		movementInput = ( ( movementInput.x * glm::normalize( cameraForwardXZ ) ) + ( movementInput.z * glm::normalize( cameraRightXZ ) ) );

		glm::vec3 newMovement = movementInput;

		// Temporary AABB while theres no PlayerController class and Collider components.
		glm::vec3 boxSize = glm::vec3( 0.25f, 0.5f, 0.25f );
		glm::vec3 boxOffset = glm::vec3( 0.f, 0.75f, 0.f );

		Core::CollisionBox playerAABB( targetPos + boxOffset - boxSize, targetPos + boxOffset + boxSize );

#if 0
		glm::vec3 intersectDir{};
		bool hit = m_Scene->Intersects( playerAABB, intersectDir );

		// Wall sliding.
		if ( hit )
		{
			glm::vec3 normal = glm::normalize( intersectDir );
			glm::vec3 projected = normal * glm::dot( newMovement, normal );
			newMovement -= projected;
		}
#endif
		targetPos += ( newMovement * moveSpeed * dt );
#if 0
		// Update AABB and Check for intersection.
		playerAABB = Core::CollisionBox( targetPos + boxOffset - boxSize, targetPos + boxOffset + boxSize );
		hit = m_Scene->Intersects( playerAABB, intersectDir );

		// If we are interescting again offset the player position (Push it away from the intersected AABB).
		if ( hit )
		{
			glm::vec3 normal = glm::normalize( intersectDir );
			glm::vec3 offset = ( normal * boxSize ) - intersectDir;

			targetPos += offset;
		}
#endif
		( *outMatrix ) = localRotation;
		( *outMatrix )[ 3 ] = glm::vec4( targetPos, 1.f );

		oldInput = inputStates;

		return isMoving;
	}

	bool m_ShowImGui{ true };

	// Scene.
	Scene* m_Scene = nullptr;

	// Assets.
	std::vector<Renderer::DescriptorLayout> m_ObjectLayouts{};

	SkinnedGLTFAsset* m_Soldier = nullptr;
	StaticGLTFAsset* m_Bistro = nullptr;
	StaticGLTFAsset* m_Cube = nullptr;

	std::unique_ptr<Physics> m_PhysicsSystem = nullptr;

	void Test1Renderer::Startup()
	{
		m_PhysicsSystem = std::make_unique<Physics>();
		m_PhysicsSystem->Initialize();

		auto MaterialDescriptorLayout = Renderer::DescriptorLayout(m_Context->m_Device, { {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} });
		auto PrimitiveDescriptorLayout = Renderer::DescriptorLayout(m_Context->m_Device, { { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT, nullptr} });

		VkDescriptorBindingFlagsEXT bindingFlagsExt = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlagsExt = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr, 1, &bindingFlagsExt };

		auto TextureDescriptorLayout = Renderer::DescriptorLayout(m_Context->m_Device, { { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } }, &setLayoutBindingFlagsExt);

		m_ObjectLayouts = { MaterialDescriptorLayout, TextureDescriptorLayout, PrimitiveDescriptorLayout };

		m_Bistro = new StaticGLTFAsset("C:\\TestAssets\\Sponza\\Sponza.gltf", m_Context->m_Device, m_Context->m_CommandPool);
		m_Bistro->SetupDevice(m_ObjectLayouts);

		m_Soldier = new SkinnedGLTFAsset("C:\\TestAssets\\Running.glb", m_Context->m_Device, m_Context->m_CommandPool);
		m_Soldier->SetupDevice(m_ObjectLayouts);

		m_Scene = new Scene(m_Context->m_Device, m_Context->m_CommandPool, m_Context->m_SwapChain, m_ObjectLayouts);
		m_Scene->m_Culler = new SceneCuller(m_Context->m_Device, m_Context->m_SwapChain.get(), PrimitiveDescriptorLayout);

		m_Scene->AddAsset(m_Bistro);
		m_Scene->AddSkinnedAsset(m_Soldier);
	}

	glm::vec4 LightDirection = { -0.3f, -1.f, -0.7f, 0.f };
	glm::vec4 LightColor = { 1.f, 1.f, 1.f, 1.f };

	void Test1Renderer::PreRender()
	{
		ShaderRegistry::Get()->OnUpdate();
	}

	void Test1Renderer::RecreateSwapchainResources()
	{
		m_Scene->RecreateSwapchainResources( );
	}

	void Test1Renderer::Update(CommandBuffer* commandBuffer)
	{
		const auto& extents = m_Context->m_SwapChain->GetExtents();

		// Update Camera.
		auto& mainCamera = m_Scene->GetMainCamera();
		mainCamera.SetPerspective(75.f, extents.width / static_cast<float>(extents.height), 0.1f, 100.f);
		mainCamera.Update();

		static glm::vec3 playerPos{ 0, 0, 0 };

		float dt = TimeSystem::GetDeltaTime();

		mainCamera.SetAttachment(playerPos + glm::vec3(0.f, 0.5f, 0.f));

		bool inMotion = ThirdPersonMovement(dt, mainCamera, playerPos, &m_Soldier->m_WorldMatrix);

		m_Soldier->m_ActiveAnimIndex = inMotion ? 1 : 0;

		static bool emoting;
		if (InputSystem::GetKeyPressed('B'))
			emoting = !emoting;

		if (emoting && !inMotion)
			m_Soldier->m_ActiveAnimIndex = 2;

		if (InputSystem::GetKeyPressed(260))
			m_ShowImGui = !m_ShowImGui;

		mainCamera.SetLockRotation(m_ShowImGui);

		m_Scene->LightDirection = LightDirection;
		m_Scene->LightColor = LightColor;

		m_Scene->Render(m_Context->m_CurrentImageIndex, *commandBuffer, *(m_Context->m_ComputeCommandBuffer),
			[&]()
			{
				m_Context->m_DebugRenderer->Render(m_Context,
					[this]()
					{
						if (m_Scene->IsFrustumFrozen())
						{
							ImGui::GetForegroundDrawList()->AddText(ImVec2(10.f, 10.f), IM_COL32(255, 255, 255, 255), "Camera Frustum Frozen");
						}

						if (m_ShowImGui)
						{
							if (ImGui::Begin("Debug"))
							{
								if (ImGui::BeginTabBar("#Tabs"))
								{
									if (ImGui::BeginTabItem("Scene"))
									{
										ImGui::EndTabItem();
									}
									if (ImGui::BeginTabItem("Lighting"))
									{
										ImGui::SliderFloat4("Light Direction", (float*)&LightDirection, -1.f, 1.f);
										ImGui::SliderFloat4("Light Color", (float*)&LightColor, 0.f, 1.f);
											
										static int currentCascade{};
										ImGui::SliderInt("Current Cascade", &currentCascade, 0, SHADOW_MAP_CASCADES - 1);

										ImGui::Image((ImTextureID)m_Scene->m_ShadowMapPass->GetImage(currentCascade), ImVec2(SHADOW_MAP_DIMENSIONS * 0.125f, SHADOW_MAP_DIMENSIONS * 0.125f));
										ImGui::EndTabItem();
									}

									if (ImGui::BeginTabItem("PrePass"))
									{
										ImGui::Image((ImTextureID)m_Scene->m_ImguiDepthPrePass, ImVec2(1920 * 0.25f, 1080 * 0.25f));
										ImGui::EndTabItem();
									}

									if (ImGui::BeginTabItem("Shaders"))
									{
										for (auto& [name, shader] : ShaderRegistry::Get()->GetShaders())
										{
											std::string label = "Reload##" + name;
											ImGui::Text(name.c_str());

											ImGui::SameLine();
											if (ImGui::Button(label.c_str()))
												shader->SetDirty(true);
										}

										ImGui::EndTabItem();
									}

									ImGui::EndTabBar();
								}

								ImGui::End();
							}
						}
					}
				);
			});
	}

	void Test1Renderer::Shutdown( )
	{

	}
}