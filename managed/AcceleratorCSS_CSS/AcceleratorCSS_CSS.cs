//
// Created by Michal Přikryl on 12.06.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using CounterStrikeSharp.API;
using CounterStrikeSharp.API.Core;
using HarmonyLib;

// ReSharper disable InconsistentNaming
namespace AcceleratorCSS_CSS;

public class AcceleratorCSS_CSS : BasePlugin
{
    public override string ModuleName => "AcceleratorCSS_CSS";
    public override string ModuleVersion => "1.0.3";
    private Harmony? _harmony;
    public static bool Lightweight;
    private static RegisterCallbackTraceBinary? NativeBinary;
    private static string[] FilterList = [];
    [StructLayout(LayoutKind.Sequential)]
    public struct PluginConfig
    {
        [MarshalAs(UnmanagedType.U1)]
        public bool LightweightMode;

        public IntPtr FiltersPtr;
    }

    public override void Load(bool hotReload)
    {
        RegisterListener<Listeners.OnMetamodAllPluginsLoaded>(OnMetamodAllPluginsLoaded);
    }

    public override void Unload(bool hotReload)
    {
        RemoveListener<Listeners.OnMetamodAllPluginsLoaded>(OnMetamodAllPluginsLoaded);
        _harmony?.UnpatchAll("AcceleratorCSS_CSS");
    }

    private void OnMetamodAllPluginsLoaded()
    {
        var path = Path.Combine(Server.GameDirectory, "csgo", "addons", "AcceleratorCSS", "bin", "linuxsteamrt64", "AcceleratorCSS.so");

        if (!File.Exists(path))
        {
            Prints.ServerLog($"[AcceleratorCSS_CSS] Native lib not found: {path}", ConsoleColor.Red);
            return;
        }

        try
        {
            var handle = NativeLibrary.Load(path);

            var fnPtr = NativeLibrary.GetExport(handle, "RegisterCallbackTraceBinary");
            NativeBinary = Marshal.GetDelegateForFunctionPointer<RegisterCallbackTraceBinary>(fnPtr);

            var initPtr = NativeLibrary.GetExport(handle, "CssPluginRegistered");
            var initFn = Marshal.GetDelegateForFunctionPointer<CssPluginRegisteredDelegate>(initPtr);
            var config = initFn();

            Lightweight = config.LightweightMode;

            if (config.FiltersPtr != IntPtr.Zero)
            {
                var filters = Marshal.PtrToStringUTF8(config.FiltersPtr);
                if (!string.IsNullOrEmpty(filters))
                {
                    if (!string.IsNullOrEmpty(filters))
                    {
                        FilterList = filters.Split(',', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                    }
                    Prints.ServerLog($"[AcceleratorCSS_CSS] Filters received: {filters}", ConsoleColor.Yellow);
                }
            }

            Prints.ServerLog("[AcceleratorCSS_CSS] Native library successfully loaded.", ConsoleColor.Green);
        }
        catch (Exception ex)
        {
            Prints.ServerLog($"[AcceleratorCSS_CSS] Native load failed: {ex}", ConsoleColor.Red);
            return;
        }

        PatchAllMethods();
    }

    private void PatchAllMethods()
    {
        _harmony = new Harmony("AcceleratorCSS_CSS");
        var selfAsm = typeof(AcceleratorCSS_CSS).Assembly;

        foreach (var asm in AppDomain.CurrentDomain.GetAssemblies())
        {
            if (asm == selfAsm || !ReferencesCounterStrikeSharpApi(asm))
                continue;

            foreach (var type in SafeGetTypes(asm))
            {
                if (type == null! || type.Namespace?.StartsWith("System") == true)
                    continue;

                foreach (var method in type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic |
                                                       BindingFlags.Instance | BindingFlags.Static))
                {
                    if (method == null! || method.IsAbstract || method.IsConstructor || method.IsGenericMethod)
                        continue;

                    if (method.Name.StartsWith("get_") || method.Name.StartsWith("set_"))
                        continue;

                    // ReSharper disable once ConditionalAccessQualifierIsNonNullableAccordingToAPIContract
                    if (method.Name.Contains("Invoke") || method.DeclaringType?.Name?.Contains("TraceFilter") == true)
                        continue;

                    var parameters = method.GetParameters();
                    if (parameters.Length == 1 && (
                            parameters[0].ParameterType == typeof(IntPtr) ||
                            parameters[0].ParameterType.Name.Contains("*")))
                        continue;

                    try
                    {
                        var prefix = new HarmonyMethod(typeof(AcceleratorCSS_CSS).GetMethod(nameof(TracePrefix),
                            BindingFlags.Static | BindingFlags.NonPublic));
                        _harmony.Patch(method, prefix: prefix);
                    }
                    catch
                    {
                        /* Silently fail */
                    }
                }
            }
        }

        Prints.ServerLog("[AcceleratorCSS_CSS] All methods patched with Harmony.", ConsoleColor.DarkGreen);
    }

