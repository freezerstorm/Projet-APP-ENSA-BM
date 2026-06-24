#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// =========================
// BLUETOOTH
// =========================
SoftwareSerial BT(5, 6);

// =========================
// TENSION
// =========================
const int   pinTensionPropre = A0;
const float facteur          = 4.4531835;

float tension_propre = 0;
float tension_second = 0;
bool  tensionRecue   = false;

// =========================
// HYSTÉRÈSIS MODE
// =========================
const float HYSTERESIS = 0.5;

// =========================
// PROTECTION BATTERIE
// =========================
const float VMIN      = 6.0;
const float VMIN_HYST = 0.2;
bool batterieFaible   = false;

// =========================
// RELAIS
// =========================
const int Relay_ondo_Bat_redro = 2;
const int Relay_ondo_Bob_redro = 3;
const int Relay_Ground         = 4;

// =========================
// PWM
// =========================
const int PWM_PIN = 9;

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
// MODES
// =========================
#define MODE_ONDULEUR   0
#define MODE_REDRESSEUR 1
int modeActuel = MODE_ONDULEUR;

// =========================
// MOTEUR COMMANDE
// =========================
char commandeMoteur = 'N';

// =========================
// SYSTEME ON / OFF
// =========================
bool systemeActif = true;

// =========================
// FIN DE COURSE
// =========================
char          dernierSensBloque = 'N';
unsigned long tempsDeblocage    = 0;
const unsigned long TEMPS_DEBLOCAGE = 500;

// =========================
// MACHINE D'ÉTAT RELAIS
// Remplace les delay(100) bloquants dans modeOnduleur/modeRedresseur
// =========================
enum EtatRelais { RELAIS_IDLE, RELAIS_ATTENTE };
EtatRelais    etatRelais     = RELAIS_IDLE;
unsigned long tempsRelais    = 0;
const unsigned long DELAI_RELAIS = 100;
int           modeCible      = -1; // MODE_ONDULEUR ou MODE_REDRESSEUR

// =========================
// LCD
// =========================
unsigned long dernierAffichage  = 0;
const unsigned long PERIODE_LCD = 500;
// Cache pour éviter lcd.clear() inutile
bool        dernierEtatBatFaible = false;
int         dernierMode          = -1;
bool        dernierSystemeActif  = true;

// =========================
// PWM 20 kHz Timer1
// =========================
void setupPWM()
{
  TCCR1A = _BV(COM1A1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS10);
  ICR1   = 799;
  OCR1A  = 0;
}

void startPWM() { OCR1A = 400; }
void stopPWM()  { OCR1A = 0;   }

// =========================
// MOTEUR
// =========================
void moteurStop()  { digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, LOW);  }
void moteurSens1() { digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);  }
void moteurSens2() { digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH); }

// =========================
// TENSION
// =========================
float lireTension(int pin)
{
  return ((analogRead(pin) * 5.0) / 1023.0) * facteur;
}

// =========================
// CHANGEMENT DE MODE (non-bloquant)
// Au lieu de faire delay(100) dans modeOnduleur/modeRedresseur,
// on configure les relais immédiatement puis on attend via millis()
// avant d'activer le PWM.
// =========================
void demanderMode(int cible)
{
  if (modeCible == cible && etatRelais != RELAIS_IDLE)
    return; // déjà en cours pour ce mode

  if (modeActuel == cible && etatRelais == RELAIS_IDLE)
    return; // déjà dans ce mode

  // Couper le PWM immédiatement
  stopPWM();

  modeCible = cible;

  if (cible == MODE_ONDULEUR)
  {
    digitalWrite(Relay_ondo_Bat_redro, LOW);
    digitalWrite(Relay_ondo_Bob_redro, HIGH);
    digitalWrite(Relay_Ground,         HIGH);
  }
  else
  {
    digitalWrite(Relay_ondo_Bat_redro, HIGH);
    digitalWrite(Relay_ondo_Bob_redro, LOW);
    digitalWrite(Relay_Ground,         LOW);
  }

  // Démarrer le timer non-bloquant
  etatRelais  = RELAIS_ATTENTE;
  tempsRelais = millis();
}

