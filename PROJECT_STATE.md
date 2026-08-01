# BUPT 2026 E-Car 搴曠洏涓庡惊杩归」鐩姸�?

> 浠撳簱锛歚Jhon0213/bupt-2026-e-car`
> 宸ヤ綔鍒嗘敮锛歚main`
> 鏋舵瀯鍦板浘瀹￠槄鍩虹嚎锛歚c22ea4d4a201171e451f3c7d3c7a7031812bc107`
> 褰撳墠鍒嗘瀽鍩虹嚎锛歚cc273286d0c12cb5bb79760eefded70338daecb1`
> 鏈枃浠舵€ц川锛氬姩鎬佺姸鎬佸叆鍙ｃ€傛瘡娆″紑濮嬫柊鐨?GPT / Codex 浼氳瘽鏃朵紭鍏堣鍙栨湰鏂囦欢銆?

---

## 1. 褰撳墠闃舵

褰撳墠鍙鐞嗗皬杞﹀簳鐩樸€佸惊杩广€侀噷绋嬭鍜屼换鍔″疄鏃舵€э紝涓嶅鐞嗘粴鐞冩帶鍒躲€?

褰撳墠涓荤洰鏍囷�?

1. 娑堥櫎鐩寸嚎寰抗鏃剁殑鍛ㄦ湡鎬у乏鍙虫憞鎽嗭�?
2. 瀹炵幇鍗婂渾鍏ュ集銆佸集涓€佸嚭寮殑杩炵画涓濇粦鎺у埗�?
3. 鏍″噯缂栫爜鍣ㄤ笌閲岀▼璁★紝浣垮叾鍙潬鏀寔璺璇嗗埆�?
4. 浣跨敤鈥滆矾娈电姸鎬?+ 閲岀▼绐楀�?+ A 鐐规í鍚戝惎鍋滅嚎鈥濆疄鐜扮簿鍑嗗仠杞︼紱
5. 纭璁捐�?10 ms 鐨勬帶鍒朵换鍔℃槸鍚︾湡瀹炴寜鏈熸墽琛岋�?
6. 鍦ㄨ涓哄拰鏁版嵁鏄庣‘鍚庯紝娓愯繘寮忛噸鏋勪唬鐮佺粨鏋勩�?

---

## 2. 褰撳墠瀹為檯杩愯閾捐�?

褰撳�?`main.c` 缂栬瘧妯″紡�?

```c
#define SELECTED_TASK_MODE TASK_MODE_OLED_KEY_TEST
```

褰撳墠鐑у綍鍚庣殑涓氬姟璺緞�?

```text
main
  �?
RobotPlatform_Init
  �?
OledKeyTest_Run
  �?
Task3_LinkedOperation_StartMode / Update
  �?
GrayTrack_Update
  �?
SpeedPI_Update
  �?
Motor.move
```

缂栫爜鍣ㄥ弽棣堣矾寰勶細

```text
缂栫爜鍣ㄨ竟娌夸腑鏂?
  �?
宸﹀彸绱璁℃�?
  �?
TIMER_0 �?10 ms 璁＄畻閫熷害
  �?
5 鐐规粦鍔ㄥ钩�?
  �?
SpeedPI 瀹為檯杞€熷弽�?
```

褰撳墠璺嚎鍒嗘璺緞�?

```text
宸﹀彸缂栫爜鍣ㄧ疮璁¤窛绂?
  �?
Task3_LinkedOperation
  �?
AB / BC / CD / DA 璺鍒ゆ柇
  �?
鐩寸�?/ 棰勫�?/ 寮亾鍙傛暟鍒囨�?
```

---

## 3. 褰撳墠宸ヤ綔鍩虹嚎鍐崇瓥

### 3.1 涓存椂鍞竴涓绘帶鍒惰矾�?

搴曠洏涓庡惊杩硅皟璇曢樁娈碉紝鏆傛椂灏嗕互涓嬭矾寰勪綔涓哄敮涓€姝ｅ紡瀹為獙鍩虹嚎�?

```text
OledKeyTest
�?Task3_LinkedOperation
�?GrayTrack
�?SpeedPI
�?Motor
```

鍘熷洜锛?

- 褰撳�?OLED 鑿滃崟鑳藉鐩存帴鍚姩�?
- 宸插寘鍚竴鍦堛€丅+5 cm銆佸垎娈垫帶鍒跺拰鏃ュ織�?
- 涓庡綋鍓嶆渶闇€瑕佽В鍐崇殑鐩寸嚎銆佸崐鍦嗗拰閲岀▼闂鐩存帴瀵瑰簲銆?

### 3.2 鏆備笉骞惰淇敼鐨勮矾�?

浠ヤ笅浠ｇ爜鏆傛椂鍙綔涓哄弬鑰冿紝涓嶄笌褰撳墠瀹為獙閾捐矾鍚屾椂淇敼�?

```text
RouteNavigator
legacy Hardware/CONTROL/control.c
TaskBonus1_LaserTrace
TaskBonusFourLap
```

閬垮厤涓夊鎺у埗浣撶郴鍚屾椂鍙樺寲锛屽鑷村弬鏁般€佹柟鍚戙€佸仠杞﹀拰鐘舵€佹満缁撹鏃犳硶褰掑洜銆?

### 3.3 闀挎湡鏂瑰�?

褰撳墠鍩虹嚎绋冲畾鍚庯紝鍐嶅喅瀹氾�?

- 淇濈暀 `Task3_LinkedOperation` 骞堕噸鏋勶紱
- 灏嗗叾璺閫昏緫鍚堝苟�?`RouteNavigator`�?
- 鎶藉彇缁熶竴鐨勫簳鐩樻帶鍒躲€侀噷绋嬭鍜岃矾绾跨姸鎬佹満�?

鐜伴樁娈典笉杩涜涓€娆℃€уぇ閲嶆瀯銆?

---

## 4. 宸茬‘璁ょ殑鏋舵瀯浜嬪疄

### 4.1 10 ms 瀹氭椂鍣ㄤ笉绛変�?10 ms 鎺у埗闂�?

`TIMER_0` �?10 ms�?

- 鏇存柊鏃堕棿�?
- 璁＄畻缂栫爜鍣ㄩ€熷害锛?
- 浜х敓鎺у埗 tick�?

�?`GrayTrack �?SpeedPI �?Motor` 瀹為檯杩愯鍦ㄤ富寰幆涓�?`Task3_LinkedOperation_Update()`锛屼笉鏄湪瀹氭椂鍣?ISR 鍐呮墽琛屻€?

鍥犳锛?

```text
10 ms 瀹氭椂鍣ㄧǔ�?
�?
鎺у埗绠楁硶姣?10 ms 鍑嗘椂瀹屾�?
```

涓诲惊鐜腑鐨勯樆濉炴搷浣滀粛鍙兘閫犳垚鎺у埗鍛ㄦ湡鍙樹�?15 ms�?0 ms 鎴栨洿闀裤€?

### 4.2 Task3 瀛樺湪杩囨湡鍛ㄦ湡涓㈠純

`Task3_LinkedOperation_Update()`�?

- 鏈€澶氳ˉ鎵ц 3 涓繃鏈熸帶鍒舵锛?
- 鑻ヤ粛鐒惰惤鍚庯紝浼氶噸缃笅涓€娆℃墽琛屾椂闂达紱
- 鏃ф帶鍒跺懆鏈熶細琚富鍔ㄤ涪寮冦�?

杩欎細瀵艰嚧锛?

- 鎺у埗鍛ㄦ湡涓嶅潎鍖€�?
- `SpeedPI` 鍥哄畾浣跨敤 `dt = 0.01 s` 鏃朵骇鐢熷弬鏁板け鐪燂紱
- 璺鍒囨崲鍜岄噷绋嬭涓轰笌璁捐涓嶄竴鑷达紱
- 瀹炶溅娉㈠舰鍙兘鍑虹幇鍋跺彂澶у箙淇�?

### 4.3 缂栫爜鍣ㄩ€熷害鍙嶉鏈夌害 50 ms 婊ゆ尝绐楀�?

缂栫爜鍣ㄩ€熷害閲囩敤鏈€杩?5 �?10 ms 鏍锋湰婊戝姩骞冲潎銆?

浼樼偣锛?

- 闄嶄綆浣庨€熻鏁伴噺鍖栧櫔澹般�?

椋庨櫓锛?

- 澧炲姞閫熷害鍙嶉寤惰繜�?
- �?PWM 鏂滅巼闄愬埗銆佷笂灞傜洰鏍囨枩鐜囬檺鍒跺彔鍔狅紱
- 鍏ュ集銆佸嚭寮拰鎬ュ仠鍝嶅簲鍙兘鏄庢樉婊炲悗�?

### 4.4 褰撳墠閲岀▼鍒嗘渚濊禆鍥哄畾妯″�?

Task3 褰撳墠鍋囪鍖呮嫭锛?

```text
杞緞绾?6.5 cm
缂栫爜鍣ㄧ害 14000 鑴夊�?/ 杞﹁疆涓€鍦?
鐩寸�?150 cm
寮亾鍗婂緞 50 cm
```

骞堕€氳繃缂栫爜鍣ㄧ疮璁″€间及�?AB / BC / CD / DA�?

鏈粡鐙珛鏍囧畾鍓嶏紝杩欎簺鍊间笉鑳戒綔涓洪珮绮惧害缁堢偣渚濇嵁銆?

### 4.5 褰撳墠瀛樺湪闃诲椋庨�?

宸茬煡闃诲鏉ユ簮锛?

- 鐏板害杞欢 I2C�?
- OLED 杞�?I2C�?
- 鏄熼棯閫愬瓧鑺傞樆濉炲彂閫侊�?
- `delay_ms()` 蹇欑瓑寰咃紱
- 50 ms 涓€娆＄殑闀?CSV 璋冭瘯杈撳嚭�?
- 鏍煎紡鍖栧瓧绗︿覆鍜屼覆鍙ｇ瓑寰呫€?

