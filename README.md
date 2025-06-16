# AcceleratorCSS

**Local crash handler for CounterStrikeSharp with callback trace logging.**

---

## Overview

`AcceleratorCSS` is a Metamod plugin for CS2 servers. It provides:

* Local crash dump creation using Breakpad
* Custom crash log file generation
* Full callback trace logging before crash (tracked by `RegisterCallbackTrace`)
* Runtime hook on CounterStrikeSharp to intercept callback calls
* Auto-repair of signal handler detour (in `GameFrame()`)
* Works independently — **no C# plugin is required**
* Currently **Linux-only**, but **Windows support is planned**

---

## Features

* **Hooked `RegisterCallbackTrace`** to log all executed C# callbacks
* **Breakpad integration** for safe `.dmp` and `.txt` log generation
* **Thread-safe ring buffer** for last 5 callback invocations
* **Hook auto-restoration** of crash signal handlers
* **Support for late plugin loading**
* **C# plugin is no longer required**

---

## File Output

Crash logs are written to:

```
/addons/AcceleratorCSS/logs/
```

With files:

```
crash_dump.dmp.txt (callbacks should in .txt, no .dmp!)
```

Text logs contain:

* Map, game path, command line
* Console output buffer
* Trace of recent callbacks (name, count, stack)
* **Accurate stacktrace metadata for each C# callback**

---

## Building

### Dependencies:

* [funchook](https://github.com/kubo/funchook)
* [Google Breakpad](https://chromium.googlesource.com/breakpad/breakpad/)
* [spdlog](https://github.com/gabime/spdlog)
* HL2SDK-CS2 headers
* Metamod\:Source (CS2 version)

### Docker + Premake:

```bash
git clone https://github.com/SlynxCZ/AcceleratorCSS.git
cd AcceleratorCSS
git submodule update --init --recursive
docker compose up --build
```

---

## Integration with CounterStrikeSharp

This plugin will hook the `RegisterCallbackTrace` symbol in `counterstrikesharp.so` automatically — **no C# plugin required**.

---

## Example Logged Trace (this is how i found out why my server crashes after 4 months lol)

```text
-------- CALLBACK TRACE BEGIN -> NEWEST CALLBACK IS FIRST --------
Name: CounterStrikeSharp.API.Core.BasePlugin+<>c__DisplayClass51_0`1[[CounterStrikeSharp.API.Core.Listeners+OnTick, CounterStrikeSharp.API, Version=1.0.0.0, Culture=neutral, PublicKeyToken=null]].<RegisterListener>b__2
Count: 1
Profile: ScriptCallback::Execute::<RegisterListener>b__2
CallerStack:
JailBreak.JailBreak+<>c__DisplayClass114_1.<EventPlayerDeath>b__3 @ :0
...
-------- CALLBACK TRACE END --------
```

---

## License

[GPLv3](https://www.gnu.org/licenses/gpl-3.0.en.html)

## Author

**Slynx / [funplay.pro](https://funplay.pro/)**
