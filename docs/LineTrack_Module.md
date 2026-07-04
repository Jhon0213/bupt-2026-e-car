# 灰度循迹模块设计文档

## 1. 模块定位

灰度循迹模块用于实现基于 8 路灰度辅助板的黑线循迹功能。当前小车平台基于 MSPM0G3507 主控、AT8236 电机驱动、带编码器直流电机和 8 路灰度辅助板。

当前阶段目标是验证以下链路是否可靠：

```text
灰度辅助板 DAT/CLK 数字读取 -> 黑线位置 black mask -> error 计算 -> 左右轮差速修正
```

当前阶段只做**低速开环循迹**，暂不引入复杂控制算法或额外传感器融合。

当前阶段不使用：

- 编码器速度闭环
- IMU 航向闭环
- 完整 PID 控制
- 摄像头
- ADC 自动校准
- S0/S1/S2 + OUT 模拟采集
- 丢线搜索
- 路口识别

这样做的目的是先确认灰度数据方向、黑线极性、error 符号、左右轮差速方向是否全部正确，避免在基础链路未验证时直接叠加 PID 和速度闭环导致问题难以定位。

## 2. 当前硬件连接

### 灰度辅助板连接

| 灰度辅助板信号 | MSPM0G3507 引脚 | 说明 |
|---|---|---|
| DAT | PB8 | 灰度辅助板串行数据输入到主控 |
| CLK | PB9 | 主控输出时钟给灰度辅助板 |
| VCC | 3V3 | 辅助板电源 |
| GND | GND | 辅助板地 |

### 调试串口连接

| MSPM0G3507 信号 | 外部连接 | 说明 |
|---|---|---|
| PA10 / UART0_TX | USB-TTL RXD | 主控 printf 输出 |
| GND | USB-TTL GND | 串口共地 |

串口参数：

```text
波特率：115200
数据位：8
校验位：None
停止位：1
格式：115200 8N1
```

## 3. 灰度辅助板输出逻辑

灰度辅助板通过 DAT/CLK 输出 8 路数字状态。程序中 `raw` 表示从辅助板直接读取到的原始 8 bit 数据。

当前实测结论：

- `raw=0xFF` 表示全白
- `raw=0x00` 表示全黑
- 辅助板原始输出逻辑为：白色 = 1，黑色 = 0
- 因此程序中使用 `black = ~raw`
- `GRAY_BLACK_IS_1` 应设置为 `0`
- 白底时 `black=0x00`，`lost=1`
- 全黑时 `black=0xFF`，`lost=0`

推荐统一使用 `black` 表示“黑线有效掩码”：

```text
black 中某一 bit 为 1：该通道检测到黑线
black 中某一 bit 为 0：该通道未检测到黑线
```

示例：

| raw | black | 场景说明 |
|---|---|---|
| `0xFF` | `0x00` | 全白，未检测到黑线 |
| `0x00` | `0xFF` | 全黑，8 路均检测到黑线 |
| `0xE7` | `0x18` | 中间黑线，bit3/bit4 检测到黑线 |
| `0xFE` | `0x01` | 一侧黑线，bit0 检测到黑线 |
| `0x7F` | `0x80` | 另一侧黑线，bit7 检测到黑线 |

## 4. 已验证的 error 方向

当前 `error` 使用 8 路加权平均计算。权重定义为：

```c
static const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};
```

含义：

| 通道 | bit | 权重 |
|---|---:|---:|
| CH0 | bit0 | -7 |
| CH1 | bit1 | -5 |
| CH2 | bit2 | -3 |
| CH3 | bit3 | -1 |
| CH4 | bit4 | 1 |
| CH5 | bit5 | 3 |
| CH6 | bit6 | 5 |
| CH7 | bit7 | 7 |

方向约定：

- 左边黑线：`error < 0`
- 中间黑线：`error ≈ 0`
- 右边黑线：`error > 0`

当多个通道同时检测到黑线时，`error` 取所有黑线通道权重的平均值。例如：

- `black=0x18`，bit3/bit4 有效，权重为 `-1` 和 `1`，平均值约为 `0`
- `black=0x03`，bit0/bit1 有效，权重为 `-7` 和 `-5`，平均值为 `-6`
- `black=0xC0`，bit6/bit7 有效，权重为 `5` 和 `7`，平均值为 `6`

