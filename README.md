# EyeCare – 护眼助手

一个基于 Win32 API 的屏幕护眼工具，帮助你在长时间使用电脑时定时休息。
- 支持硬锁（锁定键盘鼠标）和软锁（仅提醒）两种模式，所有设置持久化保存。
- 用DeepSeek聊天网页版完成。

---

## ✨ 功能特点

- **定时提醒**：自定义工作和休息时长（分钟），工作结束后自动进入休息倒计时。
- **双模式锁屏**：
  - **硬锁**：全屏遮罩 + 键盘鼠标锁定，强制休息。
  - **软锁**：居中半透明窗口，鼠标可穿透，仅提醒不打扰操作。
- **托盘集成**：运行时仅显示系统托盘图标，支持右键菜单：
  - 设置参数（工作/休息时长、锁屏模式、开机自启）
  - 立即手动触发休息
  - 退出程序
- **悬浮提示**：鼠标悬停托盘图标，显示当前阶段剩余时间（`MM:SS` 格式）。
- **开机自启动**：通过“启动”文件夹创建快捷方式，无需管理员权限。
- **设置持久化**：所有配置自动保存到 `EyeCare.ini`（与 exe 同目录），下次启动自动加载。

---

## 📦 系统要求

- Windows 7 / 8 / 10 / 11（32 位或 64 位）
- 无需管理员权限（除安装系统钩子外，但钩子无需提权）

---

## 下载

[点击下载 v1.0.0](https://github.com/ufidon/EyeCare/releases/download/v1.0.0/EyeCare.exe)
![GitHub Downloads (all assets, latest release)](https://img.shields.io/github/downloads/ufidon/EyeCare/latest/total)


---

## 🚀 使用指南

1. **启动程序**：双击 `EyeCare.exe`，无主窗口，仅在系统托盘（右下角）显示图标。
2. **配置参数**：右键点击托盘图标 → “设置”，弹出对话框：
   - **工作间隔**（分钟）：多久提醒一次休息（1–120）。
   - **休息间隔**（分钟）：每次休息时长（1–30）。
   - **锁屏模式**：硬锁（锁定输入）或软锁（仅提醒）。
   - **随系统启动**：勾选后，下次开机自动启动本程序。
   - 点击“确定”保存并立即生效。
3. **查看倒计时**：鼠标悬停托盘图标，显示当前阶段剩余时间。
4. **手动休息**：右键菜单 → “立即休息”，立刻进入休息状态（仅在工作阶段有效）。
5. **退出**：右键 → “退出”，关闭程序并自动保存设置。

---

## 🔧 编译（从源码）

### 环境准备
- Visual Studio 2022+（或 Build Tools）并安装“C++ 生成工具”工作负载。
- 在“Developer Command Prompt for VS”中操作。

### 编译步骤
1. 将 `EyeCare.cpp`、`EyeCare.rc`、`Makefile` 放在同一目录。
2. 执行 `nmake` 一键编译，生成 `EyeCare.exe`。
3. 如需清理，执行 `nmake clean`。

### 手动编译（不借助 Makefile）
```cmd
rc EyeCare.rc
cl /D "UNICODE" /D "_UNICODE" /utf-8 EyeCare.cpp EyeCare.res /FeEyeCare.exe /link /SUBSYSTEM:WINDOWS user32.lib kernel32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib
```

> **注意**：编译时使用了 `/utf-8` 选项，若编译器版本过旧（VS2015 之前），请去掉并确保源文件保存为 **UTF-8 with BOM**。

---

## 📁 配置文件 (`EyeCare.ini`)

程序首次运行或点击“确定”设置后，会在 `EyeCare.exe` 所在目录生成 `EyeCare.ini`，内容示例：
```ini
[Settings]
WorkMinutes=45
RestMinutes=5
LockMode=0        ; 0=硬锁, 1=软锁
AutoStart=1       ; 1=启用, 0=禁用
```

你可以直接编辑此文件（关闭程序后），再次启动时自动生效。

---

## ⚠️ 注意事项

- **杀毒软件**：由于使用了低级键盘/鼠标钩子（`WH_KEYBOARD_LL` / `WH_MOUSE_LL`），部分杀毒软件可能误报。请将 `EyeCare.exe` 加入信任列表。
- **Windows 锁屏**：使用 `GetTickCount()` 计算已过时间，锁屏期间计时依然准确（不会停滞）。
- **软锁模式**：窗口透明且鼠标穿透。
- **多显示器**：硬锁窗口仅覆盖主显示器，若需全屏多显示器，可自行修改代码扩展。

---

## 📝 许可

本项目仅供学习交流，无特定许可限制。欢迎修改和分发。

---

**Enjoy your healthy coding!** 👀💻