workspace "Ying-Long Engine"
	architecture "x64"

	configurations
	{
		"Debug", 
		"Release",
		"Distribution"
	}

	flags
	{
		"MultiProcessorCompile"
	}
	
project "Ying-Long Engine"
	location "Ying-Long Engine"
	kind "ConsoleApp"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	files
	{
		"%{prj.name}/CodeFile/**.h",
		"%{prj.name}/CodeFile/**.cpp",
		"%{prj.name}/Ying-Long Engine.rc",
		"%{prj.name}/resource.h",
	}

	libdirs
	{
		"Setup/DirectX 11/%{cfg.architecture}/%{cfg.buildcfg}/",
		"Setup/DirectXTex/Libraries/%{cfg.buildcfg}/",
		"Setup/Assimp/Libraries/",
		"Setup/PhysX/Libraries/",
	}

	includedirs
	{
		"ImGui/CodeFile/",
		"yaml-cpp/include/",
		"Setup/DirectXTex/Includes/",
		"Setup/Assimp/Includes/",
		"Setup/PhysX/Includes/",
	}

	links
		{
			"ImGui",
			"DirectXTex",
			"Time",

			"yaml-cpp",

			"d3d12",
			"d3d11",
			"d3dcompiler",
			"dxgi",
			"DirectXTK",

		"assimp-vc143-mtd",

		"LowLevel_static_64",
        "LowLevelAABB_static_64",
        "LowLevelDynamics_static_64",
        "PhysX_64",
        "PhysXCharacterKinematic_static_64",
        "PhysXCommon_64",
        "PhysXCooking_64",
        "PhysXExtensions_static_64",
        "PhysXFoundation_64",
        "PhysXPvdSDK_static_64",
        "PhysXTask_static_64",
        "PhysXVehicle_static_64",
        "PhysXVehicle2_static_64",
        "PVDRuntime_64"
	}

	defines
	{
		"YAML_CPP_STATIC_DEFINE"
	}

	filter "system:Windows"
		cppdialect "C++20"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"

	filter "configurations:Distribution"
		optimize "On"

project "yaml-cpp"
	location "yaml-cpp"
	kind "StaticLib"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"%{prj.name}/include/"
	}

	defines
	{
		"YAML_CPP_STATIC_DEFINE"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"
		project "yaml-cpp"
	location "yaml-cpp"
	kind "StaticLib"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"

project "ImGui"
	location "ImGui"
	kind "StaticLib"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	files
	{
		"%{prj.name}/CodeFile/**.h",
		"%{prj.name}/CodeFile/**.cpp",
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"

project "Time"
	location "Time"
	kind "StaticLib"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	files
	{
		"%{prj.name}/CodeFile/**.h",
		"%{prj.name}/CodeFile/**.cpp",
	}

	filter "system:windows"
		cppdialect "C++20"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"

project "entt-master"
	location "entt-master"
	kind "StaticLib"
	language "C++"

	targetdir ("Build/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")
	objdir ("%{prj.name}/Intermediate/%{cfg.buildcfg} - %{cfg.system}/%{cfg.architecture}")

	files
	{
		"%{prj.name}/src/entt/**.h",
		"%{prj.name}/src/entt/**.cpp",
	}

	includedirs
	{
		"%{prj.name}/include/"
	}

	defines
	{

	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "Off"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"

	filter "configurations:Release"
		runtime "Release"