#include "Renderer.h"

#include <bitset>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <map>
#include <set>
#include <chrono>

#include "text_formatting.h"

namespace raii = vk::raii;
namespace ch = std::chrono;


bool Renderer::has_extensions(const raii::PhysicalDevice &device) const {
	const std::vector<vk::ExtensionProperties> available_extensions = device.enumerateDeviceExtensionProperties();

	std::set<std::string> required_extensions{device_extensions.begin(), device_extensions.end()};

	for (auto extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}



Renderer::SwapchainSupportDetails Renderer::query_swap_chain_support(const raii::PhysicalDevice& device) const {
	SwapchainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(display_surface);
	details.formats = device.getSurfaceFormatsKHR(display_surface);
	details.presentModes = device.getSurfacePresentModesKHR(display_surface);

	return details;
}



short Renderer::rank_score(const raii::PhysicalDevice &device) const {
	const vk::PhysicalDeviceProperties properties = device.getProperties();
	const vk::PhysicalDeviceFeatures features = device.getFeatures();

	short score = 0;

	if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
		score += 1000;
	}

	if (properties.deviceType == vk::PhysicalDeviceType::eCpu) {
		score -= 1000;
	}

	if (!features.geometryShader) return INT16_MIN;
	if (!has_extensions(device)) return INT16_MIN;
	if (properties.apiVersion < vk::ApiVersion13) return INT16_MIN;

	const SwapchainSupportDetails swap_chain_support = query_swap_chain_support(device);

	if (swap_chain_support.formats.empty()) return INT16_MIN;
	if (swap_chain_support.presentModes.empty()) return INT16_MIN;

	return score;
}



