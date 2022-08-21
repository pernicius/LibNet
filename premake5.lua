---------------------------------
-- [ WORKSPACE CONFIGURATION ] --
---------------------------------
workspace "LibNet"

	startproject "TestServer"

	configurations {
		"Debug",
		"Release"
	}

	platforms {
		"Windows",
		"Linux",
		"MacOS"
	}

	--------------------------------
	-- [ PLATFORM CONFIGURATION ] --
	--------------------------------
	filter "platforms:Windows"
		system  "windows"
		defines "HE_SYSTEM_WINDOWS"

	filter "platforms:Linux"
		system  "linux"
		defines "HE_SYSTEM_LINUX"

	filter "platforms:MacOS"
		system "macosx"
		defines "HE_SYSTEM_MACOS"

	filter { "system:windows", "action:vs*" }
		flags { "MultiProcessorCompile", "NoMinimalRebuild" }
		systemversion "latest"

	-------------------------------------
	-- [ DEBUG/RELEASE CONFIGURATION ] --
	-------------------------------------
	filter "configurations:Debug"
		defines  "HE_BUILD_DEBUG"
		symbols  "On"
		optimize "Off"
		runtime  "Debug"

	filter "configurations:Release"
		defines  "HE_BUILD_RELEASE"
		symbols  "Off"
		optimize "Speed"
		runtime  "Release"

	filter {}

	-------------------------------
	-- [ PROJECT CONFIGURATION ] --
	-------------------------------

	outputdir = "%{string.lower(cfg.platform)}-%{string.lower(cfg.buildcfg)}"

	group "LibNet"
		include("projects/LibNet")
		
	group "Examples"
		include("projects/TestServer")
		include("projects/TestClient")

	group "Misc"
		include("vendor/premake5")
--		include("docu")

	group ""

	---------------------------
	-- [ WORKSPACE CLEANUP ] --
	---------------------------

	if _ACTION == "cleanup" then
		os.rmdir("bin")
		os.rmdir("build")
	end
