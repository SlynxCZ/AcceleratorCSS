using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using CounterStrikeSharp.API;
using CounterStrikeSharp.API.Core;
using HarmonyLib;

// ReSharper disable InconsistentNaming

namespace AcceleratorCSS_CSS;

// ReSharper disable once InconsistentNaming
// ReSharper disable once UnusedType.Global
public class AcceleratorCSS_CSS : BasePlugin
{
    private Harmony? _harmony;
    private static bool LightweightMode { get; set; }

    public override string ModuleName => "AcceleratorCSS_CSS";
    public override string ModuleVersion => "1.0.2";
    public override string ModuleAuthor => "Slynx";
    public override string ModuleDescription => "AcceleratorCSS's C# side";

    private static RegisterCallbackJsonDelegate? NativeTraceJson;

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
        var path = Path.Combine(Server.GameDirectory, "csgo", "addons", "AcceleratorCSS", "bin", "linuxsteamrt64",
            "AcceleratorCSS.so");

        if (!File.Exists(path))
        {
            Prints.ServerLog($"[AcceleratorCSS_CSS] Native lib not found: {path}", ConsoleColor.Red);
            return;
        }

        try
        {
            var handle = NativeLibrary.Load(path);
            var fnPtr = NativeLibrary.GetExport(handle, "RegisterCallbackTrace");
            NativeTraceJson = Marshal.GetDelegateForFunctionPointer<RegisterCallbackJsonDelegate>(fnPtr);

            var initPtr = NativeLibrary.GetExport(handle, "CssPluginRegistered");
            var initFn = Marshal.GetDelegateForFunctionPointer<CssPluginRegisteredDelegate>(initPtr);
            LightweightMode = initFn();

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

    // ReSharper disable once UnusedParameter.Local
    private static bool TracePrefix(MethodBase __originalMethod, object __instance, object[]? __args)
    {
        try
        {
            // ReSharper disable once ConditionalAccessQualifierIsNonNullableAccordingToAPIContract
            var method = CleanMethodName($"{__originalMethod?.DeclaringType?.FullName}::{__originalMethod?.Name}");
            method = TrimAndClean(method, 512);

            if (LightweightMode)
            {
                SafeNativeTrace(method, "Lightweight mode is enabled - no profile given", "Lightweight mode is enabled - no full stacktrace given");
            }
            else
            {
                var args = __args != null ? string.Join(", ", __args.Select(SafeToString)) : "null";
                var stack = new StackTrace(2, true).ToString();

                args = TrimAndClean(args, 2048);
                stack = TrimAndClean(stack, 4096);

                SafeNativeTrace(method, args, stack);
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[AcceleratorCSS] TracePrefix exception: {ex.Message}");
        }

        return true;
    }

    private static void SafeNativeTrace(string name, string profile, string stack)
    {
        if (NativeTraceJson == null)
            return;

        var payload = new
        {
            name,
            profile,
            caller = stack
        };

        string json = JsonSerializer.Serialize(payload, new JsonSerializerOptions
        {
            Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
        });

        NativeTraceJson(json);
    }

    private static string CleanMethodName(string input)
    {
        string[] unwanted = ["System.", "Microsoft.", "CounterStrikeSharp.API.", "<>c__DisplayClass", "lambda_method"];
        foreach (var pattern in unwanted)
            input = input.Replace(pattern, "");
        return input;
    }

    private static string SafeToString(object? obj)
    {
        try
        {
            return obj?.ToString() ?? "null";
        }
        catch
        {
            return "[toString_failed]";
        }
    }

    private static string TrimAndClean(string input, int maxLength)
    {
        if (string.IsNullOrWhiteSpace(input))
            return "<empty>";

        var builder = new StringBuilder(Math.Min(input.Length, maxLength));
        foreach (var ch in input)
        {
            if (builder.Length >= maxLength) break;
            if (char.IsControl(ch) || char.IsSurrogate(ch) || ch == '\uFFFD' || ch < 32 || ch > 0xFFFD)
                builder.Append('?');
            else
                builder.Append(ch);
        }

        return builder.ToString().Trim();
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate bool CssPluginRegisteredDelegate();

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void RegisterCallbackJsonDelegate(
        [MarshalAs(UnmanagedType.LPUTF8Str)] string json
    );

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

    private static Type[] SafeGetTypes(Assembly asm)
    {
        try
        {
            return asm.GetTypes();
        }
        catch
        {
            return Array.Empty<Type>();
        }
    }
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