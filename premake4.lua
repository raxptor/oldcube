solution "Turbonet"

flags { "Symbols" }
location "build"
targetdir "build"

configurations {"Debug"}

dofile("ext/putki/runtime.lua")
	
project "turbonet"
	kind "ConsoleApp"
	language "c++"
	files {"src/*.cpp", "src/**.h"}
	links { "uv" }
	
	putki_use_runtime_lib()
	putki_typedefs_runtime("src/types", true)