void Renderer::choose_physical_device() {
	wnd::begin_section("Physical device: ");

	std::vector<raii::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();

	if (physical_devices.empty()) {
		wnd::print("None");
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::multimap<short, raii::PhysicalDevice &> ranked_devices;

	for (raii::PhysicalDevice& device: physical_devices) {
		const short score = rank_score(device);
		if (score == INT16_MIN) continue;
		ranked_devices.insert(std::pair<short, raii::PhysicalDevice&>(-score, device));
	}

	if (ranked_devices.empty()) {
		throw std::runtime_error("No suitable GPU found");
	}

	wnd::begin_frame("Suitable GPUs:");

	bool first = true;

	for (std::pair<const short, const raii::PhysicalDevice &> device: ranked_devices) {
		wnd::print(
			std::string{first ? ">T" : " T"}
			+ std::to_string(static_cast<int>(device.second.getProperties().deviceType))
			+ ", " + wnd::set_length(std::to_string(-device.first), 5) + " points - "
			+ std::string{device.second.getProperties().deviceName}
			+ std::string{first ? "<" : ""});
		first = false;
	}

	wnd::end_frame();

	physical_device = std::move(ranked_devices.begin()->second);

	wnd::print();
}



void Renderer::create_display_surface() {
	VkSurfaceKHR surface_khr;
	if (glfwCreateWindowSurface(*instance, window, nullptr, &surface_khr))
		throw std::runtime_error("failed to create window surface!");
	display_surface = raii::SurfaceKHR{instance, surface_khr};
}



void Renderer::create_vulkan_instance() {
	wnd::begin_section("Vulkan instance:");

	unsigned int glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	const char **glfwExtensionsCopy = glfwExtensions;

	std::unordered_set<const char *> extensions;

	wnd::begin_frame("GLFW required extensions:");
	for (int i = 0; i < glfwExtensionCount; i++) {
		const char *glfwExtensionsPointer = *glfwExtensionsCopy;
		extensions.insert(glfwExtensionsPointer);
		wnd::print(std::string{"+ "} + glfwExtensionsPointer);
		glfwExtensionsCopy++;
	}
	wnd::end_frame();

	wnd::begin_frame("LavaChicken required extensions:");
	for (const std::string &extension: own_instance_extensions) {
		const char *name = extension.c_str();
		wnd::print((extensions.insert(name).second ? "+ " : "~ ") + extension);
	}
	wnd::end_frame();
	const std::vector<const char *> extensions_vector = {extensions.begin(), extensions.end()};


	std::unordered_set<const char *> layers;

	wnd::begin_frame("LavaChicken required layers:");
	for (const std::string &layer: own_instance_layers) {
		const char *name = layer.c_str();
		wnd::print((layers.insert(name).second ? "+ " : "~ ") + layer);
	}
	wnd::end_frame();
	const std::vector<const char *> layers_vector = {layers.begin(), layers.end()};


	wnd::begin_frame("Unsupported extensions:");
	bool unsupportedExtensions = false;
	auto extensionProperties = context.enumerateInstanceExtensionProperties();
	for (auto extension : extensions_vector)
	{
		if (std::ranges::none_of(extensionProperties,
			[extension](auto const& extensionProperty)
			{ return strcmp(extensionProperty.extensionName, extension) == 0; }
		)) {
			unsupportedExtensions = true;
			wnd::print(std::string("- ") + extension);
		}
	}
	if (unsupportedExtensions) throw std::runtime_error("Required extension not supported.");
	wnd::print("None :)");
	wnd::end_frame();

	wnd::begin_frame("Unsupported layers:");
	bool unsupportedLayers = false;
	auto layerProperties = context.enumerateInstanceLayerProperties();
	for (auto layer : layers_vector)
	{
		if (std::ranges::none_of(layerProperties,
			[layer](auto const& layerProperty)
			{ return strcmp(layerProperty.layerName, layer) == 0; }
		)) {
			unsupportedLayers = true;
			wnd::print(std::string("- ") + layer);
		}
	}
	if (unsupportedLayers) throw std::runtime_error("Required layer not supported.");
	wnd::print("None :)");
	wnd::end_frame();


	instance = raii::Instance{
		context,
		vk::InstanceCreateInfo{
			{},
			&appInfo,
			static_cast<unsigned int>(layers_vector.size()),
			layers_vector.data(),
			static_cast<unsigned int>(extensions_vector.size()),
			extensions_vector.data()
		}
	};

	wnd::print();
}



void Renderer::create_window() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, false);

	window = glfwCreateWindow(WIDTH, HEIGHT, "LavaChicken main window", nullptr, nullptr);
	wnd::begin("LavaChicken debug console", wnd::all_buttons, 64);
	wnd::begin_section("Window:");
	wnd::begin_frame("Requested:");
	wnd::print(std::string{"Width:   "} + std::to_string(WIDTH));
	wnd::print(std::string{"Height:  "} + std::to_string(HEIGHT));
	wnd::print(std::string{"Monitor: "} + "N/A");
	wnd::end_frame();

	int width, height;
	glfwGetWindowSize(window, &width, &height);

	wnd::begin_frame("Got:");
	wnd::print(std::string{"Width:   "} + std::to_string(width));
	wnd::print(std::string{"Height:  "} + std::to_string(height));

	auto monitor = glfwGetWindowMonitor(window);
	if (monitor) {
		int monitor_width, monitor_height;
		glfwGetMonitorPhysicalSize(monitor, &monitor_width, &monitor_height);
		float scale_x, scale_y;
		glfwGetMonitorContentScale(monitor, &scale_x, &scale_y);
		int pos_x, pos_y;
		glfwGetMonitorPos(monitor, &pos_x, &pos_y);
		int work_x, work_y, work_width, work_height;
		glfwGetMonitorWorkarea(monitor, &work_x, &work_y, &work_width, &work_height);

		wnd::begin_frame("Monitor: ");
		wnd::print(std::string{"Name:             "} + glfwGetMonitorName(monitor));
		wnd::print(std::string{"Width:            "} + std::to_string(monitor_width) + "mm");
		wnd::print(std::string{"Height:           "} + std::to_string(monitor_height) + "mm");
		wnd::print(std::string{"Scale X:          "} + std::to_string(scale_x));
		wnd::print(std::string{"Scale Y:          "} + std::to_string(scale_y));
		wnd::print(std::string{"Position X:       "} + std::to_string(pos_x));
		wnd::print(std::string{"Position Y:       "} + std::to_string(pos_y));
		wnd::print(std::string{"Work area X:      "} + std::to_string(work_x));
		wnd::print(std::string{"Work area Y:      "} + std::to_string(work_y));
		wnd::print(std::string{"Work area width:  "} + std::to_string(work_width));
		wnd::print(std::string{"Work area height: "} + std::to_string(work_height));
	} else {
		wnd::print(std::string{"Monitor: "} + "N/A");
	}

	wnd::end_frame();

	wnd::end_frame();
	wnd::print();
}



