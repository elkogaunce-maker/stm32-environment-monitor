# STM32 环境监测上位机协议

串口参数：

- 波特率：9600
- 数据位：8
- 校验位：None
- 停止位：1
- 电平：3.3V TTL
- 连接：STM32 PA9(TX) -> USB-TTL RXD，STM32 PA10(RX) -> USB-TTL TXD，GND 共地

## 上位机发送格式

每条命令以 `@` 开始，以 `\r\n` 结束。

```text
@STATUS?\r\n
@VERSION?\r\n
@TH=2400\r\n
@ALARM=ON\r\n
@ALARM=OFF\r\n
@MOTOR=80\r\n
```

## STM32 回复格式

```text
$OK\r\n
$OK,TH=2400\r\n
$ERR=UNKNOWN_CMD\r\n
$ERR=TH_RANGE\r\n
$VERSION=1.0.0\r\n
$STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0\r\n
```

## 建议先实现的 STM32 命令

| 命令 | STM32 行为 | 回复示例 |
|---|---|---|
| `@STATUS?` | 查询当前状态 | `$STATUS,ADC=2860,MV=2304,TH=2400,ALARM=0` |
| `@VERSION?` | 查询固件版本 | `$VERSION=1.0.0` |
| `@TH=2400` | 设置报警阈值，单位 mV | `$OK,TH=2400` |
| `@ALARM=ON` | 手动打开报警 | `$OK,ALARM=ON` |
| `@ALARM=OFF` | 手动关闭报警 | `$OK,ALARM=OFF` |
| `@MOTOR=80` | 设置电机速度 | `$OK,MOTOR=80` |

## 调试顺序

1. 先在 STM32 上电后发送 `$VERSION=1.0.0\r\n`，确认 TX 通路正常。
2. 用串口助手手动发送 `@VERSION?\r\n`，确认 STM32 能收到命令并回复。
3. 再打开 `run_upper.bat`，选择 COM 口，连接。
4. 点击“查询版本”和“查询状态”。
5. 确认无误后再测试“设置阈值”“报警开/关”“设置电机”。