115200 baud 涓嬶紝绾?100 瀛楄妭鏁版嵁闇€瑕佺�?8.7 ms 鐨勭嚎璺椂闂淬€傞�?CSV 寰堝彲鑳藉凡缁忔帴杩戞垨瓒呰繃涓€涓?10 ms 鎺у埗鍛ㄦ湡銆?

### 4.6 `Motor_Brake()` 褰撳墠瀹為檯涓烘粦�?

褰撳墠瀹炵幇璇箟�?

```c
Motor_Brake()
{
    Motor_Coast();
}
```

鍥犳锛?

- 鈥滃埞杞︹€濆疄闄呬笂鏄珮闃绘粦琛岋紱
- 鍋滆溅璺濈鍜岄€熷害銆佺數姹犮€佽矾闈㈤珮搴︾浉鍏筹紱
- 褰撳墠鎻愬墠鍋滆溅鍙傛暟鍙兘鏄湪琛ュ伩婊戣璺濈锛?
- 绮惧噯鍋滆溅鍓嶅繀椤绘槑纭湡姝ｇ殑鍒跺姩绛栫暐銆?

### 4.7 SysConfig 婧愰厤缃笌鐢熸垚鏂囦欢瀛樺湪婕傜�?

`empty.syscfg` 涓伆搴﹂厤缃笌褰撳墠瀹為檯鐢熸垚澶存枃浠剁殑寮曡剼瀹氫箟涓嶄竴鑷淬�?

鍦ㄩ噸鏂拌繍�?SysConfig 鍓嶅繀椤诲厛淇锛屽惁鍒欏彲鑳借鐩栧綋鍓嶅彲杩愯閰嶇疆�?

---

## 5. 涓変釜涓昏闂鐨勫疄闄呰€﹀悎鍏崇郴

```text
涓诲惊鐜樆�?/ 鍛ㄦ湡鎶栧姩
    �?
閫熷�?PI 鐨勫疄闄?dt 閿欒�?
    �?
宸﹀彸杞搷搴斾笉涓€鑷村拰鎺у埗杈撳嚭绐佸彉
    �?
鐩寸嚎鎽囨憜銆佸叆寮繃鍐层€佸嚭寮弽鍚戜慨姝?
    �?
杞儙渚ф粦鍜岀疮璁¤窛绂昏�?
    �?
AB / BC / CD / DA 鍒囨崲鐐规紓�?
    �?
缁堢偣鍋滆溅璇�?
```

鍥犳涓嶅緱鐩存帴璺冲埌鈥滆皟寰抗 PID鈥濇垨鈥滈噸鍐欑姸鎬佹満鈥濄�?

---

## 6. 褰撳墠浼樺厛�?

### P0锛氭帴鍙ｄ笌鍩虹嚎纭

- [ ] 纭鐪熸鐨勭數鏈烘柟鍚戝拰宸﹀彸杞槧灏勶�?
- [ ] 鏄庣�?`Motor_Brake`銆乣Motor_Coast` 鐨勫疄闄呯‖浠惰涓猴�?
- [ ] 鏆傚仠閲嶆柊鐢熸�?SysConfig�?
- [ ] 纭褰撳墠瀹為獙鍙蛋 `Task3_LinkedOperation`�?
- [ ] 璁板綍褰撳墠鍙繍琛岀増鏈?Git 鏍囩銆?

### P1锛氬疄鏃舵€у璁?

- [ ] 娴嬮�?Task3 瀹為檯鎺у埗姝ュ懆鏈燂�?
- [ ] 娴嬮噺鏈€澶ф帶鍒堕棿闅旓�?
- [ ] 缁熻涓㈠純鎺у埗鍛ㄦ湡娆℃暟�?
- [ ] 娴嬮�?Gray銆丼peedPI銆丮otor銆佹棩蹇楀�?OLED 鎵ц鏃堕棿锛?
- [ ] 涓存椂鍏抽棴鎴栭檷棰戦暱 CSV锛屾瘮杈冩帶鍒舵晥鏋溿€?

### P2锛氱紪鐮佸櫒涓庨噷绋嬭

- [ ] 纭宸﹀彸杞瘡杞疄闄呰剦鍐叉暟锛?
- [ ] 纭�?1X / 2X / 4X 璁℃暟妯″紡�?
- [ ] 纭宸﹀彸杞鍙峰拰鏂瑰悜�?
- [ ] 鍒嗗埆鏍囧畾宸﹀彸杞窛绂荤郴鏁帮紱
- [ ] 瀹屾�?1 m�? m銆佸崟鍗婂渾銆佸畬鏁翠竴鍦堟祴璇曪紱
- [ ] 鍖哄垎姣斾緥璇樊銆佸懆鏈熻宸拰杞集渚ф粦璇樊�?

### P3锛氱洿绾垮惊�?

- [ ] 鍚屾椂閲囬泦鐏板害璇樊銆佺洰鏍囪浆閫熴€佸疄闄呰浆閫熷�?PWM�?
- [ ] 鍏抽棴寰抗鍚庨獙璇佸乏鍙宠疆绛夐€熺洿琛岋�?
- [ ] 妫€鏌ョ伆搴︿綅缃及璁℃槸鍚︾鏁ｈ烦鍙橈�?
- [ ] 妫€鏌ュ惊杩硅緭鍑洪檺骞呭拰鍙樺寲鐜囷紱
- [ ] 妫€鏌ラ€熷害婊ゆ尝寤惰繜瀵规憜鍔ㄧ殑褰卞搷锛?
- [ ] 閫愭。鎻愰珮鍩虹閫熷害楠岃瘉绋冲畾鎬с€?

### P4锛氬崐鍦嗕笣婊戞帶鍒?

- [ ] 灏嗗集閬撳垝鍒嗕负鍏ュ集銆佺ǔ鎬佸集閬撱€佸嚭寮�?
- [ ] 浣跨敤骞虫粦鍩虹閫熷害鍜屽樊閫熺洰鏍囷�?
- [ ] 浠ユ洸鐜囧墠棣堟壙鎷呬富瑕佽浆鍚戯紱
- [ ] 鐏板害鍙嶉鍙慨姝ｆ畫宸�?
- [ ] 姣旇緝宸﹀彸涓や釜鍗婂渾鐨勪竴鑷存€э紱
- [ ] 妫€鏌ヨ浆寮晶婊戝閲岀▼璁＄殑褰卞搷銆?

### P5锛氱粓鐐规娴嬩笌鍋滆溅

- [ ] 寤虹珛鏈€鍚庣洿绾胯矾娈电姸鎬侊�?
- [ ] 浣跨敤閲岀▼璁¤繘鍏?A 鐐规悳绱㈢獥鍙ｏ�?
- [ ] 绋冲畾璇嗗埆妯悜鍚仠绾匡�?
- [ ] 寤虹珛鍑忛€熴€佹娴嬨€佸埗鍔ㄣ€佷繚鎸佺姸鎬侊紱
- [ ] 鏍囧畾妫€娴嬬偣鍒版寚瀹氭祴璇曠偣鐨勫嚑浣曞亸绉伙�?
- [ ] 杩炵�?10 娆￠獙璇佹渶澶у仠杞﹁宸笉瓒呰�?2 cm�?

### P6锛氭笎杩涘紡閲嶆�?

- [ ] 缁熶竴搴曠洏鎺у埗鍏ュ彛锛?
- [ ] 鎶藉彇缁熶竴 Odometry�?
- [ ] 鎶藉彇缁熶竴 RouteState�?
- [ ] 娓呯悊鎴栧仠姝㈢紪璇戦仐�?`control.c`�?
- [ ] 缁熶竴鏃ュ織鎺ュ彛锛?
- [ ] 灏嗛潪鍏抽敭浠诲姟涓庢帶鍒朵换鍔″垎棰戦殧绂汇€?

---

## 7. 褰撳墠绗竴浠诲姟锛欰RCH-001

### 7.1 闂鐘舵€?

| 缂栧�?| 闂�?| 鐘舵�?| 渚濊�?|
|---|---|---:|---|
| ARCH-001 | 娴嬮噺鎺у埗浠诲姟鐪熷疄鍛ㄦ湡鍜屾姈鍔?| DONE | �?|

ARCH-001 宸插畬鎴?B / C / D 涓夌粍瀹炴満娴嬮噺锛岀‘璁?OLED 鍏ㄥ睆杞欢 I2C 鍒锋柊鏄€犳垚涓诲惊鐜?10 ms 鎺у埗浠诲姟瓒呭懆鏈熷拰绉帇涓㈠純鐨勭‘瀹氭€ф牴鍥犮�?

### 7.2 鍒嗘瀽鍩虹嚎

```text
鍒嗘敮锛歮ain
Commit锛歝c273286d0c12cb5bb79760eefded70338daecb1
褰撳墠榛樿缂栬瘧妯″紡锛歍ASK_MODE_OLED_KEY_TEST
褰撳墠榛樿杩愯浠诲姟锛歄ledKeyTest 姝ｅ紡鑿滃崟锛汚RCH001_CASE_E_REALTIME_TIME 淇濈暀涓哄彲鍒囨崲澶嶆祴榛樿�?
```

### 7.3 宸茬‘璁や簨�?

1. `TIMER_0` 鐢辩‖浠堕厤缃�?10 ms 鍛ㄦ湡瀹氭椂鍣紱
2. `TIMER_0` 涓柇姣?10 ms 鏇存柊绯荤粺鏃堕棿銆佸乏鍙崇紪鐮佸櫒閫熷害锛屽苟浜х敓杞欢鎺у�?tick�?
3. 褰撳�?OLED 鑿滃崟涓殑 Task3 涓嶆秷璐硅蒋浠舵帶鍒?tick锛岃€屾槸鍦ㄤ富寰幆涓牴�?`board_millis` 璋冨害鎺у埗姝ワ紱
4. 鐏板害閲囨牱銆佸惊杩规帶鍒躲€侀€熷�?PI 鍜岀數鏈鸿緭鍑哄潎鍦ㄤ富寰幆涓殑 `Task3_ControlStep` 鍐呮墽琛岋紱
5. Task3 涓€娆℃渶澶氳拷�?3 涓繃鏈熸帶鍒舵锛屽墿浣欑Н鍘嬪懆鏈熶細琚涪寮冿紱
6. 褰撳墠鎺у埗姝ュ唴�?50 ms 鍚屾鍙戦€佷竴娆℃槦闂?CSV�?
7. OLED �?500 ms 閫氳繃杞欢 I2C 鍚屾鍒锋柊瀹屾�?1024 瀛楄妭甯х紦鍐诧紱
8. 浠ｇ爜瀹¤鍙兘纭鈥滆璁″懆鏈熶�?10 ms鈥濓紝灏氫笉鑳界‘璁ゅ疄闄呮帶鍒跺懆鏈熺ǔ瀹氫�?10 ms�?

### 7.4 浠ｇ爜瀹¤缁撹�?

褰撳墠鏋舵瀯瀛樺湪涓ゅ涓嶅悓鐨勬椂闂存蹇碉細

```text
纭�?10 ms 涓柇锛?
缂栫爜鍣ㄦ祴閫熷拰绯荤粺鏃跺熀

