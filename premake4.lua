solution "Turbonet"

flags { "Symbols" }
location "build"
targetdir "build"

configurations {"Debug"}
	
project "turbonet"
	kind "ConsoleApp"
	language "c"
	files {"src/*.cpp", "src/**.h"}
	links { "uv" }
