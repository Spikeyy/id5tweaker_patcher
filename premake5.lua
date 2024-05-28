-- Option to allow copying the DLL file to a custom folder after build
newoption {
	trigger = "copy-to",
	description = "Optional, copy the DLL to a custom folder after build, define the path here if wanted.",
	value = "PATH"
}

workspace "id5tweaker_patcher"
	configurations { "Debug", "Release" }
	location "./build"
	objdir "%{wks.location}/obj/%{cfg.buildcfg}"
	targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
	startproject "id5tweaker_patcher"

	filter "configurations:Debug"
		defines { "_DEBUG", "DEBUG" }
		symbols "On"
		runtime "debug"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
		runtime "release"


project "id5tweaker_patcher"
	kind "SharedLib"
	language "C++"
	cppdialect "C++20"
	architecture "x86_64"

	targetname "dinput8"
	
	-- Pre-compiled header
	pchsource "src/pch.cpp"
	pchheader "pch.h"

	characterset "MBCS"
	staticruntime "on"

	files {
		"./src/**.cpp",
		"./src/**.hpp",
		"./src/**.h"
	}
	vpaths {
		["Header Files/*"] = {"./src/**.h", "./src/**.hpp"},
		["Source Files/*"] = {"./src/**.cpp"}
	}

	-- Build events
	if _OPTIONS["copy-to"] then
		postbuildcommands {
			"copy /Y \"$(TargetPath)\" \"" .. _OPTIONS["copy-to"] .. "\""
		}
	end