涓诲惊鐜?10 ms 浠诲姟锛?
鐏板害銆佸惊杩广€侀€熷�?PI 鍜岀數鏈鸿緭�?
```

纭欢瀹氭椂鍣ㄥ噯纭苟涓嶄唬琛ㄤ富寰幆鎺у埗杈撳嚭鍑嗙‘銆?

褰撳墠鏈€澶х殑闃诲瀚岀枒鍖呮嫭�?

```text
OLED 鍏ㄥ睆杞欢 I2C 鍒锋�?
鏄熼棯鍚屾闀?CSV 鍙戦�?
鐏板害杞欢 I2C 璇诲�?
涓诲惊鐜腑鐨勫欢鏃跺拰鍏朵粬鍚屾鎿嶄�?
```

蹇呴』閫氳繃 GPIO 鍜岄€昏緫鍒嗘瀽浠垨绀烘尝鍣ㄦ祴閲忕湡瀹炲懆鏈燂紝涓嶈兘浠呴€氳繃浠ｇ爜鎺ㄦ柇�?

### 7.5 涓昏椋庨櫓

```text
1. OLED 鍏ㄥ睆鍒锋柊鎸変唬鐮佹椂搴忎及绠楃害 225 ms锛屽彲鑳藉懆鏈熸€ч樆濉炰富寰幆�?
2. Task3 澶氬瓧娈?CSV �?115200 baud 涓嬪彲鑳芥帴杩戞垨瓒呰繃 10 ms�?
3. 鎺у埗绉帇鏃舵渶澶氳ˉ鎵ц 3 娆★紝鍓╀綑鍛ㄦ湡琚涪寮冦€?
4. 杩借刀鎺у埗姝ュ彲鑳戒娇鐢ㄥ悓涓€浠芥渶鏂扮紪鐮佸櫒閫熷害锛屼笉浠ｈ〃鍘嗗彶鍛ㄦ湡琚仮澶嶃�?
5. SpeedPI 鍜岄儴鍒嗙姸鎬佽鏃跺浐瀹氬亣璁?dt = 10 ms锛屽彂鐢熻秴鍛ㄦ湡鏃跺弬鏁板惈涔夊け鐪熴�?
```

鍏朵�?OLED �?225 ms 褰撳墠鍙槸鍩轰簬浠ｇ爜鏃跺簭鐨勪及绠楀€硷紝蹇呴』閫氳繃瀹炴満娉㈠舰纭�?

### 7.6 涓嬩竴姝ユ祴閲忔柟妗?

1. �?`TIMER_0_INST_IRQHandler` 涓鍔?`DBG_TICK` 鑴夊啿锛?
2. �?`Task3_LinkedOperation_Update()` 璋冪�?`Task3_ControlStep()` 鍓嶅悗澧炲姞 `DBG_CTRL` 鑴夊啿锛?
3. 淇濇�?OLED銆佽皟璇?CSV 鍜屾帶鍒跺弬鏁颁笉鍙橈紝鍏堣褰曞綋鍓嶇湡瀹炲熀绾匡�?
4. 璁板綍鑷冲皯 3 绉掕繛缁尝褰紝骞跺畬鎴愪竴娆″畬鏁磋禌閬撹繍琛岋�?
5. 缁熻鏈€灏忋€佸钩鍧囥€佹渶澶ф帶鍒跺懆鏈熴€佹帶鍒舵鎵ц鏃堕棿鍜岃秴鍛ㄦ湡娆℃暟�?
6. 澧炲�?catch-up 娆℃暟鍜?backlog 涓㈠純娆℃暟鐨勮交閲忚鏁帮�?
7. 绂佹鍦ㄦ帶鍒跺懆鏈熷唴鐩存帴鎵撳嵃涓婅堪缁熻鍊硷�?
8. 娴嬭瘯缁撴潫鎴栧皬杞﹀仠姝㈠悗锛屽啀缁熶竴杈撳嚭缁熻缁撴灉�?

### 7.7 鎺ㄨ崘瀵圭収娴嬭瘯鐭╅樀

#### 娴嬭�?A锛氬綋鍓嶅畬鏁磋礋杞?

```text
OLED锛氬紑鍚?
Task3 CSV锛氬紑鍚?
鎺у埗鍙傛暟锛氫笉�?
```

鐩殑锛氳幏鍙栧綋鍓嶇湡瀹炲熀绾裤�?

#### 娴嬭�?B锛氬叧闂?Task3 CSV

```text
OLED锛氬紑鍚?
Task3 CSV锛氬叧闂?
```

鐩殑锛氶噺鍖栨槦闂悓姝ユ棩蹇楀鍛ㄦ湡鐨勫奖鍝嶃€?

#### 娴嬭�?C锛氬叧闂?OLED 鍒锋�?

```text
OLED锛氬叧闂埛�?
Task3 CSV锛氬紑鍚?
```

鐩殑锛氶噺�?OLED 杞�?I2C 瀵瑰懆鏈熺殑褰卞搷銆?

#### 娴嬭�?D锛氬叧闂?OLED �?CSV

```text
OLED锛氬叧闂埛�?
Task3 CSV锛氬叧闂?
```

鐩殑锛氳幏寰楀簳鐩樻帶鍒堕摼璺殑鏈€浣庤礋杞藉熀绾裤�?

### 7.8 灏氭湭纭鐨勬暟鎹?

```text
control_dt_min_us
control_dt_avg_us
control_dt_max_us
control_exec_max_us
control_overrun_count
control_backlog_drop_count
control_catchup_count
```

### 7.9 ARCH-001 瀹屾垚鏉′欢

鍙湁婊¤冻浠ヤ笅鏉′欢锛孉RCH-001 鎵嶈兘浠?`ANALYZING` 杞�?`DONE`�?

```text
宸插彇寰楀疄鏈哄懆鏈熸尝褰?
宸茬粺璁℃渶灏忋€佸钩鍧囥€佹渶澶у懆�?
宸叉祴寰楁渶澶ф帶鍒舵鎵ц鏃堕棿
宸茬粺璁¤秴鍛ㄦ湡娆℃�?
宸茬粺璁¤拷璧跺拰绉帇涓㈠純娆℃�?
宸查噺鍖?OLED �?CSV 瀵瑰懆鏈熺殑褰卞�?
宸茬‘璁?SpeedPI �?dt 绛栫暐鏄惁闇€瑕佷慨鏀?
```

鍦ㄦ涔嬪墠锛屼笉杩涘叆澶ц�?PID 璋冩暣銆侀噷绋嬭鏈€缁堟爣瀹氭�?FreeRTOS 杩佺Щ�?

---
## 7.10 ARCH-001 瀹炴祴缁撴灉涓庣粨璁?

| 娴嬭瘯缁?| OLED | CSV | 鎺у埗娆℃�?| 骞冲潎鍛ㄦ湡 | 鏈€澶у懆�?| Catch-up | Backlog 涓㈠�?|
|---|---|---|---:|---:|---:|---:|---:|
| B_NO_CSV | 寮€ | �?| 559 | 17.90 ms | 260 ms | 20 | 20 |
| C_NO_OLED | �?| 寮€ | 1000 | 10.00 ms | 10 ms | 0 | 0 |
| D_MIN_LOAD | �?| �?| 1000 | 10.00 ms | 10 ms | 0 | 0 |

缁撹锛?

1. 闃诲寮?OLED 鍏ㄥ睆鍒锋柊�?10 ms 鎺у埗浠诲姟瓒呭懆鏈熺殑纭畾鎬ф牴鍥狅�?
2. 涓€娆″畬�?OLED 鍒锋柊鏈€闀跨害 250 ms�?
3. 20 �?OLED 鍒锋柊瀵瑰�?20 娆¤秴�?30 ms 鍛ㄦ湡銆?0 �?catch-up �?20 �?backlog 涓㈠純锛?
4. 褰撳�?Task3 CSV �?C_NO_OLED 缁勬湭閫犳垚鎺у埗鍛ㄦ湡涓㈠け�?
5. 鍏抽�?OLED 瀹屾暣鍒锋柊鍚庯紝瑁告満涓诲惊鐜粨鏋勫彲浠ョǔ瀹氳揪鍒?10 ms 鎺у埗鍛ㄦ湡锛?
6. 褰撳墠涓嶈縼�?FreeRTOS锛屼紭鍏堝皢杩愯鏈?OLED 鏀逛负灞€閮ㄣ€佸垎姝ャ€侀潪闃诲鍒锋柊銆?

## 7.11 ARCH-006锛歄LED 杩愯鏈熼潪闃诲瀹炴椂璁℃椂

| 缂栧�?| 闂�?| 鐘舵�?| 渚濊�?|
|---|---|---:|---|
| ARCH-006 | OLED 杩愯鏈熼潪闃诲瀹炴椂璁℃椂 | DONE | ARCH-001 |
| FORMAL-OLED-001 | 姝ｅ�?OledKeyTest 绌鸿浇闆嗘垚楠岃�?| PENDING_HARDWARE_TEST | ARCH-006 |

璁捐鍐崇瓥�?
1. WAIT / MENU 椤甸潰鍏佽缁х画浣跨�?OLED 鍏ㄥ睆鍒锋柊�?2. 杞﹁締杩愯鍓嶅厛缁樺埗闈欐€佽繍琛岄〉闈㈠苟瀹屾垚涓€娆?`OledDisplay_Update()`�?3. 瀹屾暣鍒锋柊缁撴潫鍚庡啀璇诲�?`board_millis()` 骞跺惎鍔?`Task3_LinkedOperation`锛屽惎鍔ㄥ墠鍏ㄥ睆鍒锋柊涓嶈鍏ヨ椹舵椂闂达紱
4. 杩愯鏈熼棿绂佹�?`OledDisplay_Clear()` 鍜屽叏灞?`OledDisplay_Update()`�?5. 杩愯鏈熼棿鏄剧ず鍥哄畾瀹藉害鏁存暟�?`000s`锛屾樉绀洪�?1 Hz�?6. 鍐呴儴璁℃椂浠嶄娇鐢?`board_millis()` 鍜屾绉掔骇 `elapsed_ms`�?7. 姣忔涓诲惊鐜厛鎵ц�?`Task3_LinkedOperation_Update()`锛屽啀澶勭�?OLED 灞€閮ㄨ鏃讹紱
8. 褰撳墠灞€閮ㄥ埛鏂伴噰鐢ㄥ抚缂撳啿銆乨irty 瀛楃妫€娴嬪拰鍒楀垎鐗囧彂閫侊紝姣忚疆鏈€澶氬彂閫?2 �?OLED 鏁版嵁锛?9. 浠诲姟瀹屾垚鎴栧仠姝㈡椂鍏堝仠�?Task3 鍜岀數鏈猴紝鍐嶅厑璁告渶缁堝叏灞忓埛鏂帮�?10. 褰撳墠涓嶉渶瑕佽縼绉?FreeRTOS�?
绗竴娆?E_REALTIME_TIME 瀹炴祴缁撴灉�?.1 绉掓樉绀猴紝10 Hz 灞€閮ㄥ埛鏂帮級�?
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

鍒ゅ畾锛歚PARTIAL PASS`銆傚叏灞?OLED 闃诲宸茬粡娑堥櫎锛?0 绉掑唴浠嶅畬�?1000 娆℃帶鍒朵笖娌℃�?backlog 涓㈠純锛涗絾 35 �?20ms 鍛ㄦ湡鍚庣珛�?0ms 琛ユ墽琛岃�?10 Hz 鏁板瓧灞€閮ㄥ埛鏂颁粛浼氬甫鏉ュ彲瑙傛祴璋冨害鎶栧姩�?
鏈€�?E_REALTIME_TIME 瀹炴祴缁撴灉锛堟暣鏁扮鏄剧ず锛? Hz 灞€閮ㄥ垎鐗囧埛鏂帮級锛?
| 鎸囨�?| 缁撴�?|
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

