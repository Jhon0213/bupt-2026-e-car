# BUPT 2026 E-Car 底盘与循迹项目状态

> 仓库：`Jhon0213/bupt-2026-e-car`
> 工作分支：`main`
> 架构地图审阅基线：`c22ea4d4a201171e451f3c7d3c7a7031812bc107`
> 当前分析基线：`cc273286d0c12cb5bb79760eefded70338daecb1`
> 本文件性质：动态状态入口。每次开始新的 GPT / Codex 会话时优先读取本文件。

---

## 1. 当前阶段

当前只处理小车底盘、循迹、里程计和任务实时性，不处理滚球控制。

当前主目标：

1. 消除直线循迹时的周期性左右摇摆；
2. 实现半圆入弯、弯中、出弯的连续丝滑控制；
3. 校准编码器与里程计，使其可靠支持路段识别；
4. 使用“路段状态 + 里程窗口 + A 点横向启停线”实现精准停车；
5. 确认设计为 10 ms 的控制任务是否真实按期执行；
6. 在行为和数据明确后，渐进式重构代码结构。

---

## 2. 当前实际运行链路

当前 `main.c` 编译模式：

```c
#define SELECTED_TASK_MODE TASK_MODE_OLED_KEY_TEST
```

当前烧录后的业务路径：

```text
main
  ↓
RobotPlatform_Init
  ↓
OledKeyTest_Run
  ↓
Task3_LinkedOperation_StartMode / Update
  ↓
GrayTrack_Update
  ↓
SpeedPI_Update
  ↓
Motor.move
```

编码器反馈路径：

```text
编码器边沿中断
  ↓
左右累计计数
  ↓
TIMER_0 每 10 ms 计算速度
  ↓
5 点滑动平均
  ↓
SpeedPI 实际转速反馈
```

当前路线分段路径：

```text
左右编码器累计距离
  ↓
Task3_LinkedOperation
  ↓
AB / BC / CD / DA 路段判断
  ↓
直线 / 预弯 / 弯道参数切换
```

---

## 3. 当前工作基线决策

### 3.1 临时唯一主控制路径

底盘与循迹调试阶段，暂时将以下路径作为唯一正式实验基线：

```text
OledKeyTest
→ Task3_LinkedOperation
→ GrayTrack
→ SpeedPI
→ Motor
```

原因：

- 当前 OLED 菜单能够直接启动；
- 已包含一圈、B+5 cm、分段控制和日志；
- 与当前最需要解决的直线、半圆和里程问题直接对应。

### 3.2 暂不并行修改的路径

以下代码暂时只作为参考，不与当前实验链路同时修改：

```text
RouteNavigator
legacy Hardware/CONTROL/control.c
TaskBonus1_LaserTrace
TaskBonusFourLap
```

避免三套控制体系同时变化，导致参数、方向、停车和状态机结论无法归因。

### 3.3 长期方向

当前基线稳定后，再决定：

- 保留 `Task3_LinkedOperation` 并重构；
- 将其路段逻辑合并进 `RouteNavigator`；
- 抽取统一的底盘控制、里程计和路线状态机。

现阶段不进行一次性大重构。

---

## 4. 已确认的架构事实

### 4.1 10 ms 定时器不等于 10 ms 控制闭环

`TIMER_0` 每 10 ms：

- 更新时间；
- 计算编码器速度；
- 产生控制 tick。

但 `GrayTrack → SpeedPI → Motor` 实际运行在主循环中的 `Task3_LinkedOperation_Update()`，不是在定时器 ISR 内执行。

因此：

```text
10 ms 定时器稳定
≠
控制算法每 10 ms 准时完成
```

主循环中的阻塞操作仍可能造成控制周期变为 15 ms、20 ms 或更长。

### 4.2 Task3 存在过期周期丢弃

`Task3_LinkedOperation_Update()`：

- 最多补执行 3 个过期控制步；
- 若仍然落后，会重置下一次执行时间；
- 旧控制周期会被主动丢弃。

这会导致：

- 控制周期不均匀；
- `SpeedPI` 固定使用 `dt = 0.01 s` 时产生参数失真；
- 路段切换和里程行为与设计不一致；
- 实车波形可能出现偶发大幅修正。

