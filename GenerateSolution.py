import subprocess
import sys
import os

sys.path.insert(0, 'DgLib\\3rdParty\\BuildScripts')

import DgBuild

def CreatePluginPremakeFile(rootDir, fileName):
    folders = []
    for file in os.listdir(rootDir):
        d = os.path.join(rootDir, file)
        if os.path.isdir(d):
            folders.append(file)

    with open(fileName, "w") as file:
        for pluginName in folders:
            file.write(f"include(\"{rootDir}/{pluginName}/premake-{pluginName}.lua\")\n")

def CreatePluginDirs(rootDir):
    if not os.path.exists("XornApp/Plugins"):
        os.makedirs("XornApp/Plugins")
    
    folders = []
    for file in os.listdir(rootDir):
        d = os.path.join(rootDir, file)
        if os.path.isdir(d):
            folders.append(file)
            
    for name in folders:
        pluginPath = f"XornApp/Plugins/{name}"
        if not os.path.exists(pluginPath):
            os.makedirs(pluginPath)

CreatePluginPremakeFile("Samples", "premake-Samples.lua")
CreatePluginPremakeFile("XornPlugins", "premake-XornPlugins.lua")
CreatePluginDirs("Samples")
CreatePluginDirs("XornPlugins")

DgBuild.Make_vpaths("DgLib/src/", "DgLib/DgLib_vpaths.lua")
subprocess.call("DgLib/3rdParty/premake/premake5.exe vs2022")
