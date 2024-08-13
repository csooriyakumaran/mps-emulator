-- premake5.lua
workspace "mps-emulator"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "mps-server"
    
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "core"
group ""

group "dependencies" 
group ""

group "Application"
    include "mps-server"
group ""

newaction {
    trigger = "clean",
    description = "Cleaning build",
    execute = function ()
        print("Removing binaries")
        os.remove("./bin/**.lib")
        os.remove("./bin/**.exe")
        os.remove("./bin/**.pdb")
        print("Removing object files")
        os.remove("./build/**.h")
        os.remove("./build/**.h.gch")
        os.remove("./build/**.d")
        os.remove("./build/**.o")
        os.remove("./build/**.Build.CppClean.log")
        print("Removing project files")
        os.rmdir("./.vs")
        os.remove("**.sln")
        os.remove("**.vcxproj")
        os.remove("**.vcxproj.filters")
        os.remove("**.vcxproj.user")
        os.remove("**.vcxproj.FileListAbsolute.txt")
        os.remove("**.make")
        os.remove("**Makefile")
        print("Done")
    end
}