### 4.3 编码器速度反馈有约 50 ms 滤波窗口

编码器速度采用最近 5 个 10 ms 样本滑动平均。

优点：

- 降低低速计数量化噪声。

风险：

- 增加速度反馈延迟；
- 与 PWM 斜率限制、上层目标斜率限制叠加；
- 入弯、出弯和急停响应可能明显滞后。

### 4.4 当前里程分段依赖固定模型

Task3 当前假设包括：

```text
轮径约 6.5 cm
编码器约 14000 脉冲 / 车轮一圈
直线 150 cm
弯道半径 50 cm
```

并通过编码器累计值估算 AB / BC / CD / DA。

未经独立标定前，这些值不能作为高精度终点依据。

### 4.5 当前存在阻塞风险

已知阻塞来源：

- 灰度软件 I2C；
- OLED 软件 I2C；
- 星闪逐字节阻塞发送；
- `delay_ms()` 忙等待；
- 50 ms 一次的长 CSV 调试输出；
- 格式化字符串和串口等待。

115200 baud 下，约 100 字节数据需要约 8.7 ms 的线路时间。长 CSV 很可能已经接近或超过一个 10 ms 控制周期。

### 4.6 `Motor_Brake()` 当前实际为滑行

当前实现语义：

```c
Motor_Brake()
{
    Motor_Coast();
}
```

因此：

- “刹车”实际上是高阻滑行；
- 停车距离和速度、电池、路面高度相关；
- 当前提前停车参数可能是在补偿滑行距离；
- 精准停车前必须明确真正的制动策略。

### 4.7 SysConfig 源配置与生成文件存在漂移

`empty.syscfg` 中灰度配置与当前实际生成头文件的引脚定义不一致。

在重新运行 SysConfig 前必须先修正，否则可能覆盖当前可运行配置。

---

## 5. 三个主要问题的实际耦合关系

```text
主循环阻塞 / 周期抖动
    ↓
速度 PI 的实际 dt 错误
    ↓
左右轮响应不一致和控制输出突变
    ↓
直线摇摆、入弯过冲、出弯反向修正
    ↓
轮胎侧滑和累计距离误差
    ↓
AB / BC / CD / DA 切换点漂移
    ↓
终点停车误差
```

因此不得直接跳到“调循迹 PID”或“重写状态机”。

---

## 6. 当前优先级

### P0：接口与基线确认

- [ ] 确认真正的电机方向和左右轮映射；
- [ ] 明确 `Motor_Brake`、`Motor_Coast` 的实际硬件行为；
- [ ] 暂停重新生成 SysConfig；
- [ ] 确认当前实验只走 `Task3_LinkedOperation`；
- [ ] 记录当前可运行版本 Git 标签。

### P1：实时性审计

- [ ] 测量 Task3 实际控制步周期；
- [ ] 测量最大控制间隔；
- [ ] 统计丢弃控制周期次数；
- [ ] 测量 Gray、SpeedPI、Motor、日志和 OLED 执行时间；
- [ ] 临时关闭或降频长 CSV，比较控制效果。

### P2：编码器与里程计

- [ ] 确认左右轮每转实际脉冲数；
- [ ] 确认 1X / 2X / 4X 计数模式；
- [ ] 确认左右轮符号和方向；
- [ ] 分别标定左右轮距离系数；
- [ ] 完成 1 m、3 m、单半圆、完整一圈测试；
- [ ] 区分比例误差、周期误差和转弯侧滑误差。

### P3：直线循迹

- [ ] 同时采集灰度误差、目标转速、实际转速和 PWM；
- [ ] 关闭循迹后验证左右轮等速直行；
- [ ] 检查灰度位置估计是否离散跳变；
- [ ] 检查循迹输出限幅和变化率；
- [ ] 检查速度滤波延迟对摆动的影响；
- [ ] 逐档提高基础速度验证稳定性。

### P4：半圆丝滑控制

- [ ] 将弯道划分为入弯、稳态弯道、出弯；
- [ ] 使用平滑基础速度和差速目标；
- [ ] 以曲率前馈承担主要转向；
- [ ] 灰度反馈只修正残差；
- [ ] 比较左右两个半圆的一致性；
- [ ] 检查转弯侧滑对里程计的影响。

