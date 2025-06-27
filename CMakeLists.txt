cmake_minimum_required(VERSION 3.31)
project(LavaChicken)

set(CMAKE_CXX_STANDARD 26)


# find Vulkan SDK
find_package( Vulkan REQUIRED )
find_package( glfw3 REQUIRED )
find_package( glm CONFIG REQUIRED )

# Require Vulkan version ≥ 1.3.256 (earliest version when the Vulkan module was available)
if( ${Vulkan_VERSION} VERSION_LESS "1.3.256" )
    message( FATAL_ERROR "Minimum required Vulkan version for C++ modules is 1.3.256. "
            "Found ${Vulkan_VERSION}."
    )
endif()

# set up Vulkan C++ module as a library
add_library( VulkanHppModule )
target_sources( VulkanHppModule PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)
target_compile_definitions( VulkanHppModule PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
)
target_compile_features( VulkanHppModule PUBLIC cxx_std_20 )
target_link_libraries( VulkanHppModule PUBLIC Vulkan::Vulkan )
#target_link_libraries( VulkanHppModule PUBLIC Vulkan::Headers )

add_custom_target( Shaders
        COMMAND "${PROJECT_SOURCE_DIR}/compileShaders.sh" "${PROJECT_SOURCE_DIR}"
)

# link Vulkan C++ module into your project
add_executable( LavaChicken src/cpp/main.cpp
        src/cpp/Renderer.cpp
        src/cpp/Renderer.h)
target_link_libraries( LavaChicken PRIVATE VulkanHppModule glfw glm::glm )
add_dependencies( LavaChicken Shaders)