纭欢鐜拌薄锛歄LED 鏁存暟绉掓甯告樉绀猴紝鏄熼棯绔甯告敹鍒?CSV锛涙祴璇曞紑濮嬪墠涓插彛鏇惧�?TX 鎺ヨЕ涓嶈壇鍑虹幇涔辩爜锛岄噸鏂版帴濂藉悗鏁版嵁姝ｅ父锛岃涔辩爜灞炰簬鎺ョ嚎鎺ヨЕ闂锛屼笉灞炰簬绋嬪簭寮傚父銆?
鏈€缁堢粨璁猴細

1. 鏁存暟绉?1 Hz 鏄剧ず娑堥櫎浜嗗�?0.1 绉掓樉绀轰骇鐢熺�?35 �?catch-up�?2. OLED 瀹炴椂鏄剧ず�?CSV 鍚屾椂寮€鍚椂锛屾帶鍒跺懆鏈熶繚鎸佷弗�?10 ms�?3. 杩愯鏈熼棿鍏ㄥ睆鍒锋柊娆℃暟涓?0�?4. 灞€�?OLED 鍐欏叆娌℃湁瓒呰�?10 ms�?5. OLED 鏁存暟绉掑眬閮ㄦ樉绀烘満鍒跺凡閫氳繃绌鸿浇纭欢鏃跺簭楠屾敹�?6. 姝ｅ�?`OledKeyTest` 鑿滃崟閾捐矾宸叉帴鍏ヨ鏈哄埗锛屼絾浠嶇瓑寰呯敤鎴峰畬鎴愭渶缁堢┖杞介泦鎴愰獙璇併€?
### 姝ｅ�?OledKeyTest 绌鸿浇闆嗘垚楠岃�?
鐘舵€侊細`PENDING_HARDWARE_TEST`

楠屾敹鍐呭�?
1. 鑿滃崟鍜?K1 / K2 鎸夐敭姝ｅ父�?2. 鍚姩鍓嶅叏灞忓埛鏂颁笉璁″叆琛岄┒鏃堕棿�?3. 杩愯鏃堕棿�?`000s` 寮€濮嬶�?4. 杩愯鏈熼棿姣忕鏁存暟鏇存柊锛?5. 杩愯鏈熼棿鏃犲叏灞忛棯鐑侊�?6. 鏄熼�?CSV 姝ｅ父锛?7. Task3 瀹屾垚鍚庡厛鍋滆溅锛?8. 鏈€缁堥〉闈㈡樉绀烘椂闂村拰閲岀▼姝ｇ‘銆?
---
## 8. 閲岀▼璁℃渶缁堣璁″師�?

### 8.1 璺濈鏉ユ簮

浼樺厛鐩存帴绱缂栫爜鍣ㄥ閲忥細

```text
left_distance += left_delta_count �?K_left
right_distance += right_delta_count �?K_right
center_distance += (left_delta_distance + right_delta_distance) / 2
```

涓嶄紭鍏堜娇鐢�?

```text
distance += filtered_speed �?fixed_0.01
```

鍘熷洜鏄悗鑰呭彈鍒帮細

- 鎺у埗鍛ㄦ湡鎶栧姩�?
- 閫熷害婊ゆ尝寤惰繜锛?
- 娴偣绉垎�?
- 涓㈡帶鍒舵

褰卞搷銆?

### 8.2 宸﹀彸杞嫭绔嬫爣�?

蹇呴』淇濈暀�?

```text
K_left
K_right
```

涓嶈兘鍙€氳繃涓€涓粺涓€杞緞鍙傛暟琛ュ伩宸﹀彸杞樊寮傘�?

### 8.3 缁堢偣鍒ゅ畾

鏈€缁堥€昏緫锛?

```text
宸查€氳繃瀹屾暣璺鐘舵�?
AND
绱璺濈杩涘叆缁堢偣鎼滅储绐楀�?
AND
A 鐐规í鍚戝惎鍋滅嚎杩炵画妫€娴嬫垚�?
    �?
杩涘叆鍒跺姩鐘舵�?
```

閲岀▼璁¤礋璐ｂ€滃埌杈鹃檮杩戔€濓紝鍚仠绾胯礋璐ｂ€滅簿纭畾浣嶁€濄�?

---

## 9. 鐩寸嚎涓庡崐鍦嗘帶鍒跺師�?

### 9.1 鐩寸�?

浼樺厛鎺掓煡椤哄簭锛?

```text
瀹為檯鍛ㄦ湡
�?宸﹀彸杞€熷害闂幆涓€鑷存�?
�?鐏板害浣嶇疆杩炵画鎬?
�?寰抗澧炵泭
�?寰垎鍣０
�?杈撳嚭闄愬箙鍜屽彉鍖栫巼
```

### 9.2 鍗婂�?

鎺ㄨ崘缁撴瀯锛?

```text
鐩爣鍩虹閫熷�?
+
寮亾鏇茬巼鍓嶉�?
+
鐏板害娈嬪樊鍙嶉�?
    �?
宸﹀彸鐩爣 RPM
```

鐘舵€侊�?

```text
STRAIGHT
ENTER_CURVE
CURVE_STEADY
EXIT_CURVE
```

鎵€鏈夐€熷害鍜屽樊閫熷垏鎹㈠繀椤昏繛缁紝绂佹鍦ㄥ崟涓噷绋嬬偣杩涜闃惰穬鍒囨崲銆?

---

## 10. 褰撳墠绂佹浜嬮�?

�?ARCH-001 鍜岄噷绋嬫爣瀹氬畬鎴愬墠锛屼笉鍋氾細

- 澶ц妯＄洰褰曢噸鏋勶�?
- 鍚屾椂璋冩暣 GrayTrack �?SpeedPI 澶氱粍鍙傛暟�?
- 鍒囨崲鍒?RouteNavigator 浣滀负涓荤嚎�?
- 鍒犻櫎鐜版湁 Task3�?
- 鏍规嵁涓€鍦堣宸洿鎺ヤ慨鏀硅疆寰勶�?
- 鍙嚟瑙嗛鍒ゆ柇鎺у埗鍛ㄦ湡�?
- 鍦ㄦ帶鍒跺惊鐜唴澧炲姞鏇村�?`printf`�?
- 閲嶆柊杩愯 SysConfig�?
- 浣跨敤寮哄埗鎺ㄩ€佽鐩栫幇�?Git 鍘嗗彶銆?

