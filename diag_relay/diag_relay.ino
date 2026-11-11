/*
 * 继电器引脚/触发电平诊断（不初始化串口，避免占用 IO1=TX）
 * 上电后循环 4 段，每段 3 秒：
 *   段1 (0-3s)  : IO0=LOW , IO1=HIGH  -> 测 IO0 低电平触发
 *   段2 (3-6s)  : IO0=HIGH, IO1=LOW   -> 测 IO1 低电平触发
 *   段3 (6-9s)  : IO0=LOW , IO1=LOW   -> 对照：两脚都低
 *   段4 (9-12s) : IO0=HIGH, IO1=HIGH  -> 测高电平触发（两脚都高）
 * 观察继电器/指示灯在哪一段吸合，即可判断信号脚与触发方式。
 */
#define PIN_A 0   // IO0
#define PIN_B 1   // IO1 = TX

void setup() {
  pinMode(PIN_A, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  digitalWrite(PIN_A, HIGH);
  digitalWrite(PIN_B, HIGH);
}

void loop() {
  // 段1: IO0 低
  digitalWrite(PIN_A, LOW);  digitalWrite(PIN_B, HIGH); delay(3000);
  // 段2: IO1 低
  digitalWrite(PIN_A, HIGH); digitalWrite(PIN_B, LOW);  delay(3000);
  // 段3: 都低
  digitalWrite(PIN_A, LOW);  digitalWrite(PIN_B, LOW);  delay(3000);
  // 段4: 都高
  digitalWrite(PIN_A, HIGH); digitalWrite(PIN_B, HIGH); delay(3000);
}