## 5. Gray 模块接口说明

当前 Gray 模块提供以下接口。

### `void Gray_Init(void);`

初始化灰度模块内部状态，并将 CLK 输出保持为低电平。

建议在 `board_init()` 之后调用。

### `void Gray_Update(void);`

执行一次 DAT/CLK 串行读取，更新内部灰度数据、黑线掩码、error 和 lost 状态。

每调用一次 `Gray_Update()`，就读取一次 8 bit 灰度状态。

### `uint8_t Gray_GetRaw(void);`

返回辅助板原始串行读取值，即 `raw`。

当前实测：

- `raw=0xFF` 表示全白
- `raw=0x00` 表示全黑

### `uint8_t Gray_GetDigital(void);`

返回当前灰度原始数字值。当前语义与 `Gray_GetRaw()` 等价，主要用于兼容早期代码。

### `uint8_t Gray_GetBlackMask(void);`

返回程序正式使用的黑线掩码 `black`。

当前由于辅助板逻辑为白色 = 1、黑色 = 0，因此：

```c
black = (uint8_t)(~raw);
```

`black` 中 bit 为 1 表示对应通道检测到黑线。

### `uint8_t Gray_GetBlackMask_ActiveLow(void);`

极性诊断接口。

假设辅助板逻辑为白色 = 1、黑色 = 0，则返回：

```c
(uint8_t)(~raw)
```

### `uint8_t Gray_GetBlackMask_ActiveHigh(void);`

极性诊断接口。

假设辅助板逻辑为白色 = 0、黑色 = 1，则返回：

```c
raw
```

### `int16_t Gray_GetError(void);`

返回黑线位置偏差。

- `error < 0`：黑线偏左
- `error ≈ 0`：黑线居中
- `error > 0`：黑线偏右

当前 `error` 基于 `black` 计算，不直接基于 `raw` 计算。

### `uint8_t Gray_IsLost(void);`

返回是否丢线。

当前规则：

```c
black == 0 -> lost = 1
black != 0 -> lost = 0
```

即没有任何通道检测到黑线时，认为丢线。

## 6. LineTrack 模块设计

建议后续新增循迹模块文件：

```text
Hardware/LineTrack.c
Hardware/LineTrack.h
```

建议对外接口：

```c
void LineTrack_Init(void);
void LineTrack_Update(void);
void LineTrack_Stop(void);
```

接口职责：

| 接口 | 职责 |
|---|---|
| `LineTrack_Init()` | 初始化灰度模块和电机模块，确保上电后电机停止 |
| `LineTrack_Update()` | 周期调用，读取灰度、计算 error、输出左右轮 PWM |
| `LineTrack_Stop()` | 立即停车 |

第一版 `LineTrack_Update()` 只做低速开环差速控制，不做编码器闭环和完整 PID。

## 7. 低速开环循迹控制逻辑

第一版循迹使用比例差速控制。

核心公式：

```c
turn = LINE_KP * error;

left_pwm  = LINE_BASE_LEFT  + turn;
right_pwm = LINE_BASE_RIGHT - turn;
```

方向解释：

### `error < 0`：黑线在左边

此时：

```text
turn < 0
left_pwm  = LINE_BASE_LEFT  + turn  -> 左轮变慢
right_pwm = LINE_BASE_RIGHT - turn  -> 右轮变快
```

左轮慢、右轮快，小车向左修正。

### `error > 0`：黑线在右边

此时：

```text
turn > 0
left_pwm  = LINE_BASE_LEFT  + turn  -> 左轮变快
right_pwm = LINE_BASE_RIGHT - turn  -> 右轮变慢
```

左轮快、右轮慢，小车向右修正。

### `error ≈ 0`：黑线在中间

此时 `turn ≈ 0`，左右轮接近基础 PWM，小车直行。

## 8. 初始参数建议

第一版低速开环循迹建议参数：

```c
#define LINE_BASE_LEFT   380
#define LINE_BASE_RIGHT  280
#define LINE_KP          20
#define LINE_PWM_MIN     0
#define LINE_PWM_MAX     500
```

参数说明：

