# STM32 环境监测上位机协议

## 串口参数

- 波特率：9600
- 数据位：8
- 校验位：None
- 停止位：1
- 电平：3.3V TTL
- 连接：STM32 PA9(TX) -> USB-TTL RXD，STM32 PA10(RX) -> USB-TTL TXD，GND 共地

## 帧格式

上位机发送命令以 `@` 开始，以 `\r\n` 结束。

```text
@STATUS?\r\n
@VERSION?\r\n
@TH=2400\r\n
@ALARM=ON\r\n
@ALARM=OFF\r\n
@ALARM=AUTO\r\n
@MOTOR=80\r\n
```

STM32 回复以 `$` 开始，以 `\r\n` 结束。

```text
$VERSION=1.0.0\r\n
$STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0,MODE=AUTO\r\n
$OK,TH=2400\r\n
$OK,ALARM=ON\r\n
$OK,ALARM=OFF\r\n
$OK,ALARM=AUTO\r\n
$OK,MOTOR=080\r\n
$ERR=UNKNOWN_CMD\r\n
```

## 命令说明

| 命令 | 参数范围 | STM32 行为 | 回复示例 |
|---|---|---|---|
| `@STATUS?` | 无 | 查询当前状态 | `$STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0,MODE=AUTO` |
| `@VERSION?` | 无 | 查询固件版本 | `$VERSION=1.0.0` |
| `@TH=2400` | 0-3300，单位 mV | 设置报警阈值，并恢复自动报警模式 | `$OK,TH=2400` |
| `@ALARM=ON` | 无 | 手动打开报警 | `$OK,ALARM=ON` |
| `@ALARM=OFF` | 无 | 手动关闭报警 | `$OK,ALARM=OFF` |
| `@ALARM=AUTO` | 无 | 恢复传感器阈值自动判断 | `$OK,ALARM=AUTO` |
| `@MOTOR=80` | -100 到 100 | 设置报警时电机速度 | `$OK,MOTOR=080` |

## 状态字段

| 字段 | 含义 | 示例 |
|---|---|---|
| `ADC` | ADC 滤波后的原始采样值，范围 0-4095 | `ADC=2860` |
| `MV` | ADC 换算后的电压值，单位 mV | `MV=2304` |
| `TH` | 当前报警阈值，单位 mV | `TH=2400` |
| `ALARM` | 当前报警输出状态，`1` 表示报警，`0` 表示正常 | `ALARM=0` |
| `MODE` | 报警控制模式：`AUTO`、`MANUAL_ON`、`MANUAL_OFF` | `MODE=AUTO` |

## 错误码

| 错误码 | 触发条件 | 示例 |
|---|---|---|
| `TH_FORMAT` | 阈值参数不是纯数字 | `@TH=abc` |
| `TH_RANGE` | 阈值超过 0-3300 mV | `@TH=5000` |
| `MOTOR_FORMAT` | 电机速度参数不是整数 | `@MOTOR=abc` |
| `MOTOR_RANGE` | 电机速度超过 -100 到 100 | `@MOTOR=120` |
| `UNKNOWN_CMD` | 命令无法识别 | `@HELLO` |

## 调试顺序

1. STM32 上电后确认串口输出 `$VERSION=1.0.0\r\n`。
2. 用串口助手发送 `@VERSION?\r\n`，确认 STM32 回复版本。
3. 发送 `@STATUS?\r\n`，确认能收到包含 `MODE` 字段的状态帧。
4. 测试 `@TH=2400\r\n`，确认阈值可设置，超范围会返回 `$ERR=TH_RANGE`。
5. 测试 `@ALARM=ON\r\n`、`@ALARM=OFF\r\n`、`@ALARM=AUTO\r\n`，确认模式切换正常。
6. 测试 `@MOTOR=80\r\n`、`@MOTOR=-80\r\n`，确认电机速度可设置。
7. 测试异常命令，例如 `@MOTOR=abc\r\n`，确认返回 `$ERR=MOTOR_FORMAT`。