### P5：终点检测与停车

- [ ] 建立最后直线路段状态；
- [ ] 使用里程计进入 A 点搜索窗口；
- [ ] 稳定识别横向启停线；
- [ ] 建立减速、检测、制动、保持状态；
- [ ] 标定检测点到指定测试点的几何偏移；
- [ ] 连续 10 次验证最大停车误差不超过 2 cm。

### P6：渐进式重构

- [ ] 统一底盘控制入口；
- [ ] 抽取统一 Odometry；
- [ ] 抽取统一 RouteState；
- [ ] 清理或停止编译遗留 `control.c`；
- [ ] 统一日志接口；
- [ ] 将非关键任务与控制任务分频隔离。

---

## 7. 当前第一任务：ARCH-001

### 7.1 问题状态

| 编号 | 问题 | 状态 | 依赖 |
|---|---|---:|---|
| ARCH-001 | 测量控制任务真实周期和抖动 | DONE | 无 |

ARCH-001 已完成 B / C / D 三组实机测量，确认 OLED 全屏软件 I2C 刷新是造成主循环 10 ms 控制任务超周期和积压丢弃的确定性根因。

### 7.2 分析基线

```text
分支：main
Commit：cc273286d0c12cb5bb79760eefded70338daecb1
当前默认编译模式：TASK_MODE_OLED_KEY_TEST
当前默认运行任务：OledKeyTest 正式菜单；ARCH001_CASE_E_REALTIME_TIME 保留为可切换复测默认组
```

### 7.3 已确认事实

1. `TIMER_0` 由硬件配置为 10 ms 周期定时器；
2. `TIMER_0` 中断每 10 ms 更新系统时间、左右编码器速度，并产生软件控制 tick；
3. 当前 OLED 菜单中的 Task3 不消费软件控制 tick，而是在主循环中根据 `board_millis` 调度控制步；
4. 灰度采样、循迹控制、速度 PI 和电机输出均在主循环中的 `Task3_ControlStep` 内执行；
5. Task3 一次最多追赶 3 个过期控制步，剩余积压周期会被丢弃；
6. 当前控制步内每 50 ms 同步发送一次星闪 CSV；
7. OLED 每 500 ms 通过软件 I2C 同步刷新完整 1024 字节帧缓冲；
8. 代码审计只能确认“设计周期为 10 ms”，尚不能确认实际控制周期稳定为 10 ms。

### 7.4 代码审计结论

当前架构存在两套不同的时间概念：

```text
硬件 10 ms 中断：
编码器测速和系统时基

主循环 10 ms 任务：
灰度、循迹、速度 PI 和电机输出
```

硬件定时器准确并不代表主循环控制输出准确。

当前最大的阻塞嫌疑包括：

```text
OLED 全屏软件 I2C 刷新
星闪同步长 CSV 发送
灰度软件 I2C 读取
主循环中的延时和其他同步操作
```

必须通过 GPIO 和逻辑分析仪或示波器测量真实周期，不能仅通过代码推断。

### 7.5 主要风险

```text
1. OLED 全屏刷新按代码时序估算约 225 ms，可能周期性阻塞主循环。
2. Task3 多字段 CSV 在 115200 baud 下可能接近或超过 10 ms。
3. 控制积压时最多补执行 3 次，剩余周期被丢弃。
4. 追赶控制步可能使用同一份最新编码器速度，不代表历史周期被恢复。
5. SpeedPI 和部分状态计时固定假设 dt = 10 ms，发生超周期时参数含义失真。
```

其中 OLED 的 225 ms 当前只是基于代码时序的估算值，必须通过实机波形确认。

### 7.6 下一步测量方案

1. 在 `TIMER_0_INST_IRQHandler` 中增加 `DBG_TICK` 脉冲；
2. 在 `Task3_LinkedOperation_Update()` 调用 `Task3_ControlStep()` 前后增加 `DBG_CTRL` 脉冲；
3. 保持 OLED、调试 CSV 和控制参数不变，先记录当前真实基线；
4. 记录至少 3 秒连续波形，并完成一次完整赛道运行；
5. 统计最小、平均、最大控制周期、控制步执行时间和超周期次数；
6. 增加 catch-up 次数和 backlog 丢弃次数的轻量计数；
7. 禁止在控制周期内直接打印上述统计值；
8. 测试结束或小车停止后，再统一输出统计结果。

