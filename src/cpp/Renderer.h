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
	raii::Instance instance;
	raii::SurfaceKHR display_surface;
	raii::PhysicalDevice physical_device;
	unsigned int graphics_queue;
	unsigned int compute_queue;
	unsigned int decode_queue;
	unsigned int encode_queue;
	unsigned int optical_flow_queue;
	unsigned int present_queue;
	raii::Device device;

	bool has_extensions(const raii::PhysicalDevice &device) const;
	short rank_score(const raii::PhysicalDevice &device) const;
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

	SwapchainSupportDetails query_swap_chain_support(const raii::PhysicalDevice& device) const;



	const std::vector<std::string> own_instance_extensions{
		// Extension names
	};

	const std::vector<std::string> own_instance_layers{
		// Layer names
	};

	const std::vector<std::string> own_device_layers{
		// Layer names, Are they the same thing?
	};

	const std::vector<const char *> device_extensions = {
		vk::KHRSwapchainExtensionName
	};

	static constexpr unsigned int WIDTH = 800;
	static constexpr unsigned int HEIGHT = 600;
};