    private static bool ShouldFilter(string name)
    {
        return FilterList.Any(filter => name.Contains(filter, StringComparison.OrdinalIgnoreCase));
    }

    private static bool TracePrefix(MethodBase __originalMethod, object __instance, object[]? __args)
    {
        try
        {
            var name = Trim($"{__originalMethod?.DeclaringType?.FullName}::{__originalMethod?.Name}", 512);

            if (ShouldFilter(name))
                return true;

            string profile = Lightweight ? "LW" : Trim(string.Join(", ", __args?.Select(SafeToString) ?? []), 2048);
            string stack = Lightweight ? "LW" : Trim(new StackTrace(2, true).ToString(), 4096);
            SendBinary(name, profile, stack);
        }
        catch
        {
            // ignored
        }

        return true;
    }

    private static void SendBinary(string name, string profile, string stack)
    {
        if (NativeBinary == null) return;

        var nameBytes = Encoding.UTF8.GetBytes(name);
        var profileBytes = Encoding.UTF8.GetBytes(profile);
        var stackBytes = Encoding.UTF8.GetBytes(stack);

        var buffer = new byte[6 + nameBytes.Length + profileBytes.Length + stackBytes.Length];
        BitConverter.GetBytes((ushort)nameBytes.Length).CopyTo(buffer, 0);
        BitConverter.GetBytes((ushort)profileBytes.Length).CopyTo(buffer, 2);
        BitConverter.GetBytes((ushort)stackBytes.Length).CopyTo(buffer, 4);
        Buffer.BlockCopy(nameBytes, 0, buffer, 6, nameBytes.Length);
        Buffer.BlockCopy(profileBytes, 0, buffer, 6 + nameBytes.Length, profileBytes.Length);
        Buffer.BlockCopy(stackBytes, 0, buffer, 6 + nameBytes.Length + profileBytes.Length, stackBytes.Length);

        NativeBinary(buffer, buffer.Length);
    }

    private static string SafeToString(object? obj)
    {
        try { return obj?.ToString() ?? "null"; } catch { return "[error]"; }
    }

    private static string Trim(string str, int max)
    {
        return str.Length <= max ? str : str[..max];
    }

    private static Type[] SafeGetTypes(Assembly asm)
    {
        try { return asm.GetTypes(); } catch { return []; }
    }

    private static bool ReferencesCounterStrikeSharpApi(Assembly asm)
    {
        try
        {
            return asm.GetReferencedAssemblies().Any(n =>
                n.Name != null && n.Name.Equals("CounterStrikeSharp.API", StringComparison.OrdinalIgnoreCase));
        }
        catch
        {
            return false;
        }
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate PluginConfig CssPluginRegisteredDelegate();

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void RegisterCallbackTraceBinary(byte[] data, int len);
}

public static class Prints
{
    public static void ServerLog(string msg, ConsoleColor color = ConsoleColor.White)
    {
        Console.ForegroundColor = color;
        Console.WriteLine(msg);
        Console.ResetColor();
    }
}