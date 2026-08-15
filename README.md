# 实验室危化品库房环境智能管控系统

> Laboratory Hazardous Chemical Warehouse Environmental Intelligent Control System

基于 **GEC6818 开发板**（ARM Cortex-A53）的危化品库房环境智能管控系统，实现**门禁管理、环境监测、泄漏报警、联动控制**一体化，并提供 Qt 桌面模拟版本用于开发调试与界面演示。

![UI 预览](模拟界面预览.html)

---

## 目录

- [项目简介](#项目简介)
- [功能特性](#功能特性)
- [系统架构](#系统架构)
- [硬件组成](#硬件组成)
- [软件组成](#软件组成)
- [目录结构](#目录结构)
- [快速开始](#快速开始)
- [文档索引](#文档索引)
- [Git 提交规范](#git-提交规范)

---

## 项目简介

实验室中存放的危化品（易燃气体、有毒试剂等）一旦发生**泄漏、积水或温湿度异常**，极易引发安全事故。本系统以 GEC6818 开发板为核心，实时采集库房内多种环境参数，当检测到异常时**自动声光报警并联动继电器**（切断气阀 / 开启排风 / 控制门禁），同时提供 **RFID 刷卡门禁** 功能，仅授权人员可开门进入，从"监测、报警、控制"三个层面保障危化品库房安全。

系统包含两套软件实现：

| 版本 | 语言/框架 | 目标平台 | 用途 |
|------|-----------|----------|------|
| `app/` | C (Linux Framebuffer) | GEC6818 开发板 | 嵌入式端部署运行 |
| `app_qt/` | C++ / Qt Widgets | GEC6818 / 桌面 PC | 界面重构版本，支持桌面模拟调试 |

## 功能特性

- **RFID 门禁控制**：RC522 读卡器（UART 9600）读取卡片 UID，白名单校验；授权卡放行（继电器开门 5 秒后自动关闭），未授权卡触发红灯警告
- **气体泄漏监测**：MQ-2 气体/烟雾传感器实时检测，超阈值立即报警
- **甲醛 / CO₂ 监测**（Qt 版）：21VOC 五合一空气质量传感器解析 HCHO（甲醛）与 CO₂ 浓度，超标报警（HCHO ≥ 0.10 mg/m³，CO₂ ≥ 1000 ppm）
- **温湿度监测**：DHT11 传感器每 3 秒采样一次温湿度
- **积水/液体检测**（C 版）：ADC 水位传感器，超过阈值判定漏水报警
- **声光报警**：PWM 蜂鸣器鸣叫 + 红色 LED 闪烁 + 界面状态标红
- **继电器联动**：驱动门锁 / 气阀 / 排风扇等外设
- **实时 LCD 显示**：800×480 全彩界面，系统状态、传感器数据、时间、刷卡信息、运行时长实时刷新
- **桌面模拟调试**（Qt 版）：`SIMULATE_HARDWARE` 宏开启传感器与刷卡事件模拟，无需硬件即可演示完整流程

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                         GEC6818 开发板                        │
│                                                              │
│  ┌───────────┐   ┌──────────────┐   ┌────────────────────┐  │
│  │ LCD 800x480│   │ 主控程序      │   │ 传感器轮询 (10Hz)   │  │
│  │  framebuffer│◄──┤ main.c        │◄──┤ Gas (MQ-2)        │  │
│  └───────────┘   │  app/ 或 app_qt│   │ DHT11 (温湿度)     │  │
│  ┌───────────┐   │               │   │ 21VOC (HCHO/CO₂)   │  │
│  │ 蜂鸣器 PWM │◄──┤ 报警逻辑       │◄──┤ ADC (积水检测)      │  │
│  │ LED (红/绿)│◄──┤               │   └────────────────────┘  │
│  │ 继电器     │◄──┤ RFID 门禁     │◄── RC522 读卡器 (UART)    │
│  └───────────┘   └──────────────┘   └────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 硬件组成

| 硬件 | 型号/接口 | 设备节点 |
|------|-----------|----------|
| 主控板 | GEC6818（ARM Cortex-A53） | — |
| 显示屏 | 800×480 32bpp LCD | Framebuffer |
| RFID 读卡器 | RC522，UART 9600 8N1 | `/dev/ttySAC2`（C 版）/ `/dev/ttySAC1`（Qt 版） |
| 气体传感器 | MQ-2（GPIO） | `/dev/gec_gas_drv` |
| 温湿度传感器 | DHT11（GPIO） | `/dev/humidity` |
| 水质/水位传感器 | ADC 通道 2 | `/dev/gec6818_adc` |
| 空气质量传感器 | 21VOC 五合一（UART） | `/dev/ttySAC2`（Qt 版） |
| 蜂鸣器 | PWM2（GPIO C+14） | `/dev/pwm` |
| LED | GPIOE13(红) / GPIOC17(绿) | `/dev/Led` |
| 继电器 | GPIO B+28 | `/dev/relay` |

> 详细接线说明见 [docs/02-硬件接线说明.md](docs/02-硬件接线说明.md)。

## 软件组成

### `app/` — C 版（嵌入式部署）

| 文件 | 功能 |
|------|------|
| `main.c` | 主程序：UI 布局绘制、传感器轮询、RFID 处理、报警与联动逻辑 |
| `ui_lcd.c/h` | LCD Framebuffer 驱动（画点、画线、矩形、字符、字符串） |
| `rfid_card.c/h` | RC522 RFID 读卡与白名单校验 |
| `gas_monitor.c/h` | MQ-2 气体传感器读取 |
| `weather.c/h` | DHT11 温湿度读取 |
| `water_sensor.c/h` | ADC 水位/积水检测 |
| `alarm.c/h` | 蜂鸣器、LED、继电器控制与组合报警动作 |
| `font_8x16.h` | 8×16 点阵英文字库 |

### `app_qt/` — Qt 版（界面重构 + 桌面模拟）

| 文件 | 功能 |
|------|------|
| `main.cpp` | 应用入口（GEC6818 全屏 800×480） |
| `mainwindow.cpp/h` | 主界面：标题栏 / 状态面板 / 传感器面板 / 刷卡信息栏 / 状态栏 |
| `hardwaremanager.cpp/h` | 硬件管理：10Hz 轮询、传感器数据、RFID 事件、报警状态，通过信号驱动 UI |
| `gasmonitor.cpp/h` | MQ-2 气体传感器 |
| `weather.cpp/h` | DHT11 温湿度传感器 |
| `voc21sensor.cpp/h` | 21VOC 五合一传感器帧解析（HCHO + CO₂） |
| `rfidcard.cpp/h` | RFID 刷卡模块 |
| `alarm.cpp/h` | 蜂鸣器 / LED / 继电器控制 |

### 其他

| 文件 | 说明 |
|------|------|
| `rfid_dump.c` | RFID 卡号读取调试工具，用于采集卡片 UID 并加入白名单 |
| `模拟界面预览.html` | 网页版 UI 模拟预览（800×480，可交互按钮模拟报警/刷卡），浏览器直接打开即可 |

## 目录结构

```
lab_chemical_control/
├── README.md                      # 项目总览（本文件）
├── docs/
│   ├── 01-系统设计.md              # 系统架构与软件模块设计
│   ├── 02-硬件接线说明.md          # 硬件平台与传感器接线
│   ├── 03-编译与部署.md            # 交叉编译 / 部署 / 运行指南
│   └── 04-界面说明与模拟.md        # UI 布局、网页预览、桌面模拟调试
├── app/                           # C 版（GEC6818 嵌入式部署）
│   ├── Makefile
│   ├── main.c
│   ├── ui_lcd.c / ui_lcd.h
│   ├── rfid_card.c / rfid_card.h
│   ├── gas_monitor.c / gas_monitor.h
│   ├── weather.c / weather.h
│   ├── water_sensor.c / water_sensor.h
│   ├── alarm.c / alarm.h
│   └── font_8x16.h
├── app_qt/                        # Qt 版（支持桌面模拟）
│   ├── lab_chemical_control.pro
│   ├── main.cpp
│   ├── mainwindow.cpp / mainwindow.h
│   ├── hardwaremanager.cpp / hardwaremanager.h
│   ├── gasmonitor.cpp / gasmonitor.h
│   ├── weather.cpp / weather.h
│   ├── voc21sensor.cpp / voc21sensor.h
│   ├── rfidcard.cpp / rfidcard.h
│   └── alarm.cpp / alarm.h
├── rfid_dump.c                    # RFID 卡号调试工具
└── 模拟界面预览.html               # 网页 UI 模拟预览
```

## 快速开始

### 1. 界面预览（无需硬件）

浏览器直接打开 [`模拟界面预览.html`](模拟界面预览.html)，可交互演示气体报警、积水报警、刷卡授权/拒绝等场景，界面尺寸与 GEC6818 LCD 完全一致。

### 2. Qt 桌面模拟（PC 上演示完整流程）

```bash
cd app_qt
qmake CONFIG+=desktop    # 开启 SIMULATE_HARDWARE 模拟
make
./lab_chemical_control   # 窗口内带调试按钮，可模拟传感器与刷卡事件
```

### 3. 交叉编译并部署到 GEC6818

```bash
cd app
make                    # arm-linux-gnueabihf-gcc 交叉编译
make install BOARD_IP=192.168.1.100   # 拷贝到开发板
ssh root@192.168.1.100 ./lab_chemical_control
```

### 4. 采集 RFID 卡号（加入白名单）

```bash
arm-linux-gnueabihf-gcc rfid_dump.c -o rfid_dump
# 拷贝到板子运行，刷卡后终端输出卡号，将卡号填入 rfid_card.c 的 whitelist[]
```

> 完整编译部署步骤见 [docs/03-编译与部署.md](docs/03-编译与部署.md)。

## 文档索引

| 文档 | 内容 |
|------|------|
| [docs/01-系统设计.md](docs/01-系统设计.md) | 系统架构、模块划分、数据流、报警逻辑设计 |
| [docs/02-硬件接线说明.md](docs/02-硬件接线说明.md) | GEC6818 平台、各传感器/外设接线与设备节点 |
| [docs/03-编译与部署.md](docs/03-编译与部署.md) | C 版交叉编译、Qt 版桌面/ARM 编译、部署运行 |
| [docs/04-界面说明与模拟.md](docs/04-界面说明与模拟.md) | UI 布局说明、网页模拟预览、桌面调试功能 |

## Git 提交规范

- 提交信息请附加协作署名（本项目采用如下格式）：

```
<主题描述>

Co-Authored-By: AtomCode (deepseek-v4-flash) <noreply@atomgit.com>
```

---

## License

本项目为嵌入式系统开发课程设计作品，仅用于教学与学习交流。
