#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>
#include <map>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN

#include <vulkan/vulkan_raii.hpp>

namespace raii = vk::raii;

class Renderer {
public:
	Renderer();
	void main_loop();
	~Renderer();

private:
	GLFWwindow *window{};
	raii::Context context;
	raii::Instance instance{nullptr};
	raii::SurfaceKHR display_surface{nullptr};
	raii::PhysicalDevice physical_device{nullptr};
	unsigned int graphics_queue{};
	unsigned int compute_queue{};
	unsigned int decode_queue{};
	unsigned int encode_queue{};
	unsigned int optical_flow_queue{};
	unsigned int present_queue{};
	raii::Device device{nullptr};

	[[nodiscard]] bool has_extensions(const raii::PhysicalDevice &device) const;
	[[nodiscard]] short rank_score(const raii::PhysicalDevice &device) const;
	void choose_physical_device();
	void create_display_surface();
	void create_vulkan_instance();
	void create_window();
	void get_queue_indices();
	void create_logical_device();

	struct SwapchainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	[[nodiscard]] SwapchainSupportDetails query_swap_chain_support(const raii::PhysicalDevice& device) const;


	vk::ApplicationInfo appInfo = vk::ApplicationInfo{
		"LavaChicken on Vulkan - Test App",
		vk::makeApiVersion(1, 0, 0, 0),
		"LavaChicken",
		vk::makeApiVersion(1, 0, 0, 0),
		vk::ApiVersion14
	};

	const std::vector<std::string> own_instance_extensions{
		// Extension names
	};

	const std::vector<std::string> own_instance_layers{
		// Layer names
#ifndef NDEBUG
		"VK_LAYER_KHRONOS_validation" // Debug only
#endif
	};

	const std::vector<std::string> own_device_layers{
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

	const std::vector<const char *> device_extensions = {
		vk::KHRSwapchainExtensionName
	};

	static constexpr unsigned int WIDTH = 800;
	static constexpr unsigned int HEIGHT = 600;
};