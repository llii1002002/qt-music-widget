# MusicDesktop — Win11 桌面音乐挂件

基于 **Qt 5.14.2 + QML + MSVC(C++17)** 实现的 Windows 11 桌面音乐挂件：把歌词、频谱、封面悬浮在**桌面壁纸层**上，随系统媒体播放实时联动。

## 核心特性

| 特性 | 实现要点 |
|---|---|
| 桌面挂件 | `WindowHost` 通过 **WorkerW** 将窗口挂在壁纸层，不遮挡任务栏与桌面图标 |
| 系统媒体联动 | `MediaSessionWorker` 集成 **SMTC**（System Media Transport Controls），自动跟随 QQ音乐/网易云/浏览器等系统媒体播放 |
| 实时频谱 | `AudioCaptureWorker` 走 **WASAPI** 环回采集系统声音 → `FftAnalyzer` 做 **FFT** 变换 → `SpectrumView` 渲染 |
| 歌词引擎 | `LyricsEngine` 解析 LRC 歌词，`PlaybackClock` 维护播放时钟，逐行高亮与滚动 |
| 主题系统 | `ThemeEngine` 多主题切换（跟随封面主色 / 自定义），`ConfigStore` 持久化配置 |
| 封面集成 | `CoverImageProvider` 将播放器封面注入 QML `Image` 数据源 |
| 音频设备监测 | `DeviceMonitor` 监听默认音频设备切换，频谱自动跟随 |

## 技术栈

- Qt 5.14.2，`QT += core gui qml quick widgets quickcontrols2 concurrent network`
- C++17，MSVC 套件（`/utf-8 /EHsc /bigobj /await`）
- 系统库：`user32 / gdi32 / ole32 / oleaut32 / advapi32 / runtimeobject`（WinRT SMTC）
- 频谱链路：WASAPI 环回采集 → FFT → QML Canvas 渲染

## 目录结构

```
musicdesktop/
├── MusicDesktop.pro              # qmake 工程（MSVC 套件）
├── src/
│   ├── main.cpp                  # 程序入口
│   ├── system/                   # 系统级能力
│   │   ├── WindowHost.cpp/h      # WorkerW 桌面壁纸层挂件
│   │   ├── MediaSessionWorker    # SMTC 系统媒体会话
│   │   ├── AudioCaptureWorker    # WASAPI 环回音频采集
│   │   ├── FftAnalyzer           # FFT 频谱分析
│   │   └── DeviceMonitor         # 音频设备监测
│   ├── core/                     # 核心逻辑
│   │   ├── PlayerController      # 播放控制
│   │   ├── LyricsEngine          # LRC 歌词解析与同步
│   │   ├── PlaybackClock         # 播放时钟
│   │   ├── ThemeEngine           # 主题引擎
│   │   ├── ConfigStore           # 配置持久化
│   │   ├── SpectrumProvider      # 频谱数据供给
│   │   ├── CoverImageProvider    # 封面图数据源
│   │   └── AppActions            # 全局动作定义
│   └── qml/                      # 界面
│       ├── MainWindow.qml        # 主窗口
│       ├── LyricView.qml         # 歌词视图
│       ├── SpectrumView.qml      # 频谱视图
│       ├── AlbumCover.qml        # 封面组件
│       ├── SettingsPanel.qml     # 设置面板
│       └── NumSpin.qml           # 数值调节组件
├── resources.qrc                 # 资源
├── build_debug.bat / build_release.bat   # 构建脚本
└── MusicDesktop_架构图.html / 数据溯源图.html  # 设计文档
```

## 构建与运行

环境：**Qt 5.14.2 + MSVC2017/2019**（需先运行 vcvars64 或使用 Qt Creator 的 MSVC 套件）。

**方式一（推荐）**：Qt Creator 打开 `MusicDesktop.pro` → 选 MSVC 套件 → 构建运行。

**方式二（命令行）**：先初始化 MSVC 环境（`vcvarsall.bat x64`），再执行仓库内的构建脚本：

```bat
build_release.bat
```

产物输出到 `bin/`（`.pro` 中 `DESTDIR = $$PWD/bin`，中间产物在 `build/`）。

## 使用提示

- 运行时让系统播放任意音乐（QQ音乐 / 网易云 / 浏览器等），挂件自动跟随并显示歌词与频谱
- 歌词需与歌曲同目录或手动指定 `.lrc` 文件（见 `LyricsEngine`）
- 详细设计见 `MusicDesktop_架构图.html` 与 `MusicDesktop_数据溯源图.html`

## 说明

- 频谱采集依赖 WASAPI 环回，需系统"立体声混音"或声卡支持环回；不同声卡环境可能需调整 `AudioCaptureWorker` 参数
- 项目为桌面个人工具，未经系统全局安全测试，建议仅在本机使用
