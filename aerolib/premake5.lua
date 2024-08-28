project "aerolib"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
   
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("..build/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.cpp",
    }
    
    includedirs
    {  
        "src", 
    }

    links
    {
    }
    
    defines
    {
        "_USE_MATH_DEFINES",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_DEPRECATE",
    }


    filter "system:windows"
        systemversion "latest"
        defines "PLATFORM_WINDOWS"
        links { "ws2_32" }

    filter  "configurations:Debug" 
        defines { "DEBUG" }
        symbols "On"

    filter  "configurations:Release"
        defines { "RELEASE" }
        optimize "Full"

