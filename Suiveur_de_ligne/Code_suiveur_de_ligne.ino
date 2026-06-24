#define LEFT_SENSOR_PIN  2
#define RIGHT_SENSOR_PIN 4

// L298N
#define ENA_PIN   3
#define IN1_PIN   8
#define IN2_PIN   9

#define ENB_PIN   5
#define IN3_PIN   10
#define IN4_PIN   11

// Vitesses
#define BASE_SPEED  120
#define MAX_SPEED   255
#define MIN_SPEED   0

// =====================
// PID
// =====================
float Kp = 120.0;   // Proportionnel
float Ki = 0.0;    // Intégral (à ajuster si le robot dérive)
float Kd = 50.0;   // Dérivé (amortit les oscillations)

float lastError   = 0;
float integral    = 0;

int lastDirection = 0;
unsigned long lostLineTime = 0;
bool lineLost = false;

unsigned long lastTime = 0;

//==================================================
// SETUP
//==================================================
void setup() {
  Serial.begin(9600);

  pinMode(LEFT_SENSOR_PIN,  INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  pinMode(ENA_PIN, OUTPUT);
  pinMode(ENB_PIN, OUTPUT);
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);

  stopMotors();
  delay(2000);
}

//==================================================
// LOOP
//==================================================
void loop() {

  bool leftBlack  = (digitalRead(LEFT_SENSOR_PIN)  == HIGH);
  bool rightBlack = (digitalRead(RIGHT_SENSOR_PIN) == HIGH);

  // ── Calcul de l'erreur ──────────────────────────
  // Valeurs de position :
  //   LEFT=1, RIGHT=0  →  erreur = -1  (trop à droite)
  //   LEFT=1, RIGHT=1  →  erreur =  0  (centré)
  //   LEFT=0, RIGHT=1  →  erreur = +1  (trop à gauche)
  //   LEFT=0, RIGHT=0  →  ligne perdue

  float error = 0;

  if (leftBlack && rightBlack) {
    error = 0;
    lineLost = false;
    lastDirection = 0;
  }
  else if (leftBlack && !rightBlack) {
    error = -1;
    lineLost = false;
    lastDirection = -1;
  }
  else if (!leftBlack && rightBlack) {
    error = +1;
    lineLost = false;
    lastDirection = 1;
  }
  else {
    // Ligne perdue
    if (!lineLost) {
      lineLost = true;
      lostLineTime = millis();
      integral = 0; // reset intégral
    }

    if (millis() - lostLineTime < 1000) {
      // Continue dans le dernier sens
      error = lastDirection * 1.5; // force la correction
    }
    else {
      stopMotors();
      lastError = 0;
      integral  = 0;
      delay(5);
      return;
    }
  }

  // ── Calcul du dt ────────────────────────────────
  unsigned long now = millis();
  float dt = (now - lastTime) / 1000.0; // en secondes
  if (dt <= 0 || dt > 0.5) dt = 0.005; // sécurité
  lastTime = now;

  // ── Termes PID ──────────────────────────────────
  integral  += error * dt;
  integral   = constrain(integral, -5.0, 5.0); // anti-windup

  float derivative = (error - lastError) / dt;
  lastError = error;

  float correction = Kp * error + Ki * integral + Kd * derivative;

  // ── Application aux moteurs ─────────────────────
  int speedLeft  = (int)constrain(BASE_SPEED - correction, MIN_SPEED, MAX_SPEED);
  int speedRight = (int)constrain(BASE_SPEED + correction, MIN_SPEED, MAX_SPEED);

  setMotors(speedLeft, speedRight);

  // ── Debug série ─────────────────────────────────
  Serial.print("err:"); Serial.print(error);
  Serial.print(" corr:"); Serial.print(correction);
  Serial.print(" L:"); Serial.print(speedLeft);
  Serial.print(" R:"); Serial.println(speedRight);

  delay(5);
}

//==================================================
// MOTEURS (vitesse indépendante)
//==================================================
void setMotors(int leftSpeed, int rightSpeed) {

  // Moteur gauche (canal A)
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  analogWrite(ENA_PIN, leftSpeed);

  // Moteur droit (canal B)
  digitalWrite(IN3_PIN, HIGH);
  digitalWrite(IN4_PIN, LOW);
  analogWrite(ENB_PIN, rightSpeed);
}

//==================================================
// STOP
//==================================================
void stopMotors() {
  analogWrite(ENA_PIN, 0);
  analogWrite(ENB_PIN, 0);
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, LOW);
  digitalWrite(IN4_PIN, LOW);
}
