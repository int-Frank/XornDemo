workspace "XornDemo"
  architecture "x64"
  
  configurations
  {
    "Debug",
    "Release"
  }

  vcpkgPackageDir = "%{wks.location}/../vcpkg/packages"

  include("XornCore/premake-XornCore.lua")
  include("DgLib/premake-proj-DgLib.lua")
  include("XornApp/premake-XornApp.lua")
  group("Plugins")
	include("premake-Samples.lua")
	include("premake-XornPlugins.lua")
  group("")