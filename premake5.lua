workspace "Raytracing"
    architecture "x86_64"
    configurations { "Debug", "Release" }
    startproject "Sandbox"
    location(_ACTION)

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG" }
        symbols "On"
        optimize "On"
        
    filter "system:windows"
        systemversion "latest"

project "Glad"
    location(_ACTION)
    kind "StaticLib"
    language "C"
    
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { "vendor/glad/include" }
    files { "vendor/glad/src/**.c" }

project "GLFW" 
    location(_ACTION)
    kind "StaticLib"
    language "C"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    defines { "GLFW_STATIC" }

    includedirs { 
        "vendor/glfw/src"
    }
    files { 
        "vendor/glfw/src/context.c",
        "vendor/glfw/src/init.c",
        "vendor/glfw/src/input.c",
        "vendor/glfw/src/monitor.c",
        "vendor/glfw/src/window.c",
        "vendor/glfw/src/xkb_unicode.c",
        "vendor/glfw/src/vulkan.c",
    }

    filter "system:windows" 
        defines { "_GLFW_WIN32", "_CRT_SECURE_NO_WARNINGS" }
        files {
			"vendor/glfw/src/win32_init.c",
			"vendor/glfw/src/win32_joystick.c",
			"vendor/glfw/src/win32_monitor.c",
			"vendor/glfw/src/win32_time.c",
			"vendor/glfw/src/win32_thread.c",
			"vendor/glfw/src/win32_window.c",
			"vendor/glfw/src/wgl_context.c",
			"vendor/glfw/src/egl_context.c",
			"vendor/glfw/src/osmesa_context.c"
        }

    filter "system:linux"
        defines { "_GLFW_X11 " }
        files {
			"vendor/glfw/src/x11_init.c",
			"vendor/glfw/src/x11_monitor.c",
			"vendor/glfw/src/x11_window.c",
			"vendor/glfw/src/xkb_unicode.c",
			"vendor/glfw/src/posix_time.c",
			"vendor/glfw/src/posix_thread.c",
			"vendor/glfw/src/glx_context.c",
			"vendor/glfw/src/egl_context.c",
			"vendor/glfw/src/osmesa_context.c",
			"vendor/glfw/src/linux_joystick.c"
        }
    
project "ImGui"
    location(_ACTION)
    kind "StaticLib"
    language "C++"
    
    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { 
        "vendor/imgui", 
        "vendor/imgui/backends",
        "vendor/glfw/include",
        "vendor/glad/include" 
    }

    defines { "IMGUI_IMPL_OPENGL_LOADER_GLAD" }
    
    files { 
        "vendor/imgui/*.cpp",
        "vendor/imgui/backends/imgui_impl_glfw.cpp",
        "vendor/imgui/backends/imgui_impl_opengl3.cpp",
    }

project "Raytracing"
    location(_ACTION)
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { 
        "vendor/glm",
        "vendor/glad/include",
        "vendor/glfw/include",
        "vendor/spdlog/include",
        "vendor/stb/include",
    }

    files { "src/Raytracing/**.cpp", "src/Raytracing/**.h"  }

project "RaytracingUtility"
    location(_ACTION)
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { 
        "vendor/glm",
        "vendor/spdlog/include",
        "vendor/json",
        "src/Raytracing"
    }

    links { "Raytracing" }

    files { "src/RaytracingUtility/**.cpp", "src/RaytracingUtility/**.h"  } 

project "CLI"
    location(_ACTION)
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { 
        "vendor/glm",
        "vendor/spdlog/include",
        "vendor/stb/include",
        "vendor/json",
        "src/Raytracing",
        "src/RaytracingUtility"
    }

    files { "src/CLI/**.cpp", "src/CLI/**.h"  }

    links { "Raytracing", "RaytracingUtility" }

    postbuildcommands {
        "{COPY} ../src/res ../bin/%{cfg.buildcfg}/%{prj.name}/res"
    }

    filter "system:linux"
        links { "pthread" }

project "Sandbox"
    location(_ACTION)
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs { 
        "vendor/glm",
        "vendor/glad/include",
        "vendor/glfw/include",
        "vendor/spdlog/include",
        "vendor/stb/include",
        "vendor/imgui",
        "vendor/imgui/backends",
        "vendor/json",
        "src/Raytracing",
        "src/RaytracingUtility"
    }

    files { "src/Sandbox/**.cpp", "src/Sandbox/**.h"  }

    links { "Glad", "GLFW", "ImGui", "Raytracing", "RaytracingUtility" }

    filter "system:windows"
        links { "opengl32" }
    
    filter "system:linux"
        links { "gl" }

    postbuildcommands {
        "{COPY} ../src/res ../bin/%{cfg.buildcfg}/%{prj.name}/res"
    }
