#pragma once

#include <vector>
#include <string>

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
	unsigned int graphics_queue_index{};
	unsigned int compute_queue_index{};
	unsigned int decode_queue_index{};
	unsigned int encode_queue_index{};
	unsigned int optical_flow_queue_index{};
	unsigned int present_queue_index{};
	raii::Queue graphics_queue{nullptr};
	raii::Queue present_queue{nullptr};
	raii::Device device{nullptr};
	raii::SwapchainKHR swapchain{nullptr};
	std::vector<vk::Image> swapchain_images;
	vk::Format format = {};
	vk::Extent2D extent{};
	std::vector<raii::ImageView> image_views;
	raii::PipelineLayout pipeline_layout{nullptr};
	raii::Pipeline graphics_pipeline{nullptr};
	raii::CommandPool command_pool{nullptr};
	raii::CommandBuffer command_buffer{nullptr};

	[[nodiscard]] bool has_extensions(const raii::PhysicalDevice &device) const;
	[[nodiscard]] short rank_score(const raii::PhysicalDevice &device) const;
	void choose_physical_device();
	void create_display_surface();
	void create_vulkan_instance();
	void create_window();
	void get_queue_indices();
	void create_logical_device();
	void create_swapchain();
	void create_image_views();

	[[nodiscard]] static std::vector<char> readFile(const std::string &filename);
	[[nodiscard]] raii::ShaderModule create_shader_module(std::vector<char> code) const;

	void create_graphics_pipeline();
	void create_command_pool();
	void create_command_buffer();
	void transition_image_layout(
		uint32_t imageIndex,
		vk::ImageLayout		oldLayout,	vk::ImageLayout		newLayout,
		vk::AccessFlags2	srcAccessMask,	vk::AccessFlags2	dstAccessMask,
		vk::PipelineStageFlags2	srcStageMask,	vk::PipelineStageFlags2	dstStageMask);
	void record_command_buffer(const unsigned int &index);
	void create_sync_objects();

	static vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
	static vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
	static vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

	struct SwapchainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
	};

	[[nodiscard]] SwapchainSupportDetails query_swap_chain_support(const raii::PhysicalDevice& device) const;



	raii::Semaphore present_complete_semaphore{nullptr};
	raii::Semaphore render_finished_semaphore{nullptr};
	raii::Fence draw_fence{nullptr};

	void draw_frame();



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

	const std::vector<const char *> device_extensions = {
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName
	};

	static constexpr unsigned int WIDTH  = 800;
	static constexpr unsigned int HEIGHT = 600;

	static constexpr bool NO_FRAMES = false;
};