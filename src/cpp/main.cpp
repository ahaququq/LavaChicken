// #include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#undef GLFW_INCLUDE_VULKAN
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

import vulkan_hpp;

namespace raii = vk::raii;



static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	const long fileSize = file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}



constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;

GLFWwindow *setup_window() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, false);

	return glfwCreateWindow(WIDTH, HEIGHT, "LavaChicken main window", nullptr, nullptr);
}



auto setup_instance() {
	const raii::Context context{};

	vk::ApplicationInfo app_info{
		"LavaChicken on Vulkan - Test App", vk::makeApiVersion(1, 0, 0, 0), "LavaChicken",
		vk::makeApiVersion(1, 0, 0, 0),			vk::makeApiVersion(1, 0, 0, 0),
	};

	unsigned int glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	const vk::InstanceCreateInfo instance_create_info{{}, (&app_info), 0, nullptr, glfwExtensionCount, glfwExtensions};

	return std::move(raii::Instance{context, instance_create_info});
}



const std::vector<const char *> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

bool has_extensions(const raii::PhysicalDevice &device) {
	const std::vector<vk::ExtensionProperties> available_extensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> required_extensions{device_extensions.begin(), device_extensions.end()};

	for (auto extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}



struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

SwapChainSupportDetails query_swap_chain_support(const raii::PhysicalDevice& device, const raii::SurfaceKHR& surface) {
	SwapChainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}



short rank_score(const raii::PhysicalDevice &device, const raii::SurfaceKHR &surface) {
	vk::PhysicalDeviceProperties properties = device.getProperties();
	vk::PhysicalDeviceFeatures features = device.getFeatures();

	std::cout << "Device: " << properties.deviceName << " (Type: " << static_cast<int>(properties.deviceType) << ")" << std::endl;

	short score = 0;

	if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
		score += 1000;
	}

	if (properties.deviceType == vk::PhysicalDeviceType::eCpu) {
		score -= 1000;
	}

	if (!features.geometryShader) return INT16_MIN;
	if (!has_extensions(device)) return INT16_MIN;

	const SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device, surface);

	if (swap_chain_support.formats.empty()) return INT16_MIN;
	if (swap_chain_support.presentModes.empty()) return INT16_MIN;

	return score;
}



auto choose_physical_device(const raii::Instance &instance, const raii::SurfaceKHR &surface) {
	std::vector<raii::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();

	std::multimap<short, raii::PhysicalDevice &> ranked_devices;

	for (raii::PhysicalDevice& device: physical_devices) {
		short score = rank_score(device, surface);
		if (score == INT16_MIN) continue;
		ranked_devices.insert(std::pair<short, raii::PhysicalDevice&>(-score, device));
	}

	if (ranked_devices.empty()) {
		throw std::runtime_error("No suitable GPU found");
	}

	return std::move(ranked_devices.begin()->second);
}



auto create_surface(const raii::Instance &instance, GLFWwindow* window) {
	VkSurfaceKHR surface_khr;

	glfwCreateWindowSurface(*instance, window, nullptr, &surface_khr);

	return std::move(raii::SurfaceKHR{instance, surface_khr});
}



void check_queue_flags(std::map<vk::QueueFlagBits, unsigned int> &queue_family_indices, const unsigned int &i,
					   const vk::QueueFlags &flags, const vk::QueueFlagBits &bits) {
	if (flags & bits)
		queue_family_indices.emplace(std::pair(bits, i));
}



struct QueueIndices {
	std::map<vk::QueueFlagBits, unsigned int> queue_family_indices;
	std::optional<unsigned int> present_queue_family_index;
};

QueueIndices setup_queue_indices(const raii::PhysicalDevice &physical_device, const raii::SurfaceKHR &display_surface) {
	auto queue_family_properties = physical_device.getQueueFamilyProperties();

	QueueIndices queue_indices;

	unsigned int i = 0;
	for (auto property: queue_family_properties) {
		vk::QueueFlags flags = property.queueFlags;
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eGraphics);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eCompute);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eTransfer);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eSparseBinding);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eProtected);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eVideoDecodeKHR);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eVideoEncodeKHR);
		check_queue_flags(queue_indices.queue_family_indices, i, flags, vk::QueueFlagBits::eOpticalFlowNV);

		if (physical_device.getSurfaceSupportKHR(i, display_surface))
			queue_indices.present_queue_family_index = i;

		std::cout << "Queue #" << i << ": ";
		std::cout << std::bitset<8>(static_cast<unsigned int>(property.queueFlags)) << std::endl;

		i++;
	}

	return queue_indices;
}