void Renderer::get_queue_indices() {
	wnd::begin_section("Queues: ");
	auto queue_family_properties = physical_device.getQueueFamilyProperties();

	unsigned int i = 0;
	for (auto property: queue_family_properties) {
		wnd::begin_frame(std::to_string(i));

		vk::QueueFlags flags = property.queueFlags;
		if (flags & vk::QueueFlagBits::eGraphics) {
			graphics_queue_index = i;
			wnd::print("Graphics");
		}
		if (flags & vk::QueueFlagBits::eCompute) {
			compute_queue_index = i;
			wnd::print("Compute");
		}
		if (flags & vk::QueueFlagBits::eTransfer) wnd::print("Transfer");
		if (flags & vk::QueueFlagBits::eSparseBinding) wnd::print("SparseBinding");
		if (flags & vk::QueueFlagBits::eProtected) wnd::print("Protected");
		if (flags & vk::QueueFlagBits::eVideoDecodeKHR) {
			decode_queue_index = i;
			wnd::print("Decode");
		}
		if (flags & vk::QueueFlagBits::eVideoEncodeKHR) {
			encode_queue_index = i;
			wnd::print("Encode");
		}
		if (flags & vk::QueueFlagBits::eOpticalFlowNV) {
			optical_flow_queue_index = i;
			wnd::print("Optical Flow");
		}
		if (physical_device.getSurfaceSupportKHR(i, display_surface)) {
			present_queue_index = i;
			wnd::print("Present");
		}
		wnd::end_frame();

		i++;
	}
	wnd::print();
}



void Renderer::create_logical_device() {
	wnd::begin_section("Logical device:");

	const std::set unique_queues = {
		graphics_queue_index,
		present_queue_index
	};

	constexpr float queue_priority = 1.0f;
	std::vector<vk::DeviceQueueCreateInfo> device_queue_create_infos;

	wnd::begin_frame("LavaChicken queues:");
	for (const auto family: unique_queues) {
		device_queue_create_infos.emplace_back(vk::DeviceQueueCreateInfo{{}, family, 1, &queue_priority});
		wnd::print(std::to_string(family));
	}
	wnd::end_frame();

	// Create a chain of feature structures
	vk::StructureChain featureChain = {
		physical_device.getFeatures2(), // vk::PhysicalDeviceFeatures2 (empty for now)
		vk::PhysicalDeviceVulkan11Features{
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true
		},
		vk::PhysicalDeviceVulkan13Features{
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			false,
			true,
			false,
			false,
			true,
			false,
			false
			},      // Enable dynamic rendering from Vulkan 1.3
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT{ true }   // Enable extended dynamic state from the extension
	};

	const vk::DeviceCreateInfo device_create_info{
		{},
		static_cast<uint32_t>(device_queue_create_infos.size()),
		device_queue_create_infos.data(),
		0,
		nullptr,
		static_cast<uint32_t>(device_extensions.size()),
		device_extensions.data(),
		nullptr,
		featureChain.get<>()
	};

	device = {physical_device, device_create_info};

	graphics_queue = raii::Queue{device, graphics_queue_index, 0};
	if (graphics_queue_index != present_queue_index) {
		present_queue = raii::Queue{device, present_queue_index, 1};
	} else {
		present_queue = graphics_queue;
	}

	if (unique_queues.size() > 2) throw std::runtime_error("More queus. Won't deal with them"); // Impossible for now

	wnd::print();
}



vk::SurfaceFormatKHR Renderer::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto format : availableFormats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return format;
		}
	}

	return availableFormats[0];
}



vk::PresentModeKHR Renderer::choose_swap_present_mode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}



vk::Extent2D Renderer::choose_swap_extent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
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



