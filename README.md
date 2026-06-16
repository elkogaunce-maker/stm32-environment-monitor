# 基于 STM32 的环境监测报警系统

这是一个基于 STM32F103 的环境监测报警课程项目，包含下位机 Keil 工程、上位机串口工具、通信协议说明、演示材料和项目说明文档。

## 项目功能

- ADC 采集环境传感器数据，并进行阈值判断。
- OLED 显示采集值、阈值、报警状态等信息。
- 蜂鸣器、LED、电机等外设用于报警和状态提示。
- 串口支持状态查询、阈值设置、报警模式切换、电机控制等命令。
- Python 上位机可通过串口读取状态、发送控制命令并辅助调试。

## 目录结构

```text
.
├── 基于STM32的环境检测报警系统/        # STM32 Keil 工程
│   ├── User/                          # main.c 等用户代码
│   ├── Hardware/                      # ADC、OLED、串口、蜂鸣器等驱动
│   ├── System/                        # 系统支持代码
│   ├── Library/                       # STM32 标准外设库
│   ├── Start/                         # 启动文件
│   ├── Objects/project.hex            # 可烧录固件
│   └── project.uvprojx                # Keil 工程文件
├── STM32_UpperComputer/               # Python 上位机
│   ├── stm32_monitor_gui.py
│   ├── run_upper.bat
│   └── protocol.md
├── 基于STM32的环境检测报警系统.ppt
├── 基于STM32的环境检测报警系统/基于STM32的环境监测报警系统_项目说明.docx
└── Git命令使用说明.docx
```

## 串口协议

串口参数：

- 波特率：9600
- 数据位：8
- 校验位：None
- 停止位：1
- 电平：3.3V TTL

常用命令：

```text
@STATUS?
@VERSION?
@TH=2400
@ALARM=ON
@ALARM=OFF
@ALARM=AUTO
@MOTOR=80
```

示例回复：

```text
$OK
$OK,TH=2400
$ERR=UNKNOWN_CMD
$ERR=TH_RANGE
$VERSION=1.0.0
$STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0,MODE=AUTO
```

## 使用方法

1. 使用 Keil 打开 `基于STM32的环境检测报警系统/project.uvprojx`。
2. 编译工程并下载到 STM32F103 开发板。
3. 连接 USB-TTL：STM32 PA9(TX) 接 USB-TTL RXD，PA10(RX) 接 USB-TTL TXD，GND 共地。
4. 打开 `STM32_UpperComputer/run_upper.bat` 启动 Python 上位机。
5. 选择对应 COM 口后，进行状态查询、阈值设置和报警控制。

## 版本管理说明

仓库保留源码、Keil 工程文件、项目说明文档、PPT、协议文档和 `project.hex` 固件文件。Keil 编译生成的 `.o`、`.crf`、`.map`、`.lst`、`.axf` 等临时文件已通过 `.gitignore` 忽略。
