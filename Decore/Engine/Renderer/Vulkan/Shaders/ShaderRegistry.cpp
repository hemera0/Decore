#include "../VulkanRenderer.hpp"

namespace Engine::Renderer
{
	void ShaderRegistry::OnUpdate()
	{
		for (auto& [name, shader] : GetShaders())
		{
			if (shader->GetDirty())
			{
				shader->Recompile();

				for (auto* pipeline : shader->GetPipelines())
					pipeline->Recreate();

				shader->SetDirty(false);
			}
		}
	}
}