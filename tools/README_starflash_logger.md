# StarFlash CSV Logger

杩欎釜鐩綍涓殑 `starflash_logger.py` 鐢ㄤ簬鍦?Windows 鐢佃剳涓婃帴鏀惰摑鐗欎覆鍙ｆ暟鎹紝骞朵繚瀛樹负 CSV 鏂囦欢锛屼緵 Excel 鎴?Python 鍚庣画鍒嗘瀽銆?

## 1. 瀹夎渚濊禆

```bash
pip install pyserial
```

## 2. 鏌ョ湅褰撳墠鐢佃剳鍙敤涓插彛

```bash
python tools/starflash_logger.py --list
```

鍦ㄨ緭鍑轰腑鎵惧埌钃濈墮妯″潡瀵瑰簲鐨勪覆鍙ｏ紝渚嬪 `COM6`銆?

## 3. 寮€濮嬩繚瀛樿摑鐗欐暟鎹?

榛樿绀轰緥浣跨敤 115200 娉㈢壒鐜囷細

```bash
python tools/starflash_logger.py --port COM6 --baud 115200 --out logs/open_loop_log.csv
```

鑴氭湰浼氳嚜鍔ㄥ垱寤?`logs/` 鐩綍锛屽苟鎶婄鍚堟牸寮忕殑鏁版嵁淇濆瓨鍒?`logs/open_loop_log.csv`銆?

## 4. 濡傛灉钃濈墮娉㈢壒鐜囨槸 9600

濡傛灉 MCU 宸ョ▼鎴栬摑鐗欐ā鍧楅厤缃负 9600锛岃鎶?`--baud` 鏀逛负 9600锛?

```bash
python tools/starflash_logger.py --port COM6 --baud 9600 --out logs/open_loop_log.csv
```

## 5. CSV 鏂囦欢淇濆瓨浣嶇疆

榛樿淇濆瓨鍒帮細

```text
logs/open_loop_log.csv
```

涔熷彲浠ラ€氳繃 `--out` 鎸囧畾鍏跺畠璺緞銆?

## 6. 姝ｇ‘鐨勬暟鎹牸寮忕ず渚?

MCU 绔瘡琛屽簲鍙戦€佷竴鏉″畬鏁?CSV锛屽苟浠?`\r\n` 鎴?`\n` 缁撳熬锛?

```csv
t_ms,mode,test_id,pwm_cmd,left_speed,right_speed,left_count,right_count
0,OL,1,300,0,0,0,0
100,OL,1,300,18,62,180,620
200,OL,1,300,22,68,400,1300
```

鑴氭湰浼氳嚜宸卞啓鍏ユ爣鍑嗚〃澶达紱濡傛灉涓插彛涓篃鏀跺埌琛ㄥご琛岋紝浼氳嚜鍔ㄨ烦杩囷紝涓嶄細閲嶅鍐欏叆銆?

鍙細淇濆瓨浠ヤ笅 8 瀛楁鏍煎紡鐨勬暟鎹細

```text
t_ms,mode,test_id,pwm_cmd,left_speed,right_speed,left_count,right_count
```

绌鸿銆佷贡鐮併€佸瓧娈垫暟閲忎笉瀵圭殑琛屼細鎵撳嵃 warning锛屼絾涓嶄細璁╃▼搴忓穿婧冦€?

## 7. 濡傛灉鍑虹幇涔辩爜鎴栨病鏈夋暟鎹?

浼樺厛妫€鏌ワ細

- 娉㈢壒鐜囨槸鍚︿竴鑷达紝渚嬪 MCU 鏄?9600 鏃讹紝鑴氭湰涔熷繀椤讳娇鐢?`--baud 9600`
- COM 鍙ｆ槸鍚﹂€夐敊
- 钃濈墮鏄惁宸茶繛鎺ャ€佹槸鍚﹀凡缁忛厤瀵规垚鍔?
- MCU 鏄惁鐪熺殑鍦ㄥ彂閫佷互 `\r\n` 鎴?`\n` 缁撳熬鐨勪竴琛屾暟鎹?