void Renderer::create_swapchain() {
	wnd::begin_section("Swapchain: ");

	const SwapchainSupportDetails details = query_swap_chain_support(physical_device);
	unsigned int minImageCount = std::max(3u, details.capabilities.minImageCount);
	if(details.capabilities.maxImageCount > 0 && minImageCount > details.capabilities.maxImageCount)
		minImageCount = details.capabilities.maxImageCount;

	uint32_t imageCount = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
		imageCount = details.capabilities.maxImageCount;
	}

	unsigned int queue_family_indices[] = {graphics_queue_index, present_queue_index};
	unsigned int queue_family_count = (graphics_queue_index != present_queue_index ? 2 : 0);

	vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(details.formats);
	vk::PresentModeKHR present_mode = choose_swap_present_mode(details.presentModes);

	wnd::print(std::string("# of images: ") + std::to_string(imageCount));
	wnd::print(std::string("Format: ") + to_string(surface_format.format));
	wnd::print(std::string("Color space: ") + to_string(surface_format.colorSpace));
	wnd::print(std::string("Present mode: ") + to_string(present_mode));

	vk::Extent2D swap_extent = choose_swap_extent(details.capabilities, window);
	vk::SwapchainCreateInfoKHR swapchain_create_info = {
		{},
		display_surface,
		minImageCount,
		surface_format.format,
		surface_format.colorSpace,
		swap_extent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		queue_family_count ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
		queue_family_count,
		queue_family_count ? queue_family_indices : nullptr,
		details.capabilities.currentTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		present_mode,
		true,
		nullptr
	};

	swapchain = raii::SwapchainKHR{device, swapchain_create_info};
	swapchain_images = swapchain.getImages();

	format = surface_format.format;
	extent = swap_extent;

	wnd::print();
}



void Renderer::create_image_views() {
	wnd::begin_section("Image views: ");

	image_views.clear();
	vk::ImageViewCreateInfo image_view_create_info = {
		{},
		{},
		vk::ImageViewType::e2D,
		format,
		vk::ComponentMapping{
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity
		},
		vk::ImageSubresourceRange{
			vk::ImageAspectFlagBits::eColor,
			0,
			1,
			0,
			1
		}
	};

	int i = 0;

	for (const auto &image : swapchain_images) {
		wnd::begin_frame(std::to_string(i++));
		image_view_create_info.image = image;

		wnd::print(std::string("Format: ") + to_string(image_view_create_info.format));

		image_views.emplace_back(device, image_view_create_info);

		wnd::end_frame();
	}

	wnd::print();
}



