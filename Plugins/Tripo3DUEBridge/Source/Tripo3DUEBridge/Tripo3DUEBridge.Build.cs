// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Tripo3DUEBridge : ModuleRules
{
	public Tripo3DUEBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Version detection for conditional compilation
		int MajorVersion = Target.Version.MajorVersion;
		int MinorVersion = Target.Version.MinorVersion;
		
		// 启用预编译支持，使蓝图工程可以使用预编译的二进制文件
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"EditorStyle",
				"InputCore",
				"LevelEditor",
				"Projects",
				"AssetTools",
				"AssetRegistry",
				"ContentBrowser",
				"FBX",
				"UnrealEd",
				"MainFrame",
				"Json",
				"JsonUtilities",
				"HTTP",
				"Sockets",
				"Networking",
				"WebSockets",
				"DesktopPlatform",
				"MessageLog",
			}
		);
		
		// IXWebSocket 库配置
		string IXWebSocketBasePath = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "IXWebSocket");
		string IXWebSocketLibPath = Path.Combine(IXWebSocketBasePath, "Lib", "Win64");
		string IXWebSocketLibFile = Path.Combine(IXWebSocketLibPath, "IXWebSocket.lib");
		
		// 检查是否存在预编译的静态库
		bool bUsePrecompiledLib = File.Exists(IXWebSocketLibFile);
		
		if (bUsePrecompiledLib)
		{
			// 使用预编译的静态库（蓝图工程模式）
			System.Console.WriteLine("[Tripo3DUEBridge] Using precompiled IXWebSocket library");
			PublicAdditionalLibraries.Add(IXWebSocketLibFile);
			
			// 只需要公共头文件路径
			PublicIncludePaths.Add(IXWebSocketBasePath);
			PublicIncludePaths.Add(Path.Combine(IXWebSocketBasePath, "ixwebsocket"));
		}
		else
		{
			// 从源码编译（C++ 工程模式）
			System.Console.WriteLine("[Tripo3DUEBridge] Building IXWebSocket from source");
			
			// 添加包含路径
			PublicIncludePaths.Add(IXWebSocketBasePath);
			PublicIncludePaths.Add(Path.Combine(IXWebSocketBasePath, "ixwebsocket"));
		}
		
		// Detect zlib version based on UE version
		// Different UE versions use different zlib versions:
		// UE 5.4+: zlib 1.3
		// UE 5.1-5.3: zlib 1.2.8 (or similar 1.2.x versions)
		// UE 5.7+: zlib 1.3
		string ZlibVersion = "1.3"; // Default to latest
		if (MajorVersion == 5 && MinorVersion <= 6)
		{
			// Try common zlib versions for UE 5.1-5.6
			// Note: UE 5.4+ may use 1.3, earlier versions use 1.2.x
			// Probe to find which version exists
			string[] PossibleVersions = { "1.3", "1.2.13", "1.2.12", "1.2.11", "1.2.8" };
			bool bFoundVersion = false;
			foreach (string Version in PossibleVersions)
			{
				string TestPath = Path.Combine(Target.UEThirdPartySourceDirectory, "zlib", Version);
				if (Directory.Exists(TestPath))
				{
					ZlibVersion = Version;
					bFoundVersion = true;
					System.Console.WriteLine("[Tripo3DUEBridge] Found zlib version: " + Version);
					break;
				}
			}
			if (!bFoundVersion)
			{
				System.Console.WriteLine("[Tripo3DUEBridge] Warning: Could not find zlib, defaulting to 1.3");
			}
		}

		// Add UE built-in zlib paths
		string ZlibPath = Path.Combine(Target.UEThirdPartySourceDirectory, "zlib", ZlibVersion, "include");
		PublicIncludePaths.Add(ZlibPath);

		// Link zlib library
		string ZlibLibPath = Path.Combine(Target.UEThirdPartySourceDirectory, "zlib", ZlibVersion, "lib", "Win64", "Release");
		PublicAdditionalLibraries.Add(Path.Combine(ZlibLibPath, "zlibstatic.lib"));
		
		PublicDefinitions.Add("IXWEBSOCKET_USE_ZLIB=1");
		PublicDefinitions.Add("_WINSOCK_DEPRECATED_NO_WARNINGS");
		PublicDefinitions.Add("NOMINMAX"); // Prevent Windows.h min/max macros
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("ws2_32.lib");
			PublicSystemLibraries.Add("crypt32.lib");
			PublicSystemLibraries.Add("iphlpapi.lib");
		}
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
