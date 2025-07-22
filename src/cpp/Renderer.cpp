#include "Renderer.h"

#include <bitset>

#include "text_formatting.h"
#include <fontconfig/fontconfig.h>

namespace raii = vk::raii;



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
	vk::PhysicalDeviceFeatures physical_device_features;

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
			false,
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
	graphics_queue_index = 0; // Reuse
	if (graphics_queue_index != present_queue_index) {
		present_queue = raii::Queue{device, present_queue_index, 1};
		present_queue_index = 1; // Reuse
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

	std::cout << "\n\n\n";
}



Renderer::~Renderer() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::main_loop() {

}
