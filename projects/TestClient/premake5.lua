-----------------------
-- [ PROJECT CONFIG] --
-----------------------
project "TestClient"
	architecture  "x86_64"
	language      "C++"
	cppdialect    "C++20"
	staticruntime "On"
	kind          "ConsoleApp"
	
--	targetdir ("%{wks.location}/bin/"   .. outputdir .. "/%{string.lower(prj.name)}")
	targetdir ("%{wks.location}/bin/"   .. outputdir)
	objdir    ("%{wks.location}/build/" .. outputdir .. "/%{string.lower(prj.name)}")
	
--	pchheader "pch.h"
--	pchsource "source/pch.cpp"


	includedirs {
		"source",
		"%{wks.location}/projects/LibNet/source",
		"%{wks.location}/vendor/asio-1.24.0/include",
	}
	
	
	links {
		"LibNet"
	}


	files {
		"source/**.h",
		"source/**.cpp"
	}


	filter "configurations:Debug"
	
		defines {
		}
	

	filter "configurations:Release"
		
		defines {
		}


	filter {}