### 7.7 推荐对照测试矩阵

#### 测试 A：当前完整负载

```text
OLED：开启
Task3 CSV：开启
控制参数：不变
```

目的：获取当前真实基线。

#### 测试 B：关闭 Task3 CSV

```text
OLED：开启
Task3 CSV：关闭
```

目的：量化星闪同步日志对周期的影响。

#### 测试 C：关闭 OLED 刷新

```text
OLED：关闭刷新
Task3 CSV：开启
```

目的：量化 OLED 软件 I2C 对周期的影响。

#### 测试 D：关闭 OLED 和 CSV

```text
OLED：关闭刷新
Task3 CSV：关闭
```

目的：获得底盘控制链路的最低负载基线。

### 7.8 尚未确认的数据

```text
control_dt_min_us
control_dt_avg_us
control_dt_max_us
control_exec_max_us
control_overrun_count
control_backlog_drop_count
control_catchup_count
```

### 7.9 ARCH-001 完成条件

只有满足以下条件，ARCH-001 才能从 `ANALYZING` 转为 `DONE`：

```text
已取得实机周期波形
已统计最小、平均、最大周期
已测得最大控制步执行时间
已统计超周期次数
已统计追赶和积压丢弃次数
已量化 OLED 和 CSV 对周期的影响
已确认 SpeedPI 的 dt 策略是否需要修改
```

在此之前，不进入大规模 PID 调整、里程计最终标定或 FreeRTOS 迁移。

---
## 7.10 ARCH-001 实测结果与结论

| 测试组 | OLED | CSV | 控制次数 | 平均周期 | 最大周期 | Catch-up | Backlog 丢弃 |
|---|---|---|---:|---:|---:|---:|---:|
| B_NO_CSV | 开 | 关 | 559 | 17.90 ms | 260 ms | 20 | 20 |
| C_NO_OLED | 关 | 开 | 1000 | 10.00 ms | 10 ms | 0 | 0 |
| D_MIN_LOAD | 关 | 关 | 1000 | 10.00 ms | 10 ms | 0 | 0 |

结论：

1. 阻塞式 OLED 全屏刷新是 10 ms 控制任务超周期的确定性根因；
2. 一次完整 OLED 刷新最长约 250 ms；
3. 20 次 OLED 刷新对应 20 次超过 30 ms 周期、20 次 catch-up 和 20 次 backlog 丢弃；
4. 当前 Task3 CSV 在 C_NO_OLED 组未造成控制周期丢失；
5. 关闭 OLED 完整刷新后，裸机主循环结构可以稳定达到 10 ms 控制周期；
6. 当前不迁移 FreeRTOS，优先将运行期 OLED 改为局部、分步、非阻塞刷新。

## 7.11 ARCH-006：OLED 运行期非阻塞实时计时

| 编号 | 问题 | 状态 | 依赖 |
|---|---|---:|---|
| ARCH-006 | OLED 运行期非阻塞实时计时 | DONE | ARCH-001 |
| FORMAL-OLED-001 | 正式 OledKeyTest 空载集成验证 | PENDING_HARDWARE_TEST | ARCH-006 |

设计决策：

1. WAIT / MENU 页面允许继续使用 OLED 全屏刷新；
2. 车辆运行前先绘制静态运行页面并完成一次 `OledDisplay_Update()`；
3. 完整刷新结束后再读取 `board_millis()` 并启动 `Task3_LinkedOperation`，启动前全屏刷新不计入行驶时间；
4. 运行期间禁止 `OledDisplay_Clear()` 和全屏 `OledDisplay_Update()`；
5. 运行期间显示固定宽度整数秒 `000s`，显示频率 1 Hz；
6. 内部计时仍使用 `board_millis()` 和毫秒级 `elapsed_ms`；
7. 每次主循环先执行 `Task3_LinkedOperation_Update()`，再处理 OLED 局部计时；
8. 当前局部刷新采用帧缓冲、dirty 字符检测和列分片发送，每轮最多发送 2 列 OLED 数据；
9. 任务完成或停止时先停止 Task3 和电机，再允许最终全屏刷新；
10. 当前不需要迁移 FreeRTOS。

