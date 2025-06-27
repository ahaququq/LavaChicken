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
	GLFWwindow *window;
	raii::Context context;
	raii::Instance instance;
	raii::SurfaceKHR display_surface;
	raii::PhysicalDevice physical_device;

	short rank_score(const raii::PhysicalDevice &device) const;
	void choose_physical_device();
	void create_display_surface();
	void create_vulkan_instance();
	void create_window();

	struct SwapchainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	SwapchainSupportDetails query_swap_chain_support(const raii::PhysicalDevice& device) const;
};