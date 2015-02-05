solution "Capn"
	configurations { "Debug", "Release" }
	
	configuration "Debug"
		targetdir "build/bin/debug"
		
	configuration "Release"
		targetdir "build/bin/release"

project "Hook"
	kind "StaticLib"
	language "C++"
	files { "code/hook/**.cpp", "code/hook/**.hpp" }
	targetname "hook"
	
	configuration "Debug"
		flags { "Symbols", "ExtraWarnings" }
		objdir "build/obj/hook/debug"
	
	configuration "Release"
		flags { "ExtraWarnings", "Optimize" }
		objdir "build/obj/hook/release"
	
project "Inject"
	kind "ConsoleApp"
	language "C++"
	files { "code/inject/**.cpp", "code/inject/**.hpp" }
	targetname "inject"
	
	configuration "Debug"
		flags { "Symbols", "ExtraWarnings" }
		objdir "build/obj/inject/debug"
	
	configuration "Release"
		flags { "ExtraWarnings", "Optimize" }
		objdir "build/obj/inject/release"

project "Example"
	kind "SharedLib"
	language "C++"
	includedirs { "code/hook/" }
	files { "code/example/**.cpp", "code/example/**.hpp" }
	links { "Hook", "opengl32" }
	targetname "example"
	
	configuration "Debug"
		flags { "Symbols", "ExtraWarnings" }
		objdir "build/obj/example/debug"
	
	configuration "Release"
		flags { "ExtraWarnings", "Optimize" }
		objdir "build/obj/example/release"