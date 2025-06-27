#include "Renderer.h"

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

	const SwapchainSupportDetails swap_chain_support = query_swap_chain_support(device);

	if (swap_chain_support.formats.empty()) return INT16_MIN;
	if (swap_chain_support.presentModes.empty()) return INT16_MIN;

	return score;
}



void Renderer::choose_physical_device() {
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

	physical_device = std::move(ranked_devices.begin()->second);
}



void Renderer::create_display_surface() {
	VkSurfaceKHR surface_khr;
	glfwCreateWindowSurface(*instance, window, nullptr, &surface_khr);
	display_surface = raii::SurfaceKHR{instance, surface_khr};
}



void Renderer::create_vulkan_instance() {
	std::cout << "Creating Vulkan instance...\n";

	unsigned int glfwExtensionCount = 0;
	const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	const char **glfwExtensionsCopy = glfwExtensions;

	std::unordered_set<const char *> extensions;

	std::cout << "/- GLFW required extensions: ---\n";
	for (int i = 0; i < glfwExtensionCount; i++) {
		const char *glfwExtensionsPointer = *glfwExtensionsCopy;
		extensions.insert(glfwExtensionsPointer);
		std::cout << "| + " << glfwExtensionsPointer << "\n";
		glfwExtensionsCopy++;
	}
	std::cout << "\\-------------------------------\n\n";

	std::cout << "/- LavaChicken required extensions: ---\n";
	for (const std::string &extension: own_instance_extensions) {
		const char *name = extension.c_str();
		std::cout << (extensions.insert(name).second ? "| + " : "| ~ ");
		std::cout << extension << "\n";
	}
	std::cout << "\\--------------------------------------\n\n";
	const std::vector<const char *> extensions_vector = {extensions.begin(), extensions.end()};


	std::unordered_set<const char *> layers;

	std::cout << "/- LavaChicken required layers: ---\n";
	for (const std::string &layer: own_instance_layers) {
		const char *name = layer.c_str();
		std::cout << (extensions.insert(name).second ? "| + " : "| ~ ");
		std::cout << layer << "\n";
	}
	std::cout << "\\----------------------------------\n\n";
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
}



Renderer::Renderer(): instance(nullptr), display_surface(nullptr), physical_device(nullptr) {
	create_window();
	context = {}; // Setup context :)
	create_vulkan_instance();
	create_display_surface();
	choose_physical_device();

	vk::PhysicalDeviceProperties device_properties = physical_device.getProperties();
	std::cout << "Chosen device: " << device_properties.deviceName
		  << " (Type: " << static_cast<int>(device_properties.deviceType) << ")" << std::endl;
}



Renderer::~Renderer() {

}

void Renderer::main_loop() {

}
