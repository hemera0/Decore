# Decore
Prototype Vulkan Game Engine

![banner](/banner.jpeg)
Decore is an old Vulkan game engine framework I was working on but have since moved on to other projects. However it does make use of a few interesting features I could not find in other similar projects.

Decore implements the following features
* Simple Cascaded Shadow Maps
* AMD's FidelityFX CACAO (Screen Space Ambient Occlusion)
* A Simple Third Person Camera Controller
* Heavy usage of the Vulkan extension `VK_EXT_descriptor_buffer`
* Support for loading PBR Materials from glTF Models
* Support for loading Animations from glTF Models
* GPU Driven Rendering with DrawIndirect & Compute Culling
and has a lot of other prototyped and unfinished features.

The project is largely incomplete and for that I will not be including the dependencies however if you are truly interested in building the project you will need the following libraries. 
* entt
* AMD FidelityFX CACAO
* GLFW
* glm
* ImGUI
* JoltPhysics
* shaderc
* stb_image
* tinygltf
* Volk
