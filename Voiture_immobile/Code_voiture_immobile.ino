#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =========================
// RELAIS (ACTIVE LOW)
// =========================
const int Relay_ondo_Bat_redro = 2;
const int Relay_ondo_Bob_redro = 3;
const int Relay_Ground         = 4;

// =========================
// MOTEUR
// =========================
const int MOTOR_IN1 = 7;
const int MOTOR_IN2 = 8;

// =========================
// FIN DE COURSE
// =========================
const int FIN_COURSE = 12;

// =========================
// PWM
// =========================
const int PWM_PIN = 9;

// =========================
// MESURE TENSION (TA VERSION)
// =========================
const int pinTensionPropre = A0;
const int pinTensionSecond = A1;

const float facteur = 4.4531835;

// =========================
// PARAMETRES
// =========================
const float HYSTERESIS = 0.5;

// =========================
// VARIABLES
// =========================
float tension_propre = 0.0;
float tension_second = 0.0;

int modeActuel = -1;

#define MODE_ONDULEUR   0
#define MODE_REDRESSEUR 1

// =========================
// PWM
// =========================
void setupPWM()
{
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);

  ICR1 = 799;
  OCR1A = 400;
}

void startPWM()
{
  OCR1A = 400;
}

void stopPWM()
{
  OCR1A = 0;
}

// =========================
// MOTEUR
// =========================
void moteurStop()
{
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
}

void moteurSens1()
{
  if (digitalRead(FIN_COURSE) == HIGH)
  {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
  }
  else moteurStop();
}

void moteurSens2()
{
  if (digitalRead(FIN_COURSE) == HIGH)
  {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
  }
  else moteurStop();
}

// =========================
// LECTURE TENSION (TA MÉTHODE)
// =========================
float lireTension(int pinAnalog)
{
  int adc = analogRead(pinAnalog);

  float tensionA0 = (adc * 5.0) / 1023.0;

  float tensionReelle = tensionA0 * facteur;

  return tensionReelle;
}

// =========================
// LCD
// =========================
void afficherLCD()
{
  lcd.setCursor(0, 0);

  if (modeActuel == MODE_ONDULEUR)
    lcd.print("ONDULEUR     ");
  else
    lcd.print("REDRESSEUR   ");

  lcd.setCursor(0, 1);

  lcd.print("P:");
  lcd.print(tension_propre, 1);

  lcd.print(" S:");
  lcd.print(tension_second, 1);
}

// =========================
// MODE ONDULEUR
// =========================
void modeOnduleur()
{
  stopPWM();

  digitalWrite(Relay_ondo_Bat_redro, LOW);
  digitalWrite(Relay_ondo_Bob_redro, HIGH);
  digitalWrite(Relay_Ground, HIGH);

  delay(100);

  startPWM();

  modeActuel = MODE_ONDULEUR;
}

// =========================
// MODE REDRESSEUR
// =========================
void modeRedresseur()
{
  stopPWM();

  digitalWrite(Relay_ondo_Bat_redro, HIGH);
  digitalWrite(Relay_ondo_Bob_redro, LOW);
  digitalWrite(Relay_Ground, LOW);

  delay(100);

  modeActuel = MODE_REDRESSEUR;
}

// =========================
// SETUP
// =========================
void setup()
{
  Serial.begin(9600);

  pinMode(PWM_PIN, OUTPUT);

  pinMode(Relay_ondo_Bat_redro, OUTPUT);
  pinMode(Relay_ondo_Bob_redro, OUTPUT);
  pinMode(Relay_Ground, OUTPUT);

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  pinMode(FIN_COURSE, INPUT_PULLUP);

  moteurStop();

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Systeme V2V");
  lcd.setCursor(0, 1);
  lcd.print("Initialisation");
  delay(2000);
  lcd.clear();

  setupPWM();
}

// =========================
// LOOP
// =========================
void loop()
{
  // =========================
  // LECTURE TENSION (TA MÉTHODE)
  // =========================
  tension_propre = lireTension(A0);
  tension_second = lireTension(A1);

  // =========================
  // SERIAL MONITOR (TA PARTIE + 2 CAPTEURS)
  // =========================
  Serial.print("P = ");
  Serial.print(tension_propre);
  Serial.print(" V | S = ");
  Serial.print(tension_second);
  Serial.println(" V");

  // =========================
  // MODE AUTO
  // =========================
  if (modeActuel == MODE_ONDULEUR)
  {
    if (tension_propre < (tension_second - HYSTERESIS))
      modeRedresseur();
  }
  else
  {
    if (tension_propre > (tension_second + HYSTERESIS))
      modeOnduleur();
  }

  // =========================
  // MOTEUR
  // =========================
  if (digitalRead(FIN_COURSE) == LOW)
  {
    moteurStop();
  }
  else
  {
    if (modeActuel == MODE_ONDULEUR)
      moteurSens2();
    else
      moteurSens1();
  }

  // =========================
  // LCD
  // =========================
  afficherLCD();

  delay(500);
}
