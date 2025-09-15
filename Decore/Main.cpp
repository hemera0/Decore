#include "Engine/Core/Application/Application.hpp"
#include "Engine/Renderer/Vulkan/VulkanRenderer.hpp"

#include "Tests/Test1.hpp"

using namespace Engine::Core;
using namespace Engine::Renderer;
using namespace Engine::Tests;

std::unique_ptr<Application> MyApp = std::make_unique<Application>(1920, 1080);
std::unique_ptr<Test1Renderer> Test1{};

int main() {
	if (!MyApp->Startup()) {
		return -1;
	}

	auto* renderer = MyApp->GetRenderer();

	Test1 = std::make_unique<Test1Renderer>( renderer );

	renderer->AddRenderer(Test1.get());

	MyApp->Update(); // Main Loop.
	MyApp->Shutdown();
}