第一次 E_REALTIME_TIME 实测结果（0.1 秒显示，10 Hz 局部刷新）：

```text
control_count=1000
period_avg_x100_ms=1000
period_max_ms=20
period_0ms_count=35
period_20ms_count=35
catchup_event_count=35
catchup_step_count=35
backlog_drop_event_count=0
csv_send_count=200
oled_refresh_count=0
```

判定：`PARTIAL PASS`。全屏 OLED 阻塞已经消除，10 秒内仍完成 1000 次控制且没有 backlog 丢弃；但 35 组 20ms 周期后立即 0ms 补执行说明 10 Hz 数字局部刷新仍会带来可观测调度抖动。

最终 E_REALTIME_TIME 实测结果（整数秒显示，1 Hz 局部分片刷新）：

| 指标 | 结果 |
|---|---:|
| control_count | 1000 |
| period_sample_count | 999 |
| period_min_ms | 10 |
| period_avg_x100_ms | 1000 |
| period_max_ms | 10 |
| period_0ms_count | 0 |
| period_10ms_count | 999 |
| period_20ms_count | 0 |
| period_30ms_count | 0 |
| period_over_30ms_count | 0 |
| exec_sample_count | 1000 |
| exec_max_ms | 0 |
| exec_0ms_count | 1000 |
| catchup_event_count | 0 |
| catchup_step_count | 0 |
| backlog_drop_event_count | 0 |
| csv_send_count | 200 |
| csv_exec_max_ms | 0 |
| csv_over_10ms_count | 0 |
| oled_refresh_count | 0 |
| oled_exec_max_ms | 0 |
| oled_over_10ms_count | 0 |
| oled_partial_write_count | 27 |
| oled_partial_exec_max_ms | 10 |
| oled_partial_over_10ms_count | 0 |

硬件现象：OLED 整数秒正常显示，星闪端正常收到 CSV；测试开始前串口曾因 TX 接触不良出现乱码，重新接好后数据正常，该乱码属于接线接触问题，不属于程序异常。

最终结论：

1. 整数秒 1 Hz 显示消除了原 0.1 秒显示产生的 35 次 catch-up；
2. OLED 实时显示和 CSV 同时开启时，控制周期保持严格 10 ms；
3. 运行期间全屏刷新次数为 0；
4. 局部 OLED 写入没有超过 10 ms；
5. OLED 整数秒局部显示机制已通过空载硬件时序验收；
6. 正式 `OledKeyTest` 菜单链路已接入该机制，但仍等待用户完成最终空载集成验证。

### 正式 OledKeyTest 空载集成验证

状态：`PENDING_HARDWARE_TEST`

验收内容：

1. 菜单和 K1 / K2 按键正常；
2. 启动前全屏刷新不计入行驶时间；
3. 运行时间从 `000s` 开始；
4. 运行期间每秒整数更新；
5. 运行期间无全屏闪烁；
6. 星闪 CSV 正常；
7. Task3 完成后先停车；
8. 最终页面显示时间和里程正确。

---
## 8. 里程计最终设计原则

### 8.1 距离来源

优先直接累计编码器增量：

```text
left_distance += left_delta_count × K_left
right_distance += right_delta_count × K_right
center_distance += (left_delta_distance + right_delta_distance) / 2
```

不优先使用：

```text
distance += filtered_speed × fixed_0.01
```

原因是后者受到：

- 控制周期抖动；
- 速度滤波延迟；
- 浮点积分；
- 丢控制步

影响。

### 8.2 左右轮独立标定

必须保留：

```text
K_left
K_right
```

不能只通过一个统一轮径参数补偿左右轮差异。

### 8.3 终点判定

最终逻辑：

```text
已通过完整路段状态
AND
累计距离进入终点搜索窗口
AND
A 点横向启停线连续检测成立
    ↓
进入制动状态
```

里程计负责“到达附近”，启停线负责“精确定位”。

---

