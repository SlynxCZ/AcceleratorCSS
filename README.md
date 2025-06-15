# AcceleratorCSS

**AcceleratorCSS** is a native crash handler integration for **CounterStrikeSharp** plugins. It catches unhandled exceptions in C# plugins and logs detailed stack traces using a native `.so` Metamod plugin.

---

## 📦 Installation

If you're using a **precompiled release** (e.g., downloaded a ZIP from GitHub Releases):

1. Extract the contents of the ZIP into your server's `csgo/addons/` directory:

```
csgo/
 ├── addons/
 │   ├── AcceleratorCSS/
 │   │   └── bin/linuxsteamrt64/AcceleratorCSS.so
```

2. Done — the plugin will auto-load on next server start.

---

If you're building it yourself:

1. Clone the repository:

```bash
https://github.com/SlynxCZ/AcceleratorCSS
```

2. Build the native Metamod plugin (`AcceleratorCSS.so`) and place it into:

```
csgo/addons/AcceleratorCSS/bin/linuxsteamrt64/AcceleratorCSS.so
```

3. Build the CounterStrikeSharp C# plugin (`AcceleratorCSS_CSS.dll`) and place it into:

```
csgo/addons/counterstrikesharp/plugins/AcceleratorCSS_CSS/
```

---

## 🧠 How It Works

* On server startup, the C# plugin resolves and loads the `.so` using `DllImport`
* When any `UnhandledException` occurs in any plugin (e.g., missing null check, async task crash), it's caught via `AppDomain.CurrentDomain.UnhandledException`
* The C# plugin sends the stacktrace to the `.so`
* The `.so` logs the crash with colored output using `spdlog` and can write it to a file

---

## ✅ Features

* 🔄 Auto-loads native `.so` from proper relative path
* ⚠️ Catches unhandled exceptions across all CS# plugins
* 🧠 Logs the full exception stacktrace natively
* 📁 Uses `spdlog` for advanced formatting and color output
* 🧩 Compatible with any CounterStrikeSharp server on Linux

---

## 📝 License

GPLv3 License © 2025 [slynxcz](https://github.com/slynxcz)
