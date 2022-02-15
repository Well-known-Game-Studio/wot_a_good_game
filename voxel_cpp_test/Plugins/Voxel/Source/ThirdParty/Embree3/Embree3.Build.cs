// Copyright 2020 Phyronnaz

using System.IO;
using UnrealBuildTool;

public class Embree3 : ModuleRules
{
    public Embree3(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

#if true
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Win64", "include"));

            PublicDelayLoadDLLs.Add("embree3.dll");
            PublicDelayLoadDLLs.Add("tbb.dll");
            PublicDelayLoadDLLs.Add("tbbmalloc.dll");

            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Win64", "lib", "embree3.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Win64", "lib", "tbb.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "Win64", "lib", "tbbmalloc.lib"));

            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/Win64/lib/embree3.dll");
            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/Win64/lib/tbb.dll");
            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/Win64/lib/tbbmalloc.dll");

            PublicDefinitions.Add("USE_EMBREE_VOXEL=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "MacOSX", "include"));

            PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "MacOSX", "lib", "libembree3.3.dylib"));
            PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "MacOSX", "lib", "libtbb.dylib"));
            PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "MacOSX", "lib", "libtbbmalloc.dylib"));

            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/MacOSX/lib/libembree3.3.dylib");
            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/MacOSX/lib/libtbb.dylib");
            RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/Embree3/MacOSX/lib/libtbbmalloc.dylib");

            PublicDefinitions.Add("USE_EMBREE_VOXEL=1");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
#if UE_4_24_OR_LATER
            // TODO
            // throw new System.ArgumentException("Embree Linux Error: Need to fix the code below for 4.24. Some tricky stuff was happening with UE removing the .3 at the end, so can't directly fix it");
            PublicDefinitions.Add("USE_EMBREE_VOXEL=0");
#else
            string SDKDir = Path.Combine(ModuleDirectory, "Linux");
            string IncludeDir = SDKDir + "include/";
            string LibDir = SDKDir + "lib/";

            PublicIncludePaths.Add(IncludeDir);

            /////////////////////////////////////////
            // The following are needed for linking:

            PublicLibraryPaths.Add(LibDir);

            PublicAdditionalLibraries.Add(":libembree3.so.3");
            PublicAdditionalLibraries.Add(":libtbb.so.2");
            PublicAdditionalLibraries.Add(":libtbbmalloc.so.2");

            //////////////////////////////////////////////////////
            // The following are needed for runtime dependencies:

            // Adds the library path to LD_LIBRARY_PATH
            PublicRuntimeLibraryPaths.Add(LibDir);

            // Tells UBT to copy the .so to the packaged game directory
            RuntimeDependencies.Add(LibDir + "libembree3.so.3");
            RuntimeDependencies.Add(LibDir + "libtbb.so.2");
            RuntimeDependencies.Add(LibDir + "libtbbmalloc.so.2");

            PublicDefinitions.Add("USE_EMBREE_VOXEL=1");
#endif
        }
        else
        {
            PublicDefinitions.Add("USE_EMBREE_VOXEL=0");
        }
#else
        PublicDefinitions.Add("USE_EMBREE_VOXEL=0");
#endif
    }
}