auto setup_logical_device(const raii::PhysicalDevice &physical_device, QueueIndices &queue_indices) {
	vk::PhysicalDeviceFeatures device_features;

	const std::set unique_queue_families{queue_indices.queue_family_indices[vk::QueueFlagBits::eGraphics], *queue_indices.present_queue_family_index};

	std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;

	constexpr float queue_priority = 1.0f;

	for (const auto family: unique_queue_families) {
		queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, family, 1, &queue_priority});
	}

	const vk::DeviceCreateInfo device_create_info{
		{},
		static_cast<uint32_t>(queue_create_infos.size()),
		queue_create_infos.data(),
		0,
		{},
		static_cast<uint32_t>(device_extensions.size()),
		device_extensions.data(),
		&device_features
	};

	return std::move(raii::Device{physical_device, device_create_info});
}



vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto format : availableFormats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return format;
		}
	}

	return availableFormats[0];
}



vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}



vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		return capabilities.currentExtent;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	vk::Extent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

	actualExtent.width =
			std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height =
			std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}



void create_swap_chain(
	const	raii::PhysicalDevice	&physical_device,
	const	raii::SurfaceKHR		&surface,
	GLFWwindow				*window,
	const	raii::Device			&device,
	QueueIndices			queue_indices,
	raii::SwapchainKHR		&swapchain_out,
	vk::Format				&format_out,
	vk::Extent2D			&extent_out
) {
	const SwapChainSupportDetails support_details = query_swap_chain_support(physical_device, surface);

	const vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(support_details.formats);
	const vk::PresentModeKHR present_mode = choose_swap_present_mode(support_details.presentModes);
	const vk::Extent2D extent = choose_swap_extent(support_details.capabilities, window);

	unsigned int image_count = support_details.capabilities.minImageCount + 1;

	if (support_details.capabilities.maxImageCount > 0 && image_count > support_details.capabilities.maxImageCount) {
		image_count = support_details.capabilities.maxImageCount;
	}

	const bool same_queues = *queue_indices.present_queue_family_index ==
					   queue_indices.queue_family_indices[vk::QueueFlagBits::eGraphics];

	unsigned int queue_family_indices[] = {queue_indices.queue_family_indices[vk::QueueFlagBits::eGraphics],
										   *queue_indices.present_queue_family_index};

	const vk::SwapchainCreateInfoKHR swapchain_create_info{
			{},
			surface,
			image_count,
			surface_format.format,
			surface_format.colorSpace,
			extent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			(same_queues ? vk::SharingMode::eExclusive : vk::SharingMode::eConcurrent),
			(same_queues ? 0u : 2u),
			(same_queues ? nullptr : queue_family_indices),
			support_details.capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode,
			true};

	swapchain_out = std::move(raii::SwapchainKHR{device, swapchain_create_info});
	format_out = surface_format.format;
	extent_out = extent;
}