// Appelé dans loop() : finalise le changement de mode après DELAI_RELAIS ms
void gererTransitionRelais()
{
  if (etatRelais != RELAIS_ATTENTE)
    return;

  if (millis() - tempsRelais >= DELAI_RELAIS)
  {
    modeActuel = modeCible;
    etatRelais = RELAIS_IDLE;

    if (modeActuel == MODE_ONDULEUR && !batterieFaible)
    {
      startPWM();
    }
  }
}

// =========================
// PROTECTION BATTERIE
// =========================
void verifierBatterie()
{
  if (!batterieFaible)
  {
    if (tension_propre < VMIN)
    {
      batterieFaible = true;
      BT.println("BAT FAIBLE");
    }
  }
  else
  {
    if (tension_propre > (VMIN + VMIN_HYST))
    {
      batterieFaible = false;
      BT.println("BAT OK");
    }
  }

  // Forcer le redresseur si batterie faible
  if (batterieFaible && modeActuel == MODE_ONDULEUR)
  {
    demanderMode(MODE_REDRESSEUR);
  }
}

// =========================
// CHOIX AUTOMATIQUE DU MODE
// =========================
void miseAJourMode()
{
  if (batterieFaible)
  {
    demanderMode(MODE_REDRESSEUR);
    return;
  }

  if (tension_propre > (tension_second + HYSTERESIS))
  {
    demanderMode(MODE_ONDULEUR);
  }
  else if (tension_propre < (tension_second - HYSTERESIS))
  {
    demanderMode(MODE_REDRESSEUR);
  }
}