std::vector<char> Renderer::readFile(const std::string& filename) {
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



raii::ShaderModule Renderer::create_shader_module(std::vector<char> code) const {
	const vk::ShaderModuleCreateInfo create_info = {
		{},
		code.size() * sizeof(char),
		reinterpret_cast<const uint32_t *>(code.data())
	};

	return raii::ShaderModule{device, create_info};
}



void Renderer::create_graphics_pipeline() {
	wnd::begin_section("Graphics pipeline: ");
	wnd::begin_frame("Shader.spv");
	const std::vector<char> shader_code = readFile("shader.spv");
	wnd::print(std::string("Buffer size: ") + std::to_string(shader_code.size()));
	wnd::end_frame();

	raii::ShaderModule shader_module = create_shader_module(shader_code);

	vk::PipelineShaderStageCreateInfo vertex_stage_create_info = {
		{},
		vk::ShaderStageFlagBits::eVertex,
		shader_module,
		"vertMain",
		nullptr
	};

	vk::PipelineShaderStageCreateInfo fragment_stage_create_info = {
		{},
		vk::ShaderStageFlagBits::eFragment,
		shader_module,
		"fragMain",
		nullptr
	};

	std::vector shader_stages = {vertex_stage_create_info, fragment_stage_create_info};

	std::vector dynamic_states = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};

	vk::PipelineDynamicStateCreateInfo dynamic_state_create_info = {
		{},
		static_cast<uint32_t>(dynamic_states.size()),
		dynamic_states.data()
	};

	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};

	vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
		{},
		vk::PrimitiveTopology::eTriangleList,
		false,
	};

	vk::PipelineViewportStateCreateInfo viewport_state = {
		{},
		1,
		nullptr,
		1,
		nullptr,
	};

	vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info = {
		{},
		false,
		false,
		vk::PolygonMode::eFill,
		vk::CullModeFlagBits::eBack,
		vk::FrontFace::eClockwise,
		false,
		0.0f,
		0.0f,
		1.0f,
		1.0f,
	};

	vk::PipelineMultisampleStateCreateInfo multisample_state_create_info = {
		{},
		vk::SampleCountFlagBits::e1,
		false,
		0.0f,
		nullptr,
		false,
		false
	};

	vk::PipelineColorBlendAttachmentState blend_attachment_state = {
		false,
		vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha,
		vk::BlendOp::eAdd,
		vk::BlendFactor::eOne,
		vk::BlendFactor::eZero,
		vk::BlendOp::eAdd,
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA,
	};

	std::vector blend_attachments = {
		blend_attachment_state
	};

	vk::PipelineColorBlendStateCreateInfo blend_state_create_info = {
		{},
		false,
		vk::LogicOp::eCopy,
		static_cast<uint32_t>(blend_attachments.size()),
		blend_attachments.data(),
		{0.0, 0.0, 0.0, 0.0}
	};

	vk::PipelineLayoutCreateInfo pipeline_layout_create_info = {
		{},
		0,
		nullptr,
		0,
		nullptr
	};

	pipeline_layout = raii::PipelineLayout{
		device,
		pipeline_layout_create_info
	};

	vk::PipelineRenderingCreateInfo rendering_create_info = {
		{},
		1,
		&format,
		vk::Format::eUndefined,
		vk::Format::eUndefined,
	};

	vk::GraphicsPipelineCreateInfo pipeline_create_info = {
		{},
		static_cast<uint32_t>(shader_stages.size()),
		shader_stages.data(),
		&vertex_input_state_create_info,
		&input_assembly_create_info,
		nullptr,
		&viewport_state,
		&rasterization_state_create_info,
		&multisample_state_create_info,
		nullptr,
		&blend_state_create_info,
		&dynamic_state_create_info,
		pipeline_layout,
		nullptr,
		0,
		nullptr,
		-1,
		&rendering_create_info
	};

	graphics_pipeline = raii::Pipeline{
		device,
		nullptr,
		pipeline_create_info
	};

	wnd::print();
}



void Renderer::create_command_pool() {
	vk::CommandPoolCreateInfo pool_create_info = {
		{vk::CommandPoolCreateFlagBits::eResetCommandBuffer},
		graphics_queue_index
	};

	command_pool = raii::CommandPool{
		device,
		pool_create_info
	};
}



void Renderer::create_command_buffer() {
	vk::CommandBufferAllocateInfo command_buffer_allocate_info = {
		command_pool,
		vk::CommandBufferLevel::ePrimary,
		1
	};

	raii::CommandBuffers command_buffers = {
		device,
		command_buffer_allocate_info
	};

	command_buffer = std::move(command_buffers.front());
}



void Renderer::transition_image_layout(
    uint32_t imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout,
    vk::AccessFlags2 srcAccessMask,
    vk::AccessFlags2 dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask
) {
	vk::ImageMemoryBarrier2 barrier = {
		srcStageMask,
		srcAccessMask,
		dstStageMask,
		dstAccessMask,
		oldLayout,
		newLayout,
		vk::QueueFamilyIgnored,
		vk::QueueFamilyIgnored,
		swapchain_images[imageIndex],
		vk::ImageSubresourceRange{
			vk::ImageAspectFlagBits::eColor,
			0,
			1,
			0,
			1
		    }
	};

	vk::DependencyInfo dependencyInfo = {
		{},
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier
	};

	command_buffer.pipelineBarrier2(dependencyInfo);
}