---

## 11. 浼氳瘽鍚姩鎻愮ず璇?

```text
璇蜂娇鐢?GitHub 璇诲�?Jhon0213/bupt-2026-e-car 褰撳墠鎸囧畾鍒嗘敮銆?

棣栧厛瀹屾暣闃呰�?
- docs/PROJECT_STATE.md
- docs/ARCHITECTURE_MAP.md
- docs/CHASSIS_TRACKING_MASTER_PLAN.md

褰撳墠姝ｅ紡瀹為獙璺緞鍥哄畾涓猴細
OledKeyTest
�?Task3_LinkedOperation
�?GrayTrack
�?SpeedPI
�?Motor

褰撳墠闂缂栧彿�?
ARCH-001

浠诲姟锛?
纭璁捐�?10 ms 鐨勬帶鍒朵换鍔″疄闄呰繍琛屽懆鏈燂紝骞跺畾浣嶆墍鏈夊彲鑳介€犳垚闃诲鍜岃繃鏈熷懆鏈熶涪寮冪殑浠ｇ爜銆?

瑕佹眰锛?
1. 鍩轰簬鐪熷疄浠ｇ爜锛屼笉鏍规�?README 鎺ㄦ柇锛?
2. 鍒楀嚭瀹氭椂鍣?ISR銆佷富寰幆�?Task3 鐨勫畬鏁磋皟鐢ㄩ摼锛?
3. 鎵惧埌鎺у埗鍛ㄦ湡璋冨害銆佽ˉ鎵ц鍜屼涪寮冮€昏緫锛?
4. 鍒嗘�?Gray 杞�?I2C銆丱LED銆丼tarFlash銆乨elay 鍜屾棩蹇楃殑闃诲璺緞�?
5. 缁欏嚭鏈€灏忎镜鍏ュ紡娴嬮噺鏂规锛?
6. 鍏堜笉淇敼鎺у埗鍙傛暟锛?
7. 鍏堜笉杩涜澶ц妯￠噸鏋勶紱
8. 鏈€鍚庤緭鍑洪渶瑕佸洖鍐欏埌 PROJECT_STATE.md 鐨勭粨璁恒€?
```

---

## 12. 褰撳墠鐘舵€佹憳�?

```text
姝ｅ紡瀹為獙璺緞锛歍ask3_LinkedOperation
褰撳墠闂锛欰RCH-001
褰撳墠鐘舵€侊細ARCH-001 DONE锛汚RCH-006 DONE锛汧ORMAL-OLED-001 PENDING_HARDWARE_TEST
褰撳墠缁撹锛歄LED 鍏ㄥ睆鍒锋柊浼氱牬鍧?10 ms 涓诲惊鐜帶鍒跺懆鏈燂紱鍏抽棴鍏ㄥ睆鍒锋柊鍚庡懆鏈熺ǔ�?
鏈€澶у珜鐤戯細宸茬‘璁?OLED 鍏ㄥ睆杞欢 I2C锛涘綋鍓?CSV �?C_NO_OLED 涓嬫湭閫犳垚涓㈠懆鏈?
灏氱己鏁版嵁锛氭寮?OledKeyTest 鑿滃崟閾捐矾绌鸿浇闆嗘垚楠岃瘉鏁版嵁
褰撳墠绂佹锛氱洿鎺ヨ皟 PID銆佺洿鎺ラ噸鏋勩€佺洿鎺ヨ縼�?FreeRTOS銆佺洿鎺ヤ緷璧栭噷绋嬪仠�?
涓嬩竴鍔ㄤ綔锛氱儳褰曢粯�?TASK_MODE_OLED_KEY_TEST锛屽仛姝ｅ紡鑿滃崟绌鸿浇闆嗘垚楠岃瘉
```

---

## 13. 鏇存柊璁板綍

| 鏃ユ�?| 浠ｇ爜鍩虹嚎 | 鍐呭�?|
|---|---|---|
| 2026-07-31 | `c22ea4d4a201171e451f3c7d3c7a7031812bc107` | 鏍规嵁瀹屾暣鏋舵瀯鍦板浘鍒涘缓鍔ㄦ€侀」鐩姸�?|
| 2026-07-31 | `cc273286` | 瀹屾�?ARCH-001 浠ｇ爜璋冪敤閾惧璁★紝纭纭欢 tick 涓庝富寰幆鎺у埗浠诲姟鍒嗙锛岀‘�?GPIO 鏈€灏忔祴閲忔柟�?|
| 2026-07-31 | `cc273286` | ARCH-001 B/C/D 瀹炴祴瀹屾垚锛氱‘璁?OLED 鍏ㄥ睆鍒锋柊鏄秴鍛ㄦ湡鏍瑰洜锛涙柊�?ARCH-006 OLED 灞€閮ㄥ疄鏃惰鏃堕獙璇侀�?|
| 2026-07-31 | `cc273286` | ARCH-006 绗竴娆?E 缁勪�?PARTIAL PASS锛涜繍琛屾湡璁℃椂鏀逛负鏁存暟绉掓樉绀轰笌 2 鍒楀垎鐗囧埛鏂帮紝寰呯‖浠跺�?|
| 2026-07-31 | `cc273286` | ARCH-006 鏈€�?E 缁勯€氳繃锛氭暣鏁扮�?1Hz OLED 灞€閮ㄦ樉绀轰笌 CSV 鍚屽紑鏃?999 涓懆鏈熸牱鏈潎涓?10ms锛涢粯璁ゆā寮忓垏�?OledKeyTest锛屽緟姝ｅ紡鑿滃崟绌鸿浇楠岃�?|
---

## SPEED-CAL-001 Real-time Speed Inner-loop Calibration

Date: 2026-08-01
Status: CODE COMPLETE, READY FOR HARDWARE TEST

Implemented mode:
- SELECTED_TASK_MODE = TASK_MODE_SPEED_CALIBRATION_TEST
- SPEED_CAL_SELECTED_CASE = SPEED_CAL_RIGHT_OPEN_LOOP

Scope:
- Added independent speed calibration task.
- Right open-loop and right closed-loop cases are implemented.
- Left open-loop, left closed-loop, and both-equal cases are reserved in the enum only.
- Formal Task3, GrayTrack, route segments, curve control, exit compensation, stop distances, OLED timing, encoder parameters, and formal SpeedPI parameters were not changed.

Timing:
- Control period: 10 ms, driven by board_consume_control_tick().
- VOFA send period: 50 ms, after the control calculation and motor output step.
- No delay_ms(10) is used as the control scheduler.
- No large runtime RAM log is stored for post-run output.

VOFA protocol:
- Existing StarFlash UART2 path is used.
- Protocol is FireWater-style integer CSV: value1,value2,...\r\n
- UART: StarFlash UART2, PA21 TX / PA22 RX, 115200 baud.
- Send function path: SpeedCalibration_SendVofaFrame() -> StarFlash_SendByte().

Fixed 18 channels:
0 elapsed_ms
1 test_case: 0 RIGHT_OPEN_LOOP, 1 RIGHT_CLOSED_LOOP
2 phase
3 command_pwm
4 command_target_rpm_x10
5 control_target_rpm_x10
6 right_raw_rpm_x10
7 right_filtered_rpm_x10
8 right_error_rpm_x10
9 right_pwm
10 right_raw_pwm_x10
11 right_feedforward_pwm_x10
12 right_p_term_x10
13 right_integral_term_x10
14 right_encoder_delta
15 right_encoder_count
16 control_late_count
17 vofa_tx_count

Right open-loop sequence:
- 0-500 ms: PWM 0
- 500-2000 ms: PWM 40
- 2000-3500 ms: PWM 60
- 3500-5000 ms: PWM 80
- 5000-6500 ms: PWM 100
- 6500-8000 ms: PWM 120
- 8000-9500 ms: PWM 140
- 9500-11000 ms: PWM 160
- 11000-11500 ms: PWM 0
- Then motor stop plus 500 ms stop-state VOFA frames.

Right closed-loop sequence:
- 0-500 ms: 0 RPM
- 500-1100 ms: 0 to 100 RPM linear ramp generated from elapsed_ms
- 1100-2600 ms: 100 RPM hold
- 2600-3200 ms: 100 to 0 RPM linear ramp generated from elapsed_ms
- 3200-3700 ms: 0 RPM
- Then motor stop plus 500 ms stop-state VOFA frames.

SpeedPI calibration interface:
- SpeedPI_UpdateRightCalibrationDirect() is calibration-only.
- It uses the existing right-wheel PI parameters but bypasses the old 0.8 RPM/cycle right-only target ramp.
- Formal SpeedPI_Update() behavior is unchanged.
- SpeedPI_UpdateRightOnly() behavior is unchanged.

Next hardware step:
- Put the car on a stand.
- Run RIGHT_OPEN_LOOP baseline at least 3 times.
- Then switch to RIGHT_CLOSED_LOOP and run baseline at least 3 times.
- Keep VOFA set to FireWater/CSV numeric input at 115200 baud.

---
## SPEED-CAL-002 Left-wheel Calibration Extension

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR LEFT-WHEEL HARDWARE TEST

Baseline status:
- The 600 ms RIGHT_CLOSED_LOOP baseline has passed hardware observation.
- Right-wheel SpeedPI parameters are frozen for now.
- Formal Task3, GrayTrack, route, curve, exit compensation, stop distance, OLED realtime timing, encoder, and motor-control parameters were not changed in this update.

