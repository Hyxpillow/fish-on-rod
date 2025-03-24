#include "Keyboard.h"

// 定义状态和信号
enum State {
  STATE_1 = 1,
  STATE_2 = 2,
  STATE_2_5 = 25, // 新添加的状态2.5
  STATE_3 = 3,
  STATE_4 = 4,
  STATE_5 = 5,
  STATE_6 = 6,
  STATE_7 = 7
};

enum Signal {
  NONE = 0,
  SIGNAL_A = 'A',
  SIGNAL_B = 'B',
  SIGNAL_C = 'C',
  SIGNAL_D = 'D',
  SIGNAL_E = 'E'
};

// 当前状态
State currentState = STATE_1;

// 上一次动作的时间戳
unsigned long lastActionTime = 0;
bool isKeyPressed = false;
unsigned long keyPressTime = 0;

// 随机按键延迟范围（毫秒）
const int MIN_KEY_PRESS_DELAY = 30;  // 最小延迟
const int MAX_KEY_PRESS_DELAY = 100;  // 最大延迟

// 状态3的随机间隔范围（毫秒）
const int MIN_STATE3_INTERVAL = 1200;  // 最小间隔
const int MAX_STATE3_INTERVAL = 3000;  // 最大间隔

// 状态3的随机长按持续时间范围（毫秒）
const int MIN_STATE3_PRESS_DURATION = 400;   // 最小持续时间
const int MAX_STATE3_PRESS_DURATION = 900;  // 最大持续时间

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
  delay(1000);
}

// 生成随机按键延迟
int getRandomKeyDelay() {
  return random(MIN_KEY_PRESS_DELAY, MAX_KEY_PRESS_DELAY + 1);
}

// 更新状态3的随机参数
void updateState3RandomParams() {
  currentState3Interval = random(MIN_STATE3_INTERVAL, MAX_STATE3_INTERVAL + 1);
  currentState3Duration = random(MIN_STATE3_PRESS_DURATION, MAX_STATE3_PRESS_DURATION + 1);
}

void loop() {
  // 检查串口是否有数据
  Signal receivedSignal = NONE;
  if (Serial.available() > 0) {
    char incomingByte = Serial.read();
    // 只接受指定的信号
    if (incomingByte == SIGNAL_A || incomingByte == SIGNAL_B || 
        incomingByte == SIGNAL_C || incomingByte == SIGNAL_D || 
        incomingByte == SIGNAL_E) {
      receivedSignal = (Signal)incomingByte;
    }
  }
  
  // 根据当前状态执行操作
  executeStateAction();
  
  // 处理状态转移
  handleStateTransition(receivedSignal);
}

// 执行当前状态对应的操作
void executeStateAction() {
  unsigned long currentTime = millis();
  
  switch (currentState) {
    case STATE_1:
      // 单击.键（如果距离上次动作已经过去足够时间）
      Keyboard.press('.');
      delay(getRandomKeyDelay());  // 随机短暂按下
      Keyboard.release('.');
      lastActionTime = currentTime;
      break;
      
    case STATE_2:
      // 无操作
      break;
      
    case STATE_2_5:
      // 单击.键（类似STATE_1）
      Keyboard.press('.');
      delay(getRandomKeyDelay());  // 随机短暂按下
      Keyboard.release('.');
      break;
      
    case STATE_3:
      // 每隔随机时间长按随机时间/键
      if (!isKeyPressed && currentTime - lastActionTime > currentState3Interval) {
        Keyboard.press('/');
        isKeyPressed = true;
        keyPressTime = currentTime;
      } else if (isKeyPressed && currentTime - keyPressTime > currentState3Duration) {
        Keyboard.release('/');
        isKeyPressed = false;
        lastActionTime = currentTime;
        // 每次释放按键后更新随机参数，为下一次循环准备
        updateState3RandomParams();
      }
      break;
      
    case STATE_4:
      // 长按.键和f键
      if (!isKeyPressed) {
        Keyboard.press('.');
        Keyboard.press(';');
        isKeyPressed = true;
      }
      break;
      
    case STATE_5:
      // 单击c键（如果距离上次动作已经过去足够时间）
      Keyboard.press(',');
      delay(getRandomKeyDelay());  // 随机短暂按下
      Keyboard.release(',');
      break;
      
    case STATE_6:
      // 同时长按.键和f键(已经保留)和/键
      // 从STATE_4转换过来时，.和f键已经按下，只需要按/键
      Keyboard.press('/');
      break;
      
    case STATE_7:
      // 单击空格（如果距离上次动作已经过去足够时间）
      Keyboard.press(' ');
      delay(getRandomKeyDelay());  // 随机短暂按下
      Keyboard.release(' ');
      break;
  }
}

// 处理状态转移
void handleStateTransition(Signal signal) {
  // 特殊处理STATE_4到STATE_6的转换（保留.键和f键）
  bool keepKeysForState6 = (currentState == STATE_4 && signal == SIGNAL_D);
  
  // 在状态转移前释放所有按键（除了特殊情况）
  if (isKeyPressed && !keepKeysForState6 && (
      (currentState == STATE_3 && signal != NONE) ||
      (currentState == STATE_4 && signal != NONE) ||
      (currentState == STATE_6 && signal != NONE) ||
      (currentState != STATE_3 && currentState != STATE_4 && currentState != STATE_6)
     )) {
    Keyboard.releaseAll();
    isKeyPressed = false;
  }
  
  // 根据当前状态和接收到的信号进行状态转移
  switch (currentState) {
    case STATE_1:
      currentState = STATE_2;  // 从状态1只能转移到状态2
      break;
      
    case STATE_2:
      if (signal == SIGNAL_B) {
        currentState = STATE_2_5;  // 从状态2接收到信号B，转移到状态2.5
      } else if (signal == SIGNAL_A) {
        currentState = STATE_4;
      }
      // 否则保持在状态2
      break;
      
    case STATE_2_5:
      currentState = STATE_3;  // 状态2.5无条件跳转到状态3
      break;
      
    case STATE_3:
      if (signal == SIGNAL_A) {
        currentState = STATE_4;
      }
      // 否则保持在状态3
      break;
      
    case STATE_4:
      if (signal == SIGNAL_C) {
        currentState = STATE_5;
      } else if (signal == SIGNAL_D) {
        currentState = STATE_6;
        // 从STATE_4到STATE_6保持keys按下状态
        // 不需要重置isKeyPressed，让它保持为true
      }
      // 否则保持在状态4
      break;
      
    case STATE_5:
      currentState = STATE_2;  // 从状态5只能转移到状态2
      break;
      
    case STATE_6:
      if (signal == SIGNAL_E) {
        currentState = STATE_7;
      }
      // 否则保持在状态6
      break;
      
    case STATE_7:
      currentState = STATE_1;  // 从状态7只能转移到状态1
      break;
  }
  
  // 状态变化后重置计时器
  if (signal != NONE) {
    lastActionTime = millis();
  }
}


// int RXLED = 17;  // The RX LED has a defined Arduino pin
// void setup(){
//   pinMode(RXLED, OUTPUT);  // Set RX LED as an output
// }
// void loop() {
//   digitalWrite(RXLED, LOW);   // set the RX LED ON
//   delay(500);
//   digitalWrite(RXLED, HIGH);    // set the RX LED OFF
//   delay(500);
// }

