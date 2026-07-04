# Bluetooth CSV Logger

这个目录中的 `bt_logger.py` 用于在 Windows 电脑上接收蓝牙串口数据，并保存为 CSV 文件，供 Excel 或 Python 后续分析。

## 1. 安装依赖

```bash
pip install pyserial
```

## 2. 查看当前电脑可用串口

```bash
python tools/bt_logger.py --list
```

在输出中找到蓝牙模块对应的串口，例如 `COM6`。

## 3. 开始保存蓝牙数据

默认示例使用 115200 波特率：

```bash
python tools/bt_logger.py --port COM6 --baud 115200 --out logs/open_loop_log.csv
```

脚本会自动创建 `logs/` 目录，并把符合格式的数据保存到 `logs/open_loop_log.csv`。

## 4. 如果蓝牙波特率是 9600

如果 MCU 工程或蓝牙模块配置为 9600，请把 `--baud` 改为 9600：

```bash
python tools/bt_logger.py --port COM6 --baud 9600 --out logs/open_loop_log.csv
```

## 5. CSV 文件保存位置

默认保存到：

```text
logs/open_loop_log.csv
```

也可以通过 `--out` 指定其它路径。

## 6. 正确的数据格式示例

MCU 端每行应发送一条完整 CSV，并以 `\r\n` 或 `\n` 结尾：

```csv
t_ms,mode,test_id,pwm_cmd,left_speed,right_speed,left_count,right_count
0,OL,1,300,0,0,0,0
100,OL,1,300,18,62,180,620
200,OL,1,300,22,68,400,1300
```

脚本会自己写入标准表头；如果串口中也收到表头行，会自动跳过，不会重复写入。

只会保存以下 8 字段格式的数据：

```text
t_ms,mode,test_id,pwm_cmd,left_speed,right_speed,left_count,right_count
```

空行、乱码、字段数量不对的行会打印 warning，但不会让程序崩溃。

## 7. 如果出现乱码或没有数据

优先检查：

- 波特率是否一致，例如 MCU 是 9600 时，脚本也必须使用 `--baud 9600`
- COM 口是否选错
- 蓝牙是否已连接、是否已经配对成功
- MCU 是否真的在发送以 `\r\n` 或 `\n` 结尾的一行数据