auto create_swapchain_image_views(
	const raii::Device &device,
	const vk::Format &surface_format,
	const std::vector<vk::Image> &swap_chain_images
) {
	std::vector<raii::ImageView> swapchain_image_views;

	for (auto image: swap_chain_images) {
		vk::ImageViewCreateInfo create_info{{},
		image,
		vk::ImageViewType::e2D,
		surface_format,
		{},
		vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
		swapchain_image_views.emplace_back(device, create_info);
	}

	return std::move(swapchain_image_views);
}



auto create_shader_module(const std::vector<char> &code, const raii::Device &device) {
	vk::ShaderModuleCreateInfo shader_module_create_info{
		{},
		code.size(),
		reinterpret_cast<const uint32_t *>(code.data())
	};

	return std::move(raii::ShaderModule{device, shader_module_create_info});
}



struct graphics_pipeline_creation_info {
	raii::ShaderModule vert;
	raii::ShaderModule frag;
	vk::PipelineShaderStageCreateInfo cvert;
	vk::PipelineShaderStageCreateInfo cfrag;
};



auto create_graphics_pipeline(const raii::Device &device) {
	const auto vert_code = readFile("shader.vert.spv");
	const auto frag_code = readFile("shader.frag.spv");

	auto vert_shader_module = create_shader_module(vert_code, device);
	auto frag_shader_module = create_shader_module(frag_code, device);

	const vk::PipelineShaderStageCreateInfo vert_create_info{
		{},
		vk::ShaderStageFlagBits::eVertex,
		vert_shader_module,
		"main"
	};

	const vk::PipelineShaderStageCreateInfo frag_create_info{
		{},
		vk::ShaderStageFlagBits::eFragment,
		frag_shader_module,
		"main"
	};

	graphics_pipeline_creation_info output = {
		std::move(vert_shader_module),
		std::move(frag_shader_module),
		vert_create_info,
		frag_create_info
	};

	return output;
}



auto create_render_pass(const raii::Device &device, const vk::Format surface_format) {
	vk::AttachmentDescription color_attachment{
	{},
		surface_format,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR
	};

	vk::AttachmentReference color_attachment_ref{
		0,
		vk::ImageLayout::eAttachmentOptimal
	};

	vk::SubpassDescription subpass{
		{},
		vk::PipelineBindPoint::eGraphics,
		0,
		{},
		1,
		&color_attachment_ref
	};

	vk::RenderPassCreateInfo render_pass_create_info{
		{},
		1,
		&color_attachment,
		1,
		&subpass
	};

	raii::RenderPass render_pass{
		device,
		render_pass_create_info
	};

	return std::pair{
		std::move(render_pass),
		color_attachment
	};
}



auto configure_graphics_pipeline(
	vk::Extent2D surface_extent,
	const raii::Device &device
) {
	std::vector<vk::DynamicState> dynamic_states = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info = {
		{},
		static_cast<uint32_t>(dynamic_states.size()),
		dynamic_states.data()
	};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
		{},
		0,
		nullptr,
		0,
		nullptr
	};

	vk::PipelineInputAssemblyStateCreateInfo assembly_state_create_info = {
		{},
		vk::PrimitiveTopology::eTriangleList,
		false
	};

	vk::Viewport viewport = {
		0, 0,
		static_cast<float>(surface_extent.width),
		static_cast<float>(surface_extent.height),
		0, 1
	};

	vk::Rect2D scissor = {{0, 0}, surface_extent};

	vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info = {
		{},
		1,
		&viewport,
		1,
		&scissor
	};

	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info = {
		{},
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eClockwise,
		false,
		0,
		0,
		0,
		1
	};

	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info = {
		{},
		vk::SampleCountFlagBits::e1,
		false,
		1.0,
		nullptr,
		false,
		false
	};

	vk::PipelineColorBlendAttachmentState color_blend_attachment = {
		true,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		(
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA
		)
	};

	vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info = {
		{},
		false,
		vk::LogicOp::eCopy,
		1,
		&color_blend_attachment,
		{
			0, 0, 0, 0
		}
	};

	vk::PipelineLayoutCreateInfo pipeline_layout_create_info = {
		{},
		0,
		nullptr,
		0,
		nullptr
	};

	raii::PipelineLayout pipeline_layout = {
		device,
		pipeline_layout_create_info
	};

	return std::move(pipeline_layout);
}



void start() {
	// ---------- Init window ----------

	GLFWwindow *window = setup_window();

	// ---------- Init Vulkan ----------

	raii::Instance instance = setup_instance();
	raii::SurfaceKHR display_surface = create_surface(instance, window);

	raii::PhysicalDevice physical_device = choose_physical_device(instance, display_surface);

	vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
	std::cout << "Chosen device: " << device_properties.deviceName
			  << " (Type: " << static_cast<int>(device_properties.deviceType) << ")" << std::endl;

	auto queue_indices = setup_queue_indices(physical_device, display_surface);

	std::cout << "Has graphics: " << queue_indices.queue_family_indices.contains(vk::QueueFlagBits::eGraphics) << std::endl;
	std::cout << "Has present: " << queue_indices.present_queue_family_index.has_value() << std::endl;

	raii::Device device = setup_logical_device(physical_device, queue_indices);

	raii::Queue graphicsQueue{device, queue_indices.queue_family_indices[vk::QueueFlagBits::eGraphics], 0};



	raii::SwapchainKHR swapchain{nullptr};
	vk::Format surface_format;
	vk::Extent2D surface_extent;
	create_swap_chain(physical_device, display_surface, window, device, queue_indices, swapchain, surface_format, surface_extent);

	std::vector<vk::Image> swap_chain_images = swapchain.getImages();
	std::vector<raii::ImageView> swapchain_image_views = create_swapchain_image_views(device, surface_format, swap_chain_images);

	auto shader_pipeline_out = create_graphics_pipeline(device);

	raii::RenderPass render_pass = create_render_pass(device, surface_format);

	raii::PipelineLayout pipeline_layout = configure_graphics_pipeline(surface_extent, device);



	// ---------- Main Loop ----------

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	// ---------- Cleanup ----------



	swapchain.clear();

	glfwDestroyWindow(window);

	glfwTerminate();
}



int main() {

	try {
		start();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