| 参数 | 说明 |
|---|---|
| `LINE_BASE_LEFT` | 左轮基础 PWM |
| `LINE_BASE_RIGHT` | 右轮基础 PWM |
| `LINE_KP` | error 到差速修正量的比例系数 |
| `LINE_PWM_MIN` | PWM 输出下限 |
| `LINE_PWM_MAX` | PWM 输出上限 |

当前已知开环测试结果：

- 右轮明显比左轮快
- 左轮低 PWM 启动阈值较高
- 左轮大约 PWM = 300 后才明显启动

因此第一版建议左轮基础 PWM 大于右轮基础 PWM：

```c
LINE_BASE_LEFT  > LINE_BASE_RIGHT
```

这样可以抵消左右轮机械差异，避免小车在 `error≈0` 时明显向一侧跑偏。

## 9. LineTrack_Update 核心代码框架

以下代码为建议框架，后续可放入 `Hardware/LineTrack.c`。

```c
#include "LineTrack.h"
#include "Gray.h"
#include "Motor.h"

#define LINE_BASE_LEFT   380
#define LINE_BASE_RIGHT  280
#define LINE_KP          20
#define LINE_PWM_MIN     0
#define LINE_PWM_MAX     500

static int16_t LineTrack_LimitPWM(int16_t pwm)
{
    if (pwm < LINE_PWM_MIN) {
        return LINE_PWM_MIN;
    }

    if (pwm > LINE_PWM_MAX) {
        return LINE_PWM_MAX;
    }

    return pwm;
}

void LineTrack_Init(void)
{
    Gray_Init();
    Motor_Init();
    move(0, 0);
}

void LineTrack_Stop(void)
{
    move(0, 0);
}

void LineTrack_Update(void)
{
    int16_t error;
    int16_t turn;
    int16_t left_pwm;
    int16_t right_pwm;

    Gray_Update();

    if (Gray_IsLost()) {
        move(0, 0);
        return;
    }

    error = Gray_GetError();
    turn = LINE_KP * error;

    left_pwm  = LINE_BASE_LEFT  + turn;
    right_pwm = LINE_BASE_RIGHT - turn;

    left_pwm  = LineTrack_LimitPWM(left_pwm);
    right_pwm = LineTrack_LimitPWM(right_pwm);

    move(left_pwm, right_pwm);
}
```

注意：第一版不做丢线搜索。如果 `Gray_IsLost()` 返回 1，直接停车。

## 10. main.c 测试模式建议

建议后续在 `main.c` 中保留应用模式宏，便于在灰度调试和循迹测试之间切换。

```c
#define APP_MODE_GRAY_DEBUG       1
#define APP_MODE_LINE_TRACK_TEST  2
#define APP_MODE APP_MODE_LINE_TRACK_TEST
```

### 灰度调试模式框架

```c
board_init();
Gray_Init();

while (1)
{
    Gray_Update();

    printf("raw=0x%02X black=0x%02X error=%d lost=%d\r\n",
           Gray_GetRaw(),
           Gray_GetBlackMask(),
           Gray_GetError(),
           Gray_IsLost());

    delay_ms(200);
}
```

用途：

- 验证 DAT/CLK 读取是否稳定
- 验证 `raw` 和 `black` 极性是否正确
- 验证 `error` 方向是否符合预期
- 验证白底丢线是否为 `lost=1`

### 循迹测试模式框架

```c
board_init();
LineTrack_Init();

while (1)
{
    LineTrack_Update();

    printf("black=0x%02X error=%d lost=%d\r\n",
           Gray_GetBlackMask(),
           Gray_GetError(),
           Gray_IsLost());

    delay_ms(20);
}
```

用途：

- 低速测试左右轮差速修正
- 观察黑线位置与 error 是否对应
- 观察丢线时是否停车

## 11. 安全测试流程

### 第一轮：车轮架空测试

先将小车架空，避免参数错误导致小车突然冲出。

测试项目：

| 场景 | 期望现象 |
|---|---|
| 白底 | `lost=1`，两轮停止 |
| 黑线中间 | 左右轮都前进 |
| 黑线左侧 | 左轮变慢，右轮变快 |
| 黑线右侧 | 左轮变快，右轮变慢 |

如果发现修正方向反了，例如黑线在左边时小车却向右偏，应将公式改为：

