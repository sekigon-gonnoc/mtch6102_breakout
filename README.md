# mtch6102_breakout
Breakout board of MTCH6102, a 2D capacitive touch sensor, for DIY split keyboards with TRRS connectors.

静電容量式タッチセンサIC MTCH6102のブレークアウトボードです。
2個のTRRSコネクタを取り付けることで、自作スプリットキーボードの間に挟んで動作させることができます。(キーボードがI2Cでの通信に対応している必要があります)

## はんだ付けの注意

表面実装部品は左側の列から付けることをお勧めします。ただし、C1, C3はそれぞれU1, U2の後に付けたほうがよいです。

D1, D2, D3, U2をつける際は向きに注意してください。
![半田面](https://github.com/sekigon-gonnoc/mtch6102_breakout/blob/master/bottom.JPG "はんだ面")

|シルク|個数|パッケージ|部品|
|--|--|--|--|
|Q1|1|SOT-23|Pch FET|
|Q2|1|SOT-363|Nch FET|
|R1|1|1608抵抗|10kΩ|
|R2, R3, R4, R5|4|1608抵抗|2.2kΩ|
|C1, C2|2|1608コンデンサ|1μF|
|C3|1|1608コンデンサ|0.1μF|
|D1|1|SOT-323|ショットキーバリアダイオード|
|D2, D3|2|SOT-323|ダイオード|
|U1|1|SOT23-5|三端子レギュレータ|
|U2|1|SSOP28|MTCH6102|

## プログラム例
```
#define MTCH6102_READ_ADDR 0x25 // or ((0x25<<1)|1)
#define MTCH6102_REG_STAT 0x10

typedef union {
  struct {
    uint8_t status, x_msb, y_msb, xy_lsb, gesture;
  };
  uint8_t dat[5];
} mtch6102_reg_t;

enum {
  GES_TAP = 0x10,
  GES_DOUBLE_TAP = 0x20
} GESTURE_CODE;

enum {
  TOUCH = 1<<0,
  GESTURE= 1<<1,
} MTCH6102_STAT_BIT;

void send_mouse(report_mouse_t *report);
int process_mtch6102() {
  mtch6102_reg_t reg;
  static uint16_t x, y;
  static uint16_t x_buf, y_buf;
  static bool touch_state;
  static bool release_button = false;
  bool send_flag = false;

  report_mouse_t rep_mouse;
  uint16_t x_dif, y_dif;

  if (i2c_readReg(MTCH6102_READ_ADDR, MTCH6102_REG_STAT, reg.dat,
      sizeof(mtch6102_reg_t), 0))
    return 1;

  memset(&rep_mouse, 0, sizeof(report_mouse_t));
  x = (reg.x_msb << 4) | (reg.xy_lsb >> 4);
  y = (reg.y_msb << 4) | (reg.xy_lsb & 0xF);

  if (touch_state && (reg.status & TOUCH)) {
    x_dif = x - x_buf;
    y_dif = y - y_buf;
    rep_mouse.x = x_dif * -1;
    rep_mouse.y = y_dif * -1;
    send_flag = true;
  }
  if ((reg.status & GESTURE)
      && (reg.gesture == GES_TAP || reg.gesture == GES_DOUBLE_TAP)) {
    rep_mouse.buttons = 1;
    release_button = true;
    send_flag = true;
  } else if (release_button) {
    rep_mouse.buttons = 0;
    send_flag = true;
  }
  if (send_flag) {
    send_mouse(&rep_mouse);
  }
  touch_state = reg.status & TOUCH;
  x_buf = x;
  y_buf = y;

  return 0;
}
```
