# SPlayerLyric

TrafficMonitor 的 SPlayer 歌词任务栏显示插件

![Preview](image.png)

## 功能特性

- 🎵 **实时歌词显示** - 在任务栏显示当前播放的歌词
- 🔗 **自动连接** - 自动连接 SPlayer 的 WebSocket 服务
- 🔄 **断线重连** - 连接断开后自动重试
- 🎮 **播放控制** - 左键暂停/播放
- 🌙 **主题适配** - 自动适配深色/浅色模式

## 使用方法

### 前提条件

1. 安装 [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) (v1.82+)
2. 安装 [SPlayer](https://github.com/imsyy/SPlayer) 并确保 WebSocket 服务已启用

### 安装插件

1. 下载 `SPlayerLyric.dll` (根据系统选择 x86 或 x64 版本)
2. 将 dll 文件放入 TrafficMonitor 的 `plugins` 目录
3. 重启 TrafficMonitor
4. 在任务栏右键菜单 → 显示设置 → 勾选 "歌词"

### 操作说明

| 操作 | 功能 |
|------|------|
| 左键单击 | 播放/暂停切换 |
| 右键 | 显示 TrafficMonitor 菜单 |

## 配置文件

配置文件位于 TrafficMonitor 数据目录下的 `SPlayerLyric.ini`

```ini
[Connection]
Port=25885              ; WebSocket 端口
ReconnectInterval=5000  ; 重连间隔 (ms)

[Display]
Width=300               ; 显示宽度
FontSize=11             ; 字体大小
FontName=Microsoft YaHei UI  ; 字体名称

[Lyric]
EnableScrolling=1       ; 启用滚动动画
EnableYrc=1             ; 启用逐字高亮
HighlightColor=16751616 ; 高亮颜色 (RGB)
```

## 编译

### 环境要求

- Visual Studio 2022 (v143)
- Windows 10 SDK
- MFC 动态链接库

### 编译步骤

1. 打开 `TrafficMonitor.sln`
2. 选择 `SPlayerLyric` 项目
3. 选择配置 (Debug/Release) 和平台 (Win32/x64)
4. 编译项目

编译产物位于 `Bin\<Platform>\<Configuration>\plugins\SPlayerLyric.dll`

## 技术架构

```
SPlayerLyric
├── SPlayerLyricPlugin    # 主插件类 (ITMPlugin)
├── LyricDisplayItem      # 歌词显示项 (IPluginItem, 自绘)
├── WebSocketClient       # WebSocket 客户端
├── LyricManager          # 歌词管理器
├── JsonParser            # JSON 解析器
└── Config                # 配置管理
```

## 依赖

- TrafficMonitor 插件接口 (PluginInterface.h)
- Windows Socket API (ws2_32.lib)

## 许可证

MIT License

## 致谢

- [TrafficMonitor](https://github.com/zhongyang219/TrafficMonitor) - 网络监控工具
- [SPlayer](https://github.com/imsyy/SPlayer) - 在线音乐播放器
