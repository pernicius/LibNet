-----------------------
-- [ PROJECT CONFIG] --
-----------------------
project "LibNet"
	architecture  "x86_64"
	language      "C++"
	cppdialect    "C++20"
	staticruntime "On"
	kind          "StaticLib"
	
--	targetdir ("%{wks.location}/bin/"   .. outputdir .. "/%{string.lower(prj.name)}")
	targetdir ("%{wks.location}/bin/"   .. outputdir)
	objdir    ("%{wks.location}/build/" .. outputdir .. "/%{string.lower(prj.name)}")
	
	includedirs {
		"source",
		"%{wks.location}/vendor/asio-1.24.0/include",
	}


	files {
		"source/**.h",
		"dummy.cpp"
	}
