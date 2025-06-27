#include "Renderer.h"

#include "text_formatting.h"

namespace raii = vk::raii;

constexpr std::vector<std::string> own_instance_extensions{
	// Extension names
};

constexpr std::vector<std::string> own_instance_layers{
	// Layer names
};

const std::vector<const char *> device_extensions = {
	vk::KHRSwapchainExtensionName
};



constexpr unsigned int WIDTH = 800;
constexpr unsigned int HEIGHT = 600;



bool has_extensions(const raii::PhysicalDevice &device) {
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
	vk::PhysicalDeviceProperties properties = device.getProperties();
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

	const SwapchainSupportDetails swap_chain_support = query_swap_chain_support(device);

	if (swap_chain_support.formats.empty()) return INT16_MIN;
	if (swap_chain_support.presentModes.empty()) return INT16_MIN;

	return score;
}



void Renderer::choose_physical_device() {
	wnd::begin_section("Physical device: ");

	std::vector<raii::PhysicalDevice> physical_devices = instance.enumeratePhysicalDevices();

	std::multimap<short, raii::PhysicalDevice &> ranked_devices;

	for (raii::PhysicalDevice& device: physical_devices) {
		short score = rank_score(device);
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
}



void Renderer::create_display_surface() {
	VkSurfaceKHR surface_khr;
	glfwCreateWindowSurface(*instance, window, nullptr, &surface_khr);
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


	instance = raii::Instance{
		context,
		vk::InstanceCreateInfo{
			{},
			new vk::ApplicationInfo{
				"LavaChicken on Vulkan - Test App",
				vk::makeApiVersion(1, 0, 0, 0),
				"LavaChicken",
				vk::makeApiVersion(1, 0, 0, 0),
				vk::makeApiVersion(1, 0, 0, 0)
			},
			static_cast<unsigned int>(layers_vector.size()),
			layers_vector.data(),
			static_cast<unsigned int>(extensions_vector.size()),
			extensions_vector.data()
		}
	};
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



Renderer::Renderer(): instance(nullptr), display_surface(nullptr), physical_device(nullptr) {
	std::cout << "\n\n\n";

	create_window();
	context = {}; // Setup context :)
	create_vulkan_instance();
	create_display_surface();
	choose_physical_device();

	std::cout << "\n\n\n";
}



Renderer::~Renderer() {

}

void Renderer::main_loop() {

}
