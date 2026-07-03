# IMU 与蓝牙模块阶段总结

更新时间：2026-07-04  
主控：MSPM0G3507  
电机驱动：AT8236 双路直流电机驱动板

## 1. 当前串口资源分配

| 用途 | MSPM0 外设 | TX | RX | 波特率 |
|---|---|---|---|---|
| USB-TTL 调试输出 | UART0 | PA10 | PA11 | 115200 |
| IMU | UART1 | PA17 | PA18 | 9600 / 115200 自动检测 |
| HC-06D 蓝牙 | UART2 | PA21 | PA22 | 9600 |

三个模块使用独立 UART，避免调试输出、IMU 二进制帧和蓝牙命令互相干扰。

## 2. IMU 模块

### 2.1 硬件连接

| IMU 引脚 | MSPM0G3507 | 说明 |
|---|---|---|
| VCC | 3V3 | 模块支持 3.3V 供电 |
| GND | GND | 必须与主控及电机驱动板共地 |
| TX | PA18 / UART1_RX | IMU 向主控发送数据 |
| RX | PA17 / UART1_TX | 主控发送配置及校准命令 |

### 2.2 通信协议

IMU 使用 5 字节二进制帧，不是 ASCII 文本。

角速度帧：

```text
5A AA WzL WzH SUM
```

航向角帧：

```text
5A BB YawL YawH SUM
```

校验方式：

```text
SUM = 低8位(帧头 + 类型 + 数据低字节 + 数据高字节)
```

当前换算公式：

```c
gyro_z_dps = raw_wz  * 2000.0f / 32768.0f;
yaw_deg    = raw_yaw *  180.0f / 32768.0f;
```

数据手册中的陀螺仪量程参数与协议公式存在矛盾，后续使用已知角速度设备时应进一步验证角速度比例。

### 2.3 软件封装

文件：

```text
Hardware/IMU.c
Hardware/IMU.h
```

主要接口：

```c
void IMU_Init(void);
void IMU_GetData(IMU_Data *data);
bool IMU_IsReceiving(void);
bool IMU_SetHostBaudRate(uint32_t baud_rate);

float GyroZ(void);
float Yaw(void);

void IMU_ZeroYaw(void);
void IMU_StartBiasCalibration(void);
void IMU_SaveConfiguration(void);
```

UART1 中断负责接收并解析数据帧。解析器具备帧头同步、类型检查和校验和检查功能。主程序通过 `IMU_GetData()` 获取数据，不直接访问中断中的内部变量。

`GyroZ()` 和 `Yaw()` 是为现有控制模块保留的兼容接口，实际数据仍由独立的 IMU 模块管理。

### 2.4 已完成测试

静止测试数据示例：

```text
Wz(dps)=0.000, Yaw(deg)=0.521,
gyro_frames=363, yaw_frames=363, crc_err=0
```

测试结论：

1. UART1 接收正常。
2. 角速度帧和航向角帧均可稳定解析。
3. 校验错误数量为 0。
4. 静止时角速度为 0，Yaw 保持稳定。
5. 静止时 Yaw 不一定恰好为 0，示例中的 0.521° 是初始参考偏差，不代表模块发生移动。

`gyro_frame_count` 和 `yaw_frame_count` 是累计有效帧数量，不是角速度和角度。

### 2.5 使用注意事项

1. IMU 应刚性固定，Z 轴保持竖直。
2. 安装位置尽量远离电机、电机线及 AT8236 大电流走线。
3. 启动时建议使用软件零点：记录启动时 Yaw，之后计算相对角度。
4. 不建议每次上电都执行硬件保存命令，避免频繁写入模块 Flash。
5. 零偏校准期间模块必须保持静止，校准过程约需 20 秒。

## 3. HC-06D 蓝牙模块

### 3.1 硬件连接

| HC-06D 引脚 | MSPM0G3507 | 说明 |
|---|---|---|
| VCC | 5V | 适用于带稳压底板的 HC-06D |
| GND | GND | 必须与主控共地 |
| TXD | PA22 / UART2_RX | 蓝牙向主控发送数据 |
| RXD | PA21 / UART2_TX | 主控向蓝牙发送数据 |
| KEY | 不接 | 普通透明传输不需要 |
| STATE | 不接 | 后续可选接 GPIO 检测连接状态 |

如果使用的是没有稳压底板的裸模块，应根据模块本体规格使用 3.3V 供电，不能直接接 5V。

### 3.2 通信配置

```text
UART2
9600 baud
8 data bits
No parity
1 stop bit
No flow control
```

HC-06D 使用经典蓝牙 SPP 透明串口。蓝牙连接成功只表示无线链路建立，UART 的接线、共地和波特率仍需分别正确。

### 3.3 软件封装

文件：

```text
Hardware/Bluetooth.c
Hardware/Bluetooth.h
```

主要接口：

```c
void Bluetooth_Init(void);
bool Bluetooth_ReadByte(uint8_t *byte);
void Bluetooth_SendByte(uint8_t byte);
void Bluetooth_SendString(const char *text);
uint32_t Bluetooth_GetReceivedCount(void);
uint32_t Bluetooth_GetOverflowCount(void);
```

UART2 中断只负责将收到的数据放入 128 字节环形缓冲区，不执行 `printf()`、字符串解析或电机控制。主循环通过 `Bluetooth_ReadByte()` 读取数据。

当缓冲区已满时，新字节会被丢弃，并增加溢出计数，避免覆盖尚未处理的数据。

### 3.4 已完成测试

当前 `main.c` 为蓝牙回显测试程序：

1. HC-PC 通过蓝牙连接 HC-06D。
2. HC-PC 发送字符或文本。
3. MSPM0 通过 UART2 收到数据。
4. 收到的数据通过 UART0 输出到 XCOM。
5. MSPM0 将原数据通过 UART2 回传到 HC-PC。

实际测试中，发送字符 `1` 后：

- XCOM 能显示 `1`，说明蓝牙到主控的接收链路正常。
- HC-PC 接收区能显示回传的 `1`，说明主控到蓝牙的发送链路正常。
- 无乱码，说明 9600 波特率配置正确。

因此当前蓝牙双向透明传输已经打通。

### 3.5 后续命令协议建议

当前阶段只进行字节回显，不直接控制电机。后续建议使用以换行符结束的 ASCII 命令：

```text
STOP
MOVE,200,200
MOVE,-200,-200
YAW?
IMU?
ENCODER?
```

建议增加以下安全机制：

1. 只有收到完整并合法的命令后才执行。
2. `STOP` 命令具有最高优先级。
3. 对左右轮 PWM 参数进行范围限制。
4. 蓝牙控制超时后自动停车，建议超时时间为 500ms。
5. 蓝牙刚连接或主控刚启动时保持电机停止。
6. 非法命令返回 `ERR`，合法命令返回 `OK`。

## 4. 当前阶段结论

IMU 与 HC-06D 均已使用独立 UART 完成底层接入：

- UART0 用于电脑串口调试。
- UART1 用于 IMU 二进制数据和配置命令。
- UART2 用于 HC-06D 蓝牙透明传输。

两个模块均已独立封装，不依赖电机和编码器实现。IMU 已完成静止数据验证，蓝牙已完成双向回显验证。下一阶段可以在保持底层驱动不变的前提下，实现蓝牙命令解析、状态查询以及安全的电机控制状态机。
