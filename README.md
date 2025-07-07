# AcceleratorCSS

**Local crash handler for CounterStrikeSharp with callback trace logging.**

---

## Overview

`AcceleratorCSS` is a Metamod plugin for CS2 servers. It provides:

* Local crash dump creation using Breakpad
* Custom crash log file generation
* Full callback trace logging before crash
* Runtime hook on CounterStrikeSharp to intercept all callback calls
* Configurable trace filters (`config.json`)
* Auto-repair of signal handler detour (in `GameFrame()`)
* Requires a C# plugin (`AcceleratorCSS_CSS`) to hook callback invocations
* Currently Linux-only, but Windows support is planned

---

## Features

* Tracing of **all** executed C# callbacks (not limited to `FunctionReference`)
* Breakpad integration for safe `.txt` log generation
* Thread-safe ring buffer for last 5 callback invocations
* Hook auto-restoration of crash signal handlers
* Support for late plugin loading
* Config system for filtering noisy traces (`ProfileExcludeFilters`, defaultly "OnTick", "CheckTransmit", "Display" are blocked)

---

## File Output

Crash logs are written to:

```
/addons/AcceleratorCSS/logs/
```

Files:

```
crash_dump.dmp.txt
```

Text logs contain:

* Map, game path, command line
* Console output buffer
* Trace of recent callbacks (name, count, stack)
* Accurate stacktrace metadata for each C# callback

---

## Config (`config.json`)

Example:

```json
{
  "LightweightMode": true,
  "ProfileExcludeFilters": [
    "HeartbeatListener",
    "SomeUnimportantPluginNamespace",
    "OnTick"
  ]
}
```

In config you can set LightweightMode, this helps reducing power usage at cost of logging only method names (eg: Namespace.Class.OnAnyCommandExecuted), also you can set filters, this helps reduce log noise by skipping specific callbacks based on profile string matches, defaultly "OnTick", "CheckTransmit", "Display" are blocked.

---

## C# Plugin Requirement

Starting from `v1.0.1`, a lightweight C# plugin named `AcceleratorCSS_CSS` is required.

It hooks into the internal execution layer of CounterStrikeSharp and reports callback names, method info, and stacktrace metadata to the native plugin via `RegisterCallbackTrace`.

You can find the plugin in:

```
/managed/AcceleratorCSS_CSS/
```

It is automatically built and deployed together with the native plugin when using Docker.

---

## Building

### Dependencies:

* [funchook](https://github.com/kubo/funchook)
* [Google Breakpad](https://chromium.googlesource.com/breakpad/breakpad/)
* [spdlog](https://github.com/gabime/spdlog)
* HL2SDK-CS2 headers
* Metamod:Source (CS2 version)
* [CounterStrikeSharp v1.0.319 required](https://github.com/roflmuffin/CounterStrikeSharp)

### Docker + Premake:

```bash
git clone https://github.com/SlynxCZ/AcceleratorCSS.git
cd AcceleratorCSS
git submodule update --init --recursive
docker compose up --build
```

---

## Integration with CounterStrikeSharp

The native plugin hooks `RegisterCallbackTrace`, and the C# plugin (`AcceleratorCSS_CSS`) reports all relevant runtime callback invocations to the native layer.

---

## Example Logged Trace

```text
-------- CALLBACK TRACE BEGIN -> NEWEST CALLBACK IS FIRST --------
Name: CounterStrikeSharp.API.Core.BasePlugin+<>c__DisplayClass51_0`1[[CounterStrikeSharp.API.Core.Listeners+OnTick, CounterStrikeSharp.API]]
Profile: ScriptCallback::Execute::<RegisterListener>b__2
CallerStack: JailBreak.JailBreak+<>c__DisplayClass114_1.<EventPlayerDeath>b__3 @ :0
...
-------- CALLBACK TRACE END --------
```

---

## License

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)

## Author

**Slynx / [funplay.pro](https://funplay.pro/)**