void Renderer::record_command_buffer(const unsigned int& index) {
	command_buffer.begin({});

	transition_image_layout(
		index,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::AccessFlagBits2::eNone,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::PipelineStageFlagBits2::eTopOfPipe,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput
	);

	constexpr vk::ClearValue clear_color = vk::ClearColorValue(0.2f, 0.4f, 0.8f, 1.0f);

	vk::RenderingAttachmentInfo attachment_info = {
		image_views[index],
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ResolveModeFlagBits::eNone,
		nullptr,
		vk::ImageLayout::eUndefined,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		clear_color
	};

	vk::RenderingInfo rendering_info = {
		{},
		{{0, 0}, extent},
		1,
		0,
		1,
		&attachment_info,
		nullptr,
		nullptr
	};

	command_buffer.beginRendering(rendering_info);
	command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphics_pipeline);
	command_buffer.setViewport(0, vk::Viewport(
		0.0f, 0.0f,
		static_cast<float>(extent.width), static_cast<float>(extent.height),
		0.0f,1.0f));
	command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
	command_buffer.draw(3, 1, 0, 0);
	command_buffer.endRendering();
	transition_image_layout(
		index,
		vk::ImageLayout::eColorAttachmentOptimal,
		vk::ImageLayout::ePresentSrcKHR,
		vk::AccessFlagBits2::eColorAttachmentWrite,
		vk::AccessFlagBits2::eNone,
		vk::PipelineStageFlagBits2::eColorAttachmentOutput,
		vk::PipelineStageFlagBits2::eBottomOfPipe
		);

	command_buffer.end();
}



void Renderer::create_sync_objects() {
	present_complete_semaphore = raii::Semaphore{ device, vk::SemaphoreCreateInfo{}};
	render_finished_semaphore = raii::Semaphore{ device, vk::SemaphoreCreateInfo{}};
	draw_fence = raii::Fence{ device, {{vk::FenceCreateFlagBits::eSignaled}}};
}



Renderer::Renderer() {
	std::cout << "\n\n\n";

	create_window();
	context = {}; // Setup context :)
	create_vulkan_instance();
	create_display_surface();
	choose_physical_device();
	get_queue_indices();
	create_logical_device();
	create_swapchain();
	create_image_views();

	create_graphics_pipeline();
	create_command_pool();
	create_command_buffer();
	create_sync_objects();

	std::cout << "\n\n\n";
}



Renderer::~Renderer() {
	swapchain.clear();
	glfwDestroyWindow(window);
	glfwTerminate();
}



void Renderer::draw_frame() {
	auto [result, imageIndex] = swapchain.acquireNextImage(
		UINT64_MAX,
		*present_complete_semaphore,
		nullptr);

	record_command_buffer(imageIndex);
	device.resetFences(*draw_fence);

	vk::PipelineStageFlags wait_destination_stage_mask = {
		vk::PipelineStageFlagBits::eColorAttachmentOutput
	};

	vk::SubmitInfo submit_info = {
		*present_complete_semaphore,
		wait_destination_stage_mask,
		*command_buffer,
		*render_finished_semaphore
	};

	graphics_queue.submit(submit_info, draw_fence);

	while (device.waitForFences(*draw_fence, true, UINT64_MAX) == vk::Result::eTimeout) {}

	const vk::PresentInfoKHR presentInfoKHR = {
		*render_finished_semaphore,
		*swapchain,
		imageIndex
	};

	result = present_queue.presentKHR(presentInfoKHR);

	//glfwSwapBuffers(window);
}


void Renderer::main_loop() {
	if (NO_FRAMES) return;

	unsigned short i = 0;
	unsigned long long frame_time = 0;
	constexpr unsigned short max_i = 1'000;

	while (!glfwWindowShouldClose(window)) {
		auto begin = ch::high_resolution_clock::now();

		glfwPollEvents();
		draw_frame();

		auto end = ch::high_resolution_clock::now();

		frame_time += ch::duration_cast<ch::microseconds>(end - begin).count();

		if (i++ >= max_i) {
			const unsigned long long arg_frame_time = frame_time / max_i;
			std::cout << "Time : ";
			std::cout << arg_frame_time * 0.00'1;
			std::cout << "\tms\t; FPS: ";
			std::cout << 1'000'000.0 / arg_frame_time << "\n";
			frame_time = 0;
			i = 0;
		}
	}

	device.waitIdle();
}