Implemented test cases:
- SPEED_CAL_RIGHT_OPEN_LOOP = 0, unchanged
- SPEED_CAL_RIGHT_CLOSED_LOOP = 1, unchanged
- SPEED_CAL_LEFT_OPEN_LOOP = 2, implemented
- SPEED_CAL_LEFT_CLOSED_LOOP = 3, implemented
- SPEED_CAL_BOTH_EQUAL = 4, enum reserved only

Left-wheel behavior:
- LEFT_OPEN_LOOP uses the same PWM sequence as RIGHT_OPEN_LOOP: 0, 40, 60, 80, 100, 120, 140, 160, 0.
- LEFT_CLOSED_LOOP uses the same elapsed_ms-based 600 ms target ramp as RIGHT_CLOSED_LOOP: 0 to 100 RPM, hold, then 100 to 0 RPM.
- Left closed-loop uses SpeedPI_UpdateLeftCalibrationDirect(), so no hidden target ramp is stacked on top of the generated 600 ms target curve.
- During left-wheel tests, right target/PWM remains zero and the right SpeedPI channel is reset so its integral cannot accumulate.
- End state is SpeedPI_Reset() plus Motor_Coast(); no automatic restart.

Runtime and VOFA:
- Control remains driven by the existing 10 ms board control tick.
- VOFA output remains real-time at 50 ms after each control update, through StarFlash UART2 only.
- No large RAM post-run dump, no ISR formatting/sending, no OLED full refresh.
- VOFA remains exactly 18 integer channels. Channels 6-15 are now active-wheel fields, while right-wheel test channel order stays compatible with the previous right baseline.

Fixed 18-channel VOFA order:
0 elapsed_ms
1 test_case
2 phase
3 command_pwm
4 command_target_rpm_x10
5 control_target_rpm_x10
6 active_raw_rpm_x10
7 active_filtered_rpm_x10
8 active_error_rpm_x10
9 active_pwm
10 active_raw_pwm_x10
11 active_feedforward_pwm_x10
12 active_p_term_x10
13 active_integral_term_x10
14 active_encoder_delta
15 active_encoder_count
16 control_late_count
17 vofa_tx_count

Build verification:
- SPEED_CAL_LEFT_OPEN_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_OPEN_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s)
- TASK_MODE_ARCH001_TIMING_TEST with ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s)
- Final restored default SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)

Final default for burn test:
- SELECTED_TASK_MODE = TASK_MODE_SPEED_CALIBRATION_TEST
- SPEED_CAL_SELECTED_CASE = SPEED_CAL_LEFT_CLOSED_LOOP

Next hardware step:
- Put the car on a stand.
- Collect at least 3 LEFT_CLOSED_LOOP baseline runs.
- Compare channel 5 control_target_rpm_x10 with channel 7 active_filtered_rpm_x10 and channel 9 active_pwm.

---
## SPEED-CAL-003 Both-wheel Equal-target Calibration

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR BOTH-WHEEL GROUND TEST

Baseline status:
- RIGHT_CLOSED_LOOP 600 ms baseline has passed.
- LEFT_CLOSED_LOOP 600 ms baseline has passed.
- The observed left-wheel single-wheel response is slightly faster than the right-wheel response.
- No automatic tuning was performed in this update.

Implemented test case:
- SPEED_CAL_BOTH_EQUAL = 4 is now fully implemented.
- Final default is TASK_MODE_SPEED_CALIBRATION_TEST + SPEED_CAL_BOTH_EQUAL.

Behavior:
- GrayTrack is not read or used.
- Task3 route logic is not run.
- Curve control, entry compensation, exit compensation, heading control, encoder-difference closed loop, synchronization loop, and fixed left/right speed compensation are not used.
- Both wheels receive the exact same common_target_rpm.
- Left and right wheels use their existing formal SpeedPI parameters independently.
- Formal Task3, OledKeyTest, ARCH001, OLED realtime timing, GrayTrack parameters, motor mapping, encoder mapping, and SysConfig-generated files were not changed.

SpeedPI interface:
- Added SpeedPI_UpdateBothCalibrationDirect(left_target_rpm, right_target_rpm, left_sample, right_sample).
- The interface uses the existing left and right PI parameters.
- It uses SPEED_TARGET_STEP_DISABLED, so the 600 ms target ramp generated by SpeedCalibrationTest is not ramp-limited a second time.
- Existing SpeedPI_Update() behavior is unchanged.

Common target curve:
- 0-500 ms: 0 RPM
- 500-1100 ms: elapsed_ms-based linear ramp from 0 to 100 RPM
- 1100-2600 ms: 100 RPM hold
- 2600-3200 ms: elapsed_ms-based linear ramp from 100 to 0 RPM
- 3200-3700 ms: 0 RPM
- 3700-4200 ms: stopped frames, SpeedPI_Reset() + Motor_Coast()
- Target is calculated directly from elapsed_ms and clamped to 0.0-100.0 RPM.

BOTH_EQUAL VOFA 18-channel order for test_case=4:
0 elapsed_ms
1 test_case = 4
2 phase: 0 idle, 9 ramp_up, 10 hold, 11 ramp_down, 8 stop
3 common_target_rpm_x10
4 left_control_target_rpm_x10
5 right_control_target_rpm_x10
6 left_filtered_rpm_x10
7 right_filtered_rpm_x10
8 left_minus_right_rpm_x10
9 left_pwm
10 right_pwm
11 left_integral_term_x10
12 right_integral_term_x10
13 left_encoder_delta
14 right_encoder_delta
15 left_minus_right_count
16 control_late_count
17 vofa_tx_count

Sign definitions:
- Channel 8 positive means left filtered RPM is higher than right filtered RPM.
- Channel 15 positive means the left encoder accumulated more counts than the right encoder.
- Negative channel 15 means the right encoder accumulated more counts.

Build verification:
- SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_OPEN_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_OPEN_LOOP: 0 Error(s), 0 Warning(s)
- TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s)
- TASK_MODE_ARCH001_TIMING_TEST with ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s)
- Final restored default SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s)

Next hardware step:
- Place the car on a flat straight road.
- Run at least 3 BOTH_EQUAL ground-test datasets.
- Watch channels 4/5 for equal control target, channels 6/7/8 for RPM difference, channels 9/10 for PWM difference, channels 11/12 for integral difference, and channel 15 for cumulative encoder difference.
- Decide whether to tune the left-wheel parameters only after comparing repeatable ground-test data.

---
## DUAL-LOOP-DIAG-001 Formal Task3 Dual-loop Diagnostic

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR AB STARTUP DATA COLLECTION

Implemented mode:
- TASK_MODE_DUAL_LOOP_DIAG_TEST added.
- DUAL_LOOP_DIAG_AB_STARTUP = 0.
- DUAL_LOOP_DIAG_ONE_LAP = 1.
- Final default is TASK_MODE_DUAL_LOOP_DIAG_TEST + DUAL_LOOP_DIAG_AB_STARTUP.

Control-chain boundary:
- Formal whole-car control is GrayTrack outer loop plus SpeedPI inner loop.
- TaskDualLoopDiag directly reuses the formal Task3 chain: Task3 state/base speed -> GrayTrack -> left/right target RPM -> SpeedPI -> Motor.
- TaskDualLoopDiag does not copy, rewrite, or recalculate GrayTrack or SpeedPI formulas.
- SPEED_CAL_BOTH_EQUAL remains only two parallel speed inner loops with equal target. It is not cascaded whole-car dual-loop vehicle control.

Diagnostics:
- Added read-only Task3_DiagSnapshot and copy/getter interfaces.
- Snapshot is refreshed from the formal Task3 control cycle after GrayTrack, SpeedPI, and Motor output state are updated.
- Old Task3 debug CSV is disabled while TaskDualLoopDiag runs, so the StarFlash stream contains only the new 24-channel VOFA frames.

Runtime:
- Control remains driven by the existing 10 ms board control tick.
- VOFA output period is 50 ms.
- VOFA uses StarFlash UART2, FireWater-style integer CSV, no header text.
- No ISR formatting/sending and no delay_ms(10) scheduler.
- OLED periodic/full refresh is not used by this diagnostic task.

AB_STARTUP behavior:
- 0-500 ms: Task3/GrayTrack initialized, SpeedPI reset, motor coast, no non-zero PWM.
- 500-3500 ms: formal Task3 one-lap control chain runs normally.
- 3500 ms: diagnostic safety stop only, Task3 stop + SpeedPI_Reset + Motor_Coast.
- 3500-4200 ms: stopped VOFA frames continue.
- After 4200 ms: held stopped forever, no auto restart.

ONE_LAP behavior:
- Starts formal Task3 one-lap mode directly.
- Uses existing AB/BC/CD/DA segment switching, curve speeds, entry/exit compensation, and formal finish stop condition.
- No added 3500 ms safety timeout.

24-channel VOFA order:
0 elapsed_ms
1 diag_case
2 segment
3 progress_x10
4 gray_error
5 correction_rpm_x10
6 base_rpm_x10
7 left_target_rpm_x10
8 right_target_rpm_x10
9 left_actual_rpm_x10
10 right_actual_rpm_x10
11 left_minus_right_actual_rpm_x10
12 left_pwm
13 right_pwm
14 left_integral_term_x10
15 right_integral_term_x10
16 left_encoder_delta
17 right_encoder_delta
18 left_minus_right_count
19 black_mask
20 line_lost
21 curve_lost_hold
22 control_late_count
23 vofa_tx_count

Build verification:
- DUAL_LOOP_DIAG_AB_STARTUP: 0 Error(s), 0 Warning(s)
- DUAL_LOOP_DIAG_ONE_LAP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s)
- ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s)

Next hardware step:
- Collect 3 AB_STARTUP datasets first.
- Use ONE_LAP later for C exit and stop-position analysis.

