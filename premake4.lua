
solution "Cube"

	location "build"
	targetdir "build"
        platforms { "x64" }
	configurations {"Debug", "Release", "Ship"}
	
	configuration {"Release or Ship"}
		flags {"Optimize"}
	
	configuration { }
	
	flags { "Symbols" }

	dofile "ext/putki/runtime.lua"

project "cube-server"

        kind "ConsoleApp"
        language "c#"
        targetname "cube-server"

        putki_typedefs_runtime_csharp("src/types", true)

        files { "src/server/**.cs" }
