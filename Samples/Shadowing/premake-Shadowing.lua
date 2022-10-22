
project "Shadowing"
  location ""
  kind "SharedLib"
  targetdir ("%{wks.location}/build/%{prj.name}-%{cfg.buildcfg}")
  objdir ("%{wks.location}/build/intermediate/%{prj.name}-%{cfg.buildcfg}")
  systemversion "latest"
  language "C++"
  cppdialect "C++17"
    
  files 
  {
    "src/**.h",
    "src/**.cpp"
  }
    
  includedirs
  {
    "src",
    "%{wks.location}/XornCore/src",
    "%{wks.location}/DgLib/src"
  }
  
  links
  {
    "DgLib",
	"XornCOre"
  }
  
  postbuildcommands 
  {
    "{COPY} %{wks.location}/build/%{prj.name}-%{cfg.buildcfg}/Shadowing.dll %{wks.location}/XornApp/Plugins/Shadowing"
  }

  filter "configurations:Debug"
    runtime "Debug"
    symbols "on"

  filter "configurations:Release"
    runtime "Release"
    optimize "on"