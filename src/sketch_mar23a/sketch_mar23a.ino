#include "Keyboard.h"

enum Signal {
  NONE = 0,
  SIGNAL_1 = '1',
  SIGNAL_2 = '2',
  SIGNAL_3 = '3',
  SIGNAL_4 = '4',
  SIGNAL_5 = '5',
  SIGNAL_6 = '6',
  SIGNAL_7 = '7'
};

// 上一次动作的时间戳
unsigned long lastActionTime = 0;
bool isKeyPressed_1 = false; 
bool isKeyPressed_2 = false;
bool isKeyPressed_3 = false;
unsigned long keyPressTime = 0;

int current_state = 0;

// 状态3的随机间隔范围（毫秒）
const int MIN_STATE3_INTERVAL = 2525;  // 最小间隔
const int MAX_STATE3_INTERVAL = 4725;  // 最大间隔

// 状态3的随机长按持续时间范围（毫秒）
const int MIN_STATE3_PRESS_DURATION = 400;   // 最小持续时间
const int MAX_STATE3_PRESS_DURATION = 800;  // 最大持续时间

// 当前状态3的参数
int currentState3Interval = 1500;    // 默认值，会在使用前被随机替换
int currentState3Duration = 1000;    // 默认值，会在使用前被随机替换

void setup() {
  // 初始化键盘
  Keyboard.begin();
  // 初始化串口通信，用于接收信号
  Serial.begin(9600);
  // 初始化随机数生成器
  randomSeed(analogRead(0));
  // 初始化状态3的随机参数
  updateState3RandomParams();
  // 等待串口稳定
  delay(2000);
  Keyboard.releaseAll();
}

// 更新状态3的随机参数
void updateState3RandomParams() {
  currentState3Interval = random(MIN_STATE3_INTERVAL, MAX_STATE3_INTERVAL + 1);
  currentState3Duration = random(MIN_STATE3_PRESS_DURATION, MAX_STATE3_PRESS_DURATION + 1);
}

void click(char key) {
  Keyboard.press(key);
  delay(random(40, 101));  // 随机短暂按下
  Keyboard.release(key);
}

void releaseAll() {
  if (isKeyPressed_1) {
    Keyboard.release('.');
    isKeyPressed_1 = false;
  }
  if (isKeyPressed_2) {
    Keyboard.release(';');
    isKeyPressed_2 = false;
  }
  if (isKeyPressed_3) {
    Keyboard.release('/');
    isKeyPressed_3 = false;
  }
}

void loop() {
  // 检查串口是否有数据
  Signal receivedSignal = NONE;
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    // 只接受指定的信号
    if (incomingByte == SIGNAL_1 || incomingByte == SIGNAL_2 || 
        incomingByte == SIGNAL_3 || incomingByte == SIGNAL_4 || 
        incomingByte == SIGNAL_5 || incomingByte == SIGNAL_6 || incomingByte == SIGNAL_7) {
      receivedSignal = (Signal)incomingByte;
    }
  }
  // 1单击左键(抛竿) 2关闭线杯+轻抽(到达指定位置) 3长按左键+加速(咬钩) 4打开线杯(滑口) 5长按右键(到达高度) 6空格(入户)
  switch (receivedSignal)
  {
  case SIGNAL_1:
    releaseAll();
    current_state = 1;
    break;
  case SIGNAL_2:
    if (current_state != 2) {
      click('.');
      current_state = 2;
    }
    break;
  case SIGNAL_3:
    delay(random(200, 600));
    releaseAll();
    delay(random(20, 100));
    Keyboard.press('.');
    isKeyPressed_1 = true;
    delay(random(20, 100));
    Keyboard.press(';');
    isKeyPressed_2 = true;
    current_state = 3;
    break;
  case SIGNAL_4:
    releaseAll();
    click(',');
    current_state = 4;
    break;
  case SIGNAL_5:
    if (current_state != 5) {
      Keyboard.press('/');
      isKeyPressed_3 = true;
      current_state = 5;
    }
    break;
  case SIGNAL_6:
    releaseAll();
    delay(random(1200, 1800));
    click(' ');
    delay(random(100, 1000));
    click('.');
    current_state = 6;
    break;
  case SIGNAL_7:
    releaseAll();
    delay(random(1200, 1800));
    click(KEY_BACKSPACE);
    delay(random(100, 1000));
    click('.');
    current_state = 7;
    break;
  default:
    break;
  }
  unsigned long currentTime = millis();
  if (current_state == 2) {
      if (!isKeyPressed_3 && currentTime - lastActionTime > currentState3Interval) {
          // 如果当前没有按下键，并且距离上次动作已经超过了间隔时间
          Keyboard.press('/');
          isKeyPressed_3 = true;
          keyPressTime = currentTime;
      } else if (isKeyPressed_3 && currentTime - keyPressTime > currentState3Duration) {
          // 如果已经按下键，并且按键时间已经超过了预定的持续时间
          Keyboard.release('/');
          isKeyPressed_3 = false;
          lastActionTime = currentTime;
          // 每次释放按键后更新随机参数，为下一次循环准备
          updateState3RandomParams();
      }
  }

}