---
## TASK3-STARTUP-RAMP-001 Common Base-speed Startup Ramp

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR 3 AB_STARTUP HARDWARE RUNS

Observed baseline from AB dual-loop diagnostics:
- Formal Task3 startup previously stepped the common base speed from 0 RPM to 100 RPM.
- During this startup window, GrayTrack error and correction were mostly zero.
- The left wheel responded faster than the right wheel during the step: left peak was about 123-124 RPM, right peak was about 116 RPM.
- Startup right drift was therefore mainly caused by the common base-speed step, not by GrayTrack correction or steady-state SpeedPI mismatch.
- Left/right SpeedPI parameters were not changed.

Implemented change:
- Added TASK3_STARTUP_RAMP_MS = 600U inside formal Task3 control.
- The ramp is applied once per full Task3 task start, from the actual Task3 start timestamp.
- desired_base_rpm is still produced by the existing Task3 route/segment logic.
- ramped_base_rpm is calculated directly from elapsed_ms relative to Task3 start: desired_base_rpm * elapsed_ms / 600 ms.
- Only the common base speed is ramped. GrayTrack correction remains real-time and is not ramped.
- During startup ramp, left/right targets are generated as ramped_base_rpm + correction_rpm and ramped_base_rpm - correction_rpm, clamped to 0..TASK3_TARGET_RPM_MAX to prevent reverse targets.
- After the 600 ms startup ramp, Task3 returns to the existing route speed behavior; later segment, curve, pre-curve, and exit transitions are not limited by this startup ramp.

Diagnostics:
- Task3_DiagSnapshot channel 6 now reports the actual ramped_base_rpm_x10 used for target generation, not the unramped desired base.
- Stop paths clear base_rpm, correction, left/right targets, actual RPM fields, PWM fields, and integral fields in the diagnostic snapshot.
- Segment, progress, encoder delta/count, black_mask, line_lost, control_late_count, and vofa_tx_count remain available.

Expected AB_STARTUP VOFA behavior:
- At the first Task3 control point around 500 ms, channel 6 should be near 0.
- Around 550 ms, channel 6 should be about 83 for 8.3 RPM when desired base is 100 RPM.
- Around 600 ms, channel 6 should be about 167 for 16.7 RPM.
- Around 1050 ms, channel 6 should be about 917 for 91.7 RPM.
- Around 1100 ms and later, channel 6 should be about 1000 for 100 RPM, unless the route business logic requests a different base speed.
- When gray_error=0 and correction=0 during startup, channels 7 and 8 should match channel 6.

Isolation:
- No GrayTrack parameters were changed.
- No SpeedPI gains, feedforward, PWM step limits, anti-windup, encoder filter, motor direction, or SpeedCalibrationTest behavior was changed.
- Formal OledKeyTest and DualLoopDiag reuse the same Task3 startup ramp implementation.

Build verification:
- DUAL_LOOP_DIAG_AB_STARTUP: 0 Error(s), 0 Warning(s)
- DUAL_LOOP_DIAG_ONE_LAP: 0 Error(s), 0 Warning(s)
- TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s)
- SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s)

Next hardware step:
- Burn final default TASK_MODE_DUAL_LOOP_DIAG_TEST + DUAL_LOOP_DIAG_AB_STARTUP.
- Collect 3 AB_STARTUP datasets.
- Verify channel 6 ramp and channels 7/8 target equality when correction is zero.
- After AB startup is stable, run ONE_LAP for C-exit and final-stop analysis.
---
## TASK3-STRAIGHT-SLEW-001 Straight Correction Slew Limiter

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR AB_STARTUP DATA COLLECTION

Hardware observation:
- The 600 ms common base-speed startup ramp has been verified as the correct first fix for startup drift.
- With GrayTrack correction effectively zero, left and right inner speed loops are stable near 100 RPM and the speed difference is mostly about +/-1 RPM.
- Straight-line jerk now correlates with GrayTrack correction jumps on AB/CD. Observed raw correction can jump to about +/-11 RPM, creating a target difference of up to about 22 RPM before the wheel speeds catch up.

Implemented change:
- Added TASK3_STRAIGHT_CORR_SLEW_ENABLE = 1 and TASK3_STRAIGHT_CORR_SLEW_RPM_PER_S = 200.0f.
- The per-control-step limit is calculated from TASK3_CONTROL_MS: 200 RPM/s * 10 ms = 2 RPM/step.
- GrayTrack still computes raw_correction_rpm. Task3 now computes applied_correction_rpm before generating left/right targets.
- Formula: left_target = ramped_base_rpm + applied_correction_rpm; right_target = ramped_base_rpm - applied_correction_rpm.
- Existing target clamps are reused. Startup still allows 0 RPM minimum; normal running uses the existing GrayTrack target min/max.

Scope and safety:
- The slew limiter is applied only on straight segments detected by Task3_IsStraightSegment(), currently AB and CD.
- BC and DA bypass the limiter and synchronize applied_correction_rpm directly to raw_correction_rpm.
- On leaving a straight for a curve, the limiter is deactivated and applied is synced to raw.
- On entering CD from BC, the first straight cycle syncs applied to raw, then later straight cycles are limited.
- On task start in AB, raw=0 and applied=0 are initialized after diagnostic clearing, so the first AB correction starts from zero.
- line_lost bypasses the straight slew path by requiring line_detected before limiting; lost-line stop/hold behavior is unchanged.
- Stop, complete, exception, and diagnostic safety-stop paths clear raw/applied correction, targets, base, actual RPM, PWM, and integral fields.

DualLoopDiag protocol:
- Fixed VOFA frame expanded from 24 to 26 integer channels.
- Channel 5 is raw_correction_rpm_x10.
- Channel 6 is applied_correction_rpm_x10.
- Channel 7 is ramped_base_rpm_x10.
- Channel 23 is straight_slew_active.
- Channels 24 and 25 are control_late_count and vofa_tx_count.
- SpeedCalibrationTest remains unchanged at 18 channels.

Build verification:
- DUAL_LOOP_DIAG_AB_STARTUP, slew enabled: 0 Error(s), 0 Warning(s)
- DUAL_LOOP_DIAG_AB_STARTUP, slew disabled: 0 Error(s), 0 Warning(s)
- DUAL_LOOP_DIAG_ONE_LAP, slew enabled: 0 Error(s), 0 Warning(s)
- TASK_MODE_OLED_KEY_TEST, slew enabled: 0 Error(s), 0 Warning(s)
- SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s)
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s)
- ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s)
- Final default build: 0 Error(s), 0 Warning(s)

Final default for burn test:
- SELECTED_TASK_MODE = TASK_MODE_DUAL_LOOP_DIAG_TEST
- DUAL_LOOP_DIAG_SELECTED_CASE = DUAL_LOOP_DIAG_AB_STARTUP
- TASK3_STRAIGHT_CORR_SLEW_ENABLE = 1
- TASK3_STRAIGHT_CORR_SLEW_RPM_PER_S = 200.0f
- SPEED_CAL_SELECTED_CASE remains SPEED_CAL_BOTH_EQUAL for later speed tests.

Next hardware step:
1. Burn the final default firmware.
2. Collect 3 AB_STARTUP datasets first.
3. In VOFA, compare channel 5 raw_correction_rpm_x10 with channel 6 applied_correction_rpm_x10. On AB, applied should move toward raw by no more than about 20 x10 units per 10 ms control cycle.
4. Confirm channels 8 and 9 follow channel 7 +/- channel 6.
5. After AB is repeatable, switch to ONE_LAP for C-exit observation. C-exit remains a separate tuning topic.
---
## DUAL-LOOP-DIAG-002 One-lap Diagnostic Default

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR THREE ONE_LAP RUNS

AB_STARTUP hardware verification status:
- The 600 ms common base-speed startup ramp has passed hardware observation.
- The 200 RPM/s straight correction slew limiter has passed AB_STARTUP observation.
- raw_correction_rpm_x10 and applied_correction_rpm_x10 are separated correctly.
- straight_slew_active behaves correctly in AB_STARTUP.
- Stop snapshots clear targets, correction, base, PWM, RPM, and integrals.
- control_late_count remained zero in the AB_STARTUP data.

Default change:
- SELECTED_TASK_MODE remains TASK_MODE_DUAL_LOOP_DIAG_TEST.
- DUAL_LOOP_DIAG_SELECTED_CASE is now DUAL_LOOP_DIAG_ONE_LAP.
- TASK3_STARTUP_RAMP_MS remains 600U.
- TASK3_STRAIGHT_CORR_SLEW_ENABLE remains 1.
- TASK3_STRAIGHT_CORR_SLEW_RPM_PER_S remains 200.0f.

ONE_LAP behavior:
- ONE_LAP directly reuses the formal Task3 control chain through Task3_LinkedOperation_StartMode(start_ms, TASK3_RUN_ONE_LAP) and Task3_LinkedOperation_Update(now_ms).
- TaskDualLoopDiag does not duplicate route switching, GrayTrack, SpeedPI, odometry, stop logic, or curve/exit compensation.
- The AB_STARTUP 3500 ms diagnostic stop is not used by ONE_LAP.
- ONE_LAP ends only when formal Task3 reports completion through its finish-stop condition.
- After formal Task3 completes, TaskDualLoopDiag sends stopped 26-channel VOFA frames for 1000 ms, then holds stopped forever without auto-restart.

Expected straight_slew_active values:
- AB straight: 1
- BC curve: 0
- CD straight: 1
- DA curve: 0
- stopped frames after completion: 0

VOFA protocol:
- The fixed 26 integer channels remain unchanged.
- 50 ms real-time StarFlash UART2 output is retained.
- No header, Chinese text, mixed debug text, dynamic allocation, or post-run bulk dump is used.