// =========================
// LCD – sans lcd.clear() systématique
// On ne réécrit que si quelque chose a changé
// =========================
void afficherLCD()
{
  unsigned long maintenant = millis();
  if (maintenant - dernierAffichage < PERIODE_LCD)
    return;
  dernierAffichage = maintenant;

  bool changement = (batterieFaible   != dernierEtatBatFaible) ||
                    (modeActuel        != dernierMode)          ||
                    (systemeActif      != dernierSystemeActif);

  if (!changement)
  {
    // Juste mettre à jour les valeurs numériques (ligne 1)
    if (!systemeActif)
      return;

    if (batterieFaible)
    {
      lcd.setCursor(2, 1);
      lcd.print(tension_propre, 1);
      lcd.print("V  ");
    }
    else
    {
      lcd.setCursor(2, 1);
      lcd.print(tension_propre, 1);
      lcd.print(" S:");
      lcd.print(tension_second, 1);
    }
    return;
  }

  // Changement d'état : réécrire tout l'écran
  lcd.clear();
  dernierEtatBatFaible = batterieFaible;
  dernierMode          = modeActuel;
  dernierSystemeActif  = systemeActif;

  if (!systemeActif)
  {
    lcd.setCursor(0, 0);
    lcd.print("SYSTEME ARRETE");
    return;
  }

  if (batterieFaible)
  {
    lcd.setCursor(0, 0);
    lcd.print("BAT FAIBLE");
    lcd.setCursor(0, 1);
    lcd.print("P=");
    lcd.print(tension_propre, 1);
    lcd.print("V");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print(modeActuel == MODE_ONDULEUR ? "ONDULEUR" : "REDRESSEUR");
  lcd.setCursor(0, 1);
  lcd.print("P:");
  lcd.print(tension_propre, 1);
  lcd.print(" S:");
  lcd.print(tension_second, 1);
}

// =========================
// SETUP
// =========================
void setup()
{
  Serial.begin(9600);
  BT.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Systeme V2V");
  lcd.setCursor(0, 1);
  lcd.print("Initialisation");
  // Pas de delay() ici : on attend 1500 ms avec millis()
  unsigned long t0 = millis();
  while (millis() - t0 < 1500) { /* attente non-bloquante courte au démarrage */ }
  lcd.clear();

  pinMode(PWM_PIN,              OUTPUT);
  pinMode(Relay_ondo_Bat_redro, OUTPUT);
  pinMode(Relay_ondo_Bob_redro, OUTPUT);
  pinMode(Relay_Ground,         OUTPUT);
  pinMode(MOTOR_IN1,            OUTPUT);
  pinMode(MOTOR_IN2,            OUTPUT);
  pinMode(FIN_COURSE,           INPUT_PULLUP);

  moteurStop();
  setupPWM();

  BT.println("SYSTEME PRET");
  BT.println("A=Sens1 B=Sens2 S=ON X=OFF");
}

// =========================
// LOOP – aucun delay() bloquant
// =========================
void loop()
{
  // ------------------------------------------------
  // Système arrêté
  // ------------------------------------------------
  if (!systemeActif)
  {
    moteurStop();
    stopPWM();
    afficherLCD();
    // Pas de delay() : on reste réactif au Bluetooth
    if (BT.available())
    {
      char cmd = BT.read();
      if (cmd == 'S')
      {
        systemeActif = true;
        BT.println("SYSTEME ACTIVE");
      }
    }
    return;
  }

  // ------------------------------------------------
  // Finaliser une transition relais si en cours
  // ------------------------------------------------
  gererTransitionRelais();

  // ------------------------------------------------
  // Lecture tension primaire
  // ------------------------------------------------
  tension_propre = lireTension(pinTensionPropre);

  // ------------------------------------------------
  // Lecture unique tension secondaire (Serial)
  // ------------------------------------------------
  if (!tensionRecue && Serial.available())
  {
    float v = Serial.parseFloat();
    if (v > 0 && v < 100)
    {
      tension_second = v;
      tensionRecue   = true;
    }
  }

  // ------------------------------------------------
  // Protection batterie
  // ------------------------------------------------
  verifierBatterie();

  // ------------------------------------------------
  // Mise à jour mode Onduleur / Redresseur
  // ------------------------------------------------
  if (tensionRecue)
  {
    miseAJourMode();
  }

  // ------------------------------------------------
  // Réception Bluetooth
  // ------------------------------------------------
  if (BT.available())
  {
    char cmd = BT.read();

    if (cmd == 'A')
    {
      commandeMoteur = 'A';
      if (dernierSensBloque == 'A')
        tempsDeblocage = millis();
      BT.println("MOTEUR SENS 1");
    }
    else if (cmd == 'B')
    {
      commandeMoteur = 'B';
      if (dernierSensBloque == 'B')
        tempsDeblocage = millis();
      BT.println("MOTEUR SENS 2");
    }
    else if (cmd == 'X')
    {
      systemeActif   = false;
      commandeMoteur = 'N';
      moteurStop();
      stopPWM();
      BT.println("SYSTEME ARRETE");
    }
    else if (cmd == 'S')
    {
      systemeActif = true;
      BT.println("SYSTEME ACTIVE");
    }
  }

  // ------------------------------------------------
  // Gestion fin de course + commande moteur
  // ------------------------------------------------
  bool finCourseActive = (digitalRead(FIN_COURSE) == LOW);

  if (commandeMoteur == 'A')
  {
    if (finCourseActive)
    {
      if (dernierSensBloque == 'B' &&
          millis() - tempsDeblocage < TEMPS_DEBLOCAGE)
      {
        moteurSens1(); // Sortie de butée B autorisée
      }
      else
      {
        dernierSensBloque = 'A';
        commandeMoteur    = 'N';
        moteurStop();
        BT.println("FIN COURSE - SENS 1 BLOQUE");
      }
    }
    else
    {
      moteurSens1();
    }
  }
  else if (commandeMoteur == 'B')
  {
    if (finCourseActive)
    {
      if (dernierSensBloque == 'A' &&
          millis() - tempsDeblocage < TEMPS_DEBLOCAGE)
      {
        moteurSens2(); // Sortie de butée A autorisée
      }
      else
      {
        dernierSensBloque = 'B';
        commandeMoteur    = 'N';
        moteurStop();
        BT.println("FIN COURSE - SENS 2 BLOQUE");
      }
    }
    else
    {
      moteurSens2();
    }
  }
  else
  {
    moteurStop();
  }

  // ------------------------------------------------
  // LCD
  // ------------------------------------------------
  afficherLCD();

  // Pas de delay(20) — la boucle tourne librement
}
