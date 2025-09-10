#include <Stepper.h>

/* ===== Pines ===== */
// L298 #1 (Motor A)
#define A_IN1 2
#define A_IN2 3
#define A_IN3 4
#define A_IN4 5
// L298 #2 (Motor B)
#define B_IN1 8
#define B_IN2 9
#define B_IN3 10
#define B_IN4 11
// Ultrasonido
#define TRIG  6
#define ECHO  7
// LED y Botones
#define LED_ROJO 13
#define BTN_HOR  31   // clic = toggle horario
#define BTN_ANTI 33   // clic = toggle antihorario

/* ===== Parámetros ===== */
const int PASOS_POR_VUELTA = 200;   // 1.8°
const int RPM = 20;
const int UMBRAL_CM = 10;           // parar si < 10 cm

Stepper motorA(PASOS_POR_VUELTA, A_IN1, A_IN2, A_IN3, A_IN4);
Stepper motorB(PASOS_POR_VUELTA, B_IN1, B_IN2, B_IN3, B_IN4);

/* ---------- Ultrasonido ---------- */
unsigned long medirPulso() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, 30000UL); // 30 ms timeout
}
long distanciaCm() {
  // Mediana de 3 lecturas para estabilidad
  unsigned long t1 = medirPulso(); delayMicroseconds(100);
  unsigned long t2 = medirPulso(); delayMicroseconds(100);
  unsigned long t3 = medirPulso();
  if (t1 > t2) { unsigned long t=t1; t1=t2; t2=t; }
  if (t2 > t3) { unsigned long t=t2; t2=t3; t3=t; }
  if (t1 > t2) { unsigned long t=t1; t1=t2; t2=t; }
  unsigned long dur = t2;
  if (!dur) return 9999;  // sin eco: lejos
  return (long)((dur * 0.0343f) / 2.0f);
}

/* ---------- Botones por “clic” (borde descendente) + debounce ---------- */
bool fallingEdge(uint8_t pin) {
  static bool last[70];
  static unsigned long tMark[70] = {0};
  bool cur = (digitalRead(pin) == LOW);  // INPUT_PULLUP: LOW = presionado
  unsigned long now = millis();
  if (cur != last[pin] && (now - tMark[pin]) < 20) return false;   // debounce
  bool edge = (cur && !last[pin]);          // HIGH->LOW (clic)
  if (cur != last[pin]) { last[pin] = cur; tMark[pin] = now; }
  return edge;
}

/* ---------- Estado ---------- */
// -1 = horario, +1 = antihorario, 0 = parado
int direccionPedida = 0;

// para imprimir distancia cada cierto tiempo
unsigned long tLog = 0;
const unsigned long LOG_MS = 200;

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT);  digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT);

  pinMode(LED_ROJO, OUTPUT);  digitalWrite(LED_ROJO, LOW);

  pinMode(BTN_HOR,  INPUT_PULLUP);
  pinMode(BTN_ANTI, INPUT_PULLUP);

  motorA.setSpeed(RPM);
  motorB.setSpeed(RPM);

  Serial.println("Controles por CLIC: BTN31 horario (toggle), BTN33 antihorario (toggle).");
  Serial.println("Obstaculo <10 cm = stop + LED13 ON; al despejar retoma.");
}

void loop() {
  // 1) Clics
  if (fallingEdge(BTN_HOR))  direccionPedida = (direccionPedida == -1) ? 0 : -1;
  if (fallingEdge(BTN_ANTI)) direccionPedida = (direccionPedida == +1) ? 0 : +1;

  // 2) Distancia + LED
  long d = distanciaCm();
  bool obstaculo = (d < UMBRAL_CM);
  digitalWrite(LED_ROJO, obstaculo ? HIGH : LOW);

  // 3) Mover (no bloqueante)
  if (!obstaculo && direccionPedida != 0) {
    motorA.step(direccionPedida);
    motorB.step(direccionPedida);

  }

  // 4) LOG periódico de distancia y estado (cada 200 ms)
  unsigned long now = millis();
  if (now - tLog >= LOG_MS) {
    tLog = now;
    Serial.print("d = ");
    Serial.print(d);
    Serial.print(" cm | estado: ");
    if (obstaculo) {
      Serial.print("OBSTACULO (<10 cm) | ");
    } else {
      Serial.print("OK | ");
    }
    Serial.print("dirPedida = ");
    if (direccionPedida == -1)      Serial.println("HORARIO");
    else if (direccionPedida == +1) Serial.println("ANTIHORARIO");
    else                            Serial.println("PARADO");
  }

  delay(1);
}