```c
left_pwm  = LINE_BASE_LEFT  - turn;
right_pwm = LINE_BASE_RIGHT + turn;
```

或者检查 `error` 权重方向是否与实际传感器安装方向相反。

### 第二轮：地面低速测试

架空测试正确后，再进行地面低速测试。

测试要求：

- 只看小车是否能朝黑线方向修正
- 不追求速度
- 不追求平滑
- 丢线时必须停车
- 如果小车冲出或抖动严重，应立即减小基础 PWM 或 `LINE_KP`

## 12. 参数调试方法

### 小车太慢

适当增大：

```c
LINE_BASE_LEFT
LINE_BASE_RIGHT
```

由于左轮启动阈值较高，优先确保左轮基础 PWM 高于启动阈值。

### 小车太快

适当减小：

```c
LINE_BASE_LEFT
LINE_BASE_RIGHT
```

第一版建议宁可慢，不要快。

### 修正不明显

增大：

```c
LINE_KP
```

表现为黑线偏到一侧时，小车转向修正太弱，仍继续偏离。

### 抖动严重

减小：

```c
LINE_KP
```

表现为小车左右摆动明显，或者在黑线附近频繁大幅修正。

### 总是偏向一侧

调整左右基础 PWM：

- 如果小车直线时总向左偏，说明右轮相对过快或左轮相对过慢，可增大左轮 base 或减小右轮 base。
- 如果小车直线时总向右偏，说明左轮相对过快或右轮相对过慢，可减小左轮 base 或增大右轮 base。

结合当前已知结果：

- 右轮明显比左轮快
- 左轮低 PWM 启动阈值较高

因此初始参数设置为：

```c
#define LINE_BASE_LEFT   380
#define LINE_BASE_RIGHT  280
```

即左轮 base 更大，右轮 base 更小。

## 13. 当前阶段不建议加入的功能

第一阶段不建议加入以下功能：

- 编码器速度闭环
- IMU 航向闭环
- 完整 PID
- 丢线搜索
- 转圈找线
- 路口识别
- 停车点识别
- 云台 / 机械臂动作
- 复杂任务状态机

原因：当前首要目标是验证基础链路：

```text
灰度 black mask -> error -> 左右轮差速修正
```

如果此链路尚未稳定，就加入速度闭环、IMU、PID 或任务状态机，会导致问题耦合，难以判断到底是灰度方向、error 符号、电机方向、PWM 参数还是控制算法的问题。

## 14. 第一版循迹完成标准

第一版低速开环循迹完成标准：

1. 白底 `lost=1`，小车停车。
2. 黑线中间 `error≈0`，小车直行。
3. 黑线左侧 `error<0`，小车向左修正。
4. 黑线右侧 `error>0`，小车向右修正。
5. 小车能低速沿直线黑线行驶。
6. 小车轻微偏离黑线后，能重新修正回来。

达到以上标准后，再考虑进入第二阶段控制优化。

## 15. 后续升级方向

建议按以下顺序逐步升级，不要一次性叠加多个复杂功能。

### 第一阶段：低速开环循迹

目标：验证灰度 error 到左右轮差速修正链路。

特点：

- 不使用编码器闭环
- 不使用 IMU
- 不做完整 PID
- 丢线直接停车

### 第二阶段：循迹 PD 控制

目标：在 P 控制基础上加入 D 项，减小左右摆动。

可能形式：

```c
turn = KP * error + KD * (error - last_error);
```

### 第三阶段：编码器速度 PI

目标：解决左右轮实际速度不一致问题，使给定 PWM 或目标速度更稳定。

重点：

- 左右轮分别做速度闭环
- 先单独调好电机速度环，再叠加循迹外环

### 第四阶段：路口 / 停车点识别

目标：利用 `black` 模式识别路口、横线、停止线等特殊图案。

例如：

- 多路连续黑线可能表示横线
- 全黑可能表示停止区或特殊标志
- 左右大面积黑线可能表示路口

### 第五阶段：IMU 辅助直线稳定

目标：在长直线或特殊区域中使用 IMU 航向角辅助稳定方向。

注意：IMU 不应替代灰度循迹，而是辅助某些场景。

### 第六阶段：任务动作状态机

目标：将循迹、停车、转向、云台或机械臂动作组织成完整任务流程。

建议在前面阶段稳定后再做。