Control-parameter isolation:
- No SpeedPI gains, feedforward, PWM step limits, GrayTrack Kp, error map, correction max, startup ramp, straight correction slew rate, base speeds, segment lengths, route switching, exit compensation, finish-stop offset, encoder scaling/filtering, motor direction, or Motor_Brake behavior was changed.

Next hardware step:
1. Burn the final default firmware.
2. Run three full ONE_LAP diagnostic laps.
3. Check C exit, CD straight stability, DA entry/exit, and final stop position.
4. Do not change stop compensation until the three ONE_LAP datasets are compared.
---
## DUAL-LOOP-DIAG-003 Finish Extra Advance 1.5cm

Date: 2026-08-01
Status: CODE COMPLETE, BUILD VERIFIED, READY FOR FIVE ONE_LAP STOP TESTS

Hardware observation before this change:
- Full ONE_LAP control completed repeatably in about 18.65 s.
- control_late_count stayed at 0 in the observed run.
- AB/BC/CD/DA segment switching was repeatable.
- 600 ms startup ramp, 200 RPM/s straight correction slew, and both SpeedPI loops behaved normally.
- Real car stop position was typically about 2 to 3 cm past the finish black line.

Single-variable change:
- Added TASK3_FINISH_EXTRA_ADVANCE_ENABLE in Application/Task3_LinkedOperation.c.
- Added TASK3_FINISH_EXTRA_ADVANCE_CM = 1.5f.
- The extra advance is added on top of the existing TASK3_FINISH_STOP_OFFSET_CM = 33.0f.
- Existing finish window, confirm count, route switching, stop action, speed targets, GrayTrack parameters, SpeedPI parameters, encoder scaling, motor behavior, and VOFA 26-channel protocol were not changed.

Finish trigger math:
- Original formula: finish_trigger_count = lap_target_count - DistanceCmToCount(33.0 cm).
- New formula when enabled: finish_trigger_count = lap_target_count - DistanceCmToCount(33.0 cm) - DistanceCmToCount(1.5 cm).
- Disabled formula: same as the original formula.
- Distance conversion reuses Task3_DistanceCmToCount().
- Current conversion gives about 685.59 counts/cm.
- 1.5 cm extra advance converts to 1029 counts.
- Lap target count is 421062.
- Existing 33.0 cm advance converts to 22625 counts.
- Old finish trigger threshold is 398437.
- New finish trigger threshold is 397408.
- The new threshold is 1029 counts smaller, so the stop trigger is earlier.

Task scope:
- Applies to TASK3_RUN_ONE_LAP and TASK3_RUN_ONE_LAP_ALT because they use Task3_GetFinishStopCount().
- Applies to DUAL_LOOP_DIAG_ONE_LAP because it directly reuses formal Task3 one-lap logic.
- Applies to OLED_KEY_TEST Task2 and Task4 one-lap modes.
- Does not apply to TASK3_RUN_B_PLUS_5CM / OLED_KEY_TEST Task3 because that mode uses Task3_GetBPlusStopTargetCount().
- Does not change speed calibration, ARCH001 timing, or AB_STARTUP diagnostic behavior.

Build verification:
- DUAL_LOOP_DIAG_ONE_LAP, extra enabled: 0 Error(s), 0 Warning(s).
- DUAL_LOOP_DIAG_ONE_LAP, extra disabled: 0 Error(s), 0 Warning(s).
- DUAL_LOOP_DIAG_AB_STARTUP: 0 Error(s), 0 Warning(s).
- TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s).
- SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s).
- SPEED_CAL_LEFT_CLOSED_LOOP: 0 Error(s), 0 Warning(s).
- SPEED_CAL_RIGHT_CLOSED_LOOP: 0 Error(s), 0 Warning(s).
- ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s).
- Final default build: 0 Error(s), 0 Warning(s).

Final default for burn test:
- SELECTED_TASK_MODE = TASK_MODE_DUAL_LOOP_DIAG_TEST.
- DUAL_LOOP_DIAG_SELECTED_CASE = DUAL_LOOP_DIAG_ONE_LAP.
- TASK3_STARTUP_RAMP_MS = 600U.
- TASK3_STRAIGHT_CORR_SLEW_ENABLE = 1.
- TASK3_STRAIGHT_CORR_SLEW_RPM_PER_S = 200.0f.
- TASK3_FINISH_EXTRA_ADVANCE_ENABLE = 1.
- TASK3_FINISH_EXTRA_ADVANCE_CM = 1.5f.

Next hardware step:
1. Burn the final default firmware.
2. Run five full ONE_LAP diagnostic laps without changing other parameters.
3. Record final physical stop error relative to the finish black line for each lap.
4. Save the final stopped VOFA frame for each run and compare progress_x10, segment, control_late_count, and vofa_tx_count.
5. If all five stops remain late, continue with 0.5 cm earlier steps. If stops scatter both early and late, stop changing trigger distance and evaluate braking/stop action instead.
---
## FINAL-RELEASE-001 Competition Default Build

Date: 2026-08-01
Status: FINAL RELEASE CONFIGURED, BUILD VERIFIED

Release entry:
- FINAL_RELEASE_BUILD = 1 in Application/BuildConfig.h.
- SELECTED_TASK_MODE = TASK_MODE_OLED_KEY_TEST in main.c.
- Power-on enters RobotPlatform_Init, OLED menu, Task1 Standby. The car does not auto-run after power-on.

OLED menu:
- Task1 Standby.
- Task2 One Lap.
- Task3 B+5cm.
- Task4 Ball Lap.
- K1 start/stop and K2 task switching remain in OledKeyTest.
- Task switching remains blocked while running.
- OLED realtime integer-second timing remains enabled during trace tasks through OledRealtimeTime partial updates.

Formal motion profiles in Application/Task3_LinkedOperation.c:
- kTask2Profile: straight_base_rpm = 100.0f, curve_base_rpm = 88.0f, startup_ramp_ms = 600U, finish_extra_advance_enable = 1U.
- kTask3Profile: straight_base_rpm = 70.0f, curve_base_rpm = 60.0f, startup_ramp_ms = 600U, finish_extra_advance_enable = 0U.
- kTask4Profile: straight_base_rpm = 75.0f, curve_base_rpm = 60.0f, startup_ramp_ms = 600U, finish_extra_advance_enable = 1U.

Task mapping:
- TASK3_RUN_ONE_LAP selects kTask2Profile.
- TASK3_RUN_B_PLUS_5CM selects kTask3Profile.
- TASK3_RUN_ONE_LAP_ALT selects kTask4Profile.
- The profile is selected once in Task3_LinkedOperation_StartMode() and stored in g_task3_motion_profile.
- Runtime base speed generation reads only the active profile.

Task3 B+5cm behavior:
- Stop condition remains Task3_GetBPlusStopTargetCount(): straight target count plus DistanceCmToCount(5.0 cm).
- It does not use the full-lap 1.5 cm finish extra advance.
- AB uses the 70 RPM straight profile value.
- Pre-curve/BC base-speed generation uses the active profile curve value, 60 RPM.

Finish extra advance scope:
- TASK3_FINISH_EXTRA_ADVANCE_ENABLE remains 1.
- TASK3_FINISH_EXTRA_ADVANCE_CM remains 1.5f.
- Application is now controlled by the active profile.
- Task2 and Task4 enable the extra 1.5 cm full-lap stop advance.
- Task3 B+5cm disables it and keeps the B+5cm stop target.
- DUAL_LOOP_DIAG_ONE_LAP uses TASK3_RUN_ONE_LAP and therefore the Task2 one-lap profile when debug builds are selected.

Release debug-output policy:
- TASK3_PERIODIC_CSV_ENABLE = 0U.
- DUAL_LOOP_DIAG_TELEMETRY_ENABLE = 0U.
- SPEED_CAL_TELEMETRY_ENABLE = 0U.
- ARCH001_TEXT_OUTPUT_ENABLE = 0U.
- CONTROL_DEBUG_PRINT_ENABLE = 0U.
- Final release does not send Task3 periodic CSV, 26-channel dual-loop VOFA, 18-channel speed-calibration VOFA, ARCH001 text, OLED menu ready text, or periodic debug strings.
- StarFlash UART receive/send interfaces are not globally disabled, so formal business communication remains available.

Unchanged control items:
- SpeedPI gains, feedforward, PWM step limits, anti-windup, and reset behavior were not changed.
- GrayTrack Kp, error map, correction limits, line-lost and curve-lost handling were not changed.
- Encoder scaling, wheel diameter, segment lengths, B+5cm definition, motor direction, and 10 ms control period were not changed.
- Straight correction slew remains enabled at 200 RPM/s.

Build verification:
- FINAL_RELEASE_BUILD=1, TASK_MODE_OLED_KEY_TEST: 0 Error(s), 0 Warning(s).
- FINAL_RELEASE_BUILD=0, DUAL_LOOP_DIAG_ONE_LAP: 0 Error(s), 0 Warning(s).
- FINAL_RELEASE_BUILD=0, SPEED_CAL_BOTH_EQUAL: 0 Error(s), 0 Warning(s).
- FINAL_RELEASE_BUILD=0, ARCH001_CASE_E_REALTIME_TIME: 0 Error(s), 0 Warning(s).
- Final default restored and rebuilt: 0 Error(s), 0 Warning(s).

Minimum burn-test steps:
1. Burn the final default firmware.
2. Confirm power-on stays on Task1 Standby and the motor does not auto-run.
3. Press K2 to Task2, press K1, verify one-lap run around 19 s and final stop position.
4. Press K2 to Task3, press K1, verify B+5cm stop and lower 70/60 RPM behavior near B/BC.
5. Press K2 to Task4, press K1, verify one-lap ball-load stability at 75/60 RPM.
6. Confirm no VOFA/CSV/debug text is emitted in final release while OLED timing still updates.