## 9. 直线与半圆控制原则

### 9.1 直线

优先排查顺序：

```text
实际周期
→ 左右轮速度闭环一致性
→ 灰度位置连续性
→ 循迹增益
→ 微分噪声
→ 输出限幅和变化率
```

### 9.2 半圆

推荐结构：

```text
目标基础速度
+
弯道曲率前馈
+
灰度残差反馈
    ↓
左右目标 RPM
```

状态：

```text
STRAIGHT
ENTER_CURVE
CURVE_STEADY
EXIT_CURVE
```

所有速度和差速切换必须连续，禁止在单个里程点进行阶跃切换。

---

## 10. 当前禁止事项

在 ARCH-001 和里程标定完成前，不做：

- 大规模目录重构；
- 同时调整 GrayTrack 和 SpeedPI 多组参数；
- 切换到 RouteNavigator 作为主线；
- 删除现有 Task3；
- 根据一圈误差直接修改轮径；
- 只凭视频判断控制周期；
- 在控制循环内增加更多 `printf`；
- 重新运行 SysConfig；
- 使用强制推送覆盖现有 Git 历史。

---

## 11. 会话启动提示词

```text
请使用 GitHub 读取 Jhon0213/bupt-2026-e-car 当前指定分支。

首先完整阅读：
- docs/PROJECT_STATE.md
- docs/ARCHITECTURE_MAP.md
- docs/CHASSIS_TRACKING_MASTER_PLAN.md

当前正式实验路径固定为：
OledKeyTest
→ Task3_LinkedOperation
→ GrayTrack
→ SpeedPI
→ Motor

当前问题编号：
ARCH-001

任务：
确认设计为 10 ms 的控制任务实际运行周期，并定位所有可能造成阻塞和过期周期丢弃的代码。

要求：
1. 基于真实代码，不根据 README 推断；
2. 列出定时器 ISR、主循环和 Task3 的完整调用链；
3. 找到控制周期调度、补执行和丢弃逻辑；
4. 分析 Gray 软件 I2C、OLED、StarFlash、delay 和日志的阻塞路径；
5. 给出最小侵入式测量方案；
6. 先不修改控制参数；
7. 先不进行大规模重构；
8. 最后输出需要回写到 PROJECT_STATE.md 的结论。
```

---

## 12. 当前状态摘要

```text
正式实验路径：Task3_LinkedOperation
当前问题：ARCH-001
当前状态：ARCH-001 DONE；ARCH-006 DONE；FORMAL-OLED-001 PENDING_HARDWARE_TEST
当前结论：OLED 全屏刷新会破坏 10 ms 主循环控制周期；关闭全屏刷新后周期稳定
最大嫌疑：已确认 OLED 全屏软件 I2C；当前 CSV 在 C_NO_OLED 下未造成丢周期
尚缺数据：正式 OledKeyTest 菜单链路空载集成验证数据
当前禁止：直接调 PID、直接重构、直接迁移 FreeRTOS、直接依赖里程停车
下一动作：烧录默认 TASK_MODE_OLED_KEY_TEST，做正式菜单空载集成验证
```

---

## 13. 更新记录

| 日期 | 代码基线 | 内容 |
|---|---|---|
| 2026-07-31 | `c22ea4d4a201171e451f3c7d3c7a7031812bc107` | 根据完整架构地图创建动态项目状态 |
| 2026-07-31 | `cc273286` | 完成 ARCH-001 代码调用链审计，确认硬件 tick 与主循环控制任务分离，确定 GPIO 最小测量方案 |
| 2026-07-31 | `cc273286` | ARCH-001 B/C/D 实测完成：确认 OLED 全屏刷新是超周期根因；新增 ARCH-006 OLED 局部实时计时验证项 |
| 2026-07-31 | `cc273286` | ARCH-006 第一次 E 组为 PARTIAL PASS；运行期计时改为整数秒显示与 2 列分片刷新，待硬件复测 |
| 2026-07-31 | `cc273286` | ARCH-006 最终 E 组通过：整数秒 1Hz OLED 局部显示与 CSV 同开时 999 个周期样本均为 10ms；默认模式切回 OledKeyTest，待正式菜单空载验证 |
