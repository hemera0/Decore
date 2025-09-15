#include "GLTFImporter.hpp"
#include "../../Core/Components/Components.hpp"
#pragma once

namespace Engine::Assets {
	GLTFImporter::GLTFImporter(Renderer::Scene& scene) : m_Scene(scene)
	{

	}

	bool GLTFImporter::LoadFromFile(const std::string& path)
	{


		return true;
	}

	void GLTFImporter::ParseNode(const tinygltf::Node& inputNode, entt::entity parent, glm::mat4 transform)
	{
		glm::mat4 localTransform(1.f);
		bool hasTransform = false;

		if (inputNode.matrix.size() == 16) {
			localTransform = glm::make_mat4(inputNode.matrix.data());
			hasTransform = true;
		}
		else {
			if (inputNode.translation.size() == 3)
			{
				localTransform = glm::translate(localTransform, glm::vec3(glm::make_vec3(inputNode.translation.data())));
				hasTransform = true;
			}

			if (inputNode.rotation.size() == 4)
			{
				localTransform = localTransform * glm::mat4(glm::quat(glm::make_quat(inputNode.rotation.data())));
				hasTransform = true;
			}

			if (inputNode.scale.size() == 3)
			{
				localTransform = glm::scale(localTransform, glm::vec3(glm::make_vec3(inputNode.scale.data())));
				hasTransform = true;
			}
		}

		transform *= localTransform;

		entt::entity entity = parent;

		if (hasTransform || (inputNode.mesh > -1 && m_Scene.m_Registry.all_of<Core::Mesh>(parent))) {
			if (parent != entt::null) {

			}
		}

		if (hasTransform) {

		}

		if (inputNode.mesh > -1) {
			const auto& mesh = m_GltfFile.meshes[inputNode.mesh];

			if (mesh.primitives.size() == 1) {
				ParseMesh(entity, mesh.primitives[0]);
			}
			else if (mesh.primitives.size() >= 1) {
				for (auto i = 0; i < mesh.primitives.size(); i++) {
					entt::entity subEntity = m_Scene.m_Registry.create();

					ParseMesh(subEntity, mesh.primitives[i]);
				}
			}
		}

		for (auto i = 0; i < inputNode.children.size(); i++)
			ParseNode(m_GltfFile.nodes[inputNode.children[i]], entity, transform);
	}

	void GLTFImporter::ParseMaterials(entt::entity entity, const tinygltf::Material& srcMaterial)
	{

	}

	void GLTFImporter::ParseMesh(entt::entity entity, const tinygltf::Primitive& srcPrimitive)
	{
		if (srcPrimitive.mode != TINYGLTF_MODE_TRIANGLES)
			return;

		auto& mesh = m_Scene.m_Registry.emplace<Core::Mesh>(entity);

		// Link material.
		if (srcPrimitive.material > -1)
			mesh.m_Material = m_MaterialEntities[srcPrimitive.material];

		for (const auto& [attributeName, attributeAccessor] : srcPrimitive.attributes) {
			if (attributeAccessor < 0)
				continue;

			const auto& accessor = m_GltfFile.accessors[attributeAccessor];
			if (accessor.bufferView < 0)
				continue;

			const auto& bufferView = m_GltfFile.bufferViews[accessor.bufferView];
			const auto& buffer = m_GltfFile.buffers[bufferView.buffer];
			if (attributeName == "POSIITON") {
				for (size_t i = 0; i < accessor.count; i++) {

				}
			}
		}

		// Grab Indices.
		if (srcPrimitive.indices > -1) {
			const auto& accessor = m_GltfFile.accessors[srcPrimitive.indices];
			const auto& bufferView = m_GltfFile.bufferViews[accessor.bufferView];
			const auto& buffer = m_GltfFile.buffers[bufferView.buffer];
			const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

			if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
				const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					mesh.m_Indices.push_back(buf[index]);
				}
			}

			if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					mesh.m_Indices.push_back(buf[index]);
				}
			}

			if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
				const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					mesh.m_Indices.push_back(buf[index]);
				}
			}
		}
	}
}