/* =====================================================================
   AMR — FIRMWARE COMPLET (pré-PN532)   [core 3.x]
   ---------------------------------------------------------------------
   OTA + ToF + Servos + MPU + Encodeurs filtrés + Dashboard complet

   *** ENREGISTREMENT MÉTHODE B (taps alternés) ***
   - 1er tap court : démarre, pousse la 1ère LIGNE droite
   - tap court     : fin ligne -> tourne le VIRAGE
   - tap court     : fin virage -> pousse la ligne suivante
   - ... alternance LIGNE <-> VIRAGE
   - tap LONG (2s) : termine l'enregistrement
   Rejeu : bouton START du dashboard (ou tap long quand pas en enreg)

   Librairies : VL53L0X (Pololu), ESP32Servo
   WiFi "AMR-ROBOT" / "amr12345" — Dashboard http://192.168.4.1
   OTA : Outils -> Port -> "AMR-OTA at 192.168.4.1" (mdp amr12345)
   ===================================================================== */

#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <VL53L0X.h>
#include <ESP32Servo.h>
#include "page_html.h"

/* ============== Broches ============== */
#define ENC_L_A 5
#define ENC_L_B 4
#define ENC_R_A 6
#define ENC_R_B 7
#define I2C_SDA 1
#define I2C_SCL 2
#define MPU_ADDR 0x68
#define BTN_PIN 47
#define BUZZER  21

#define L_FOR  11
#define L_BACK 10
#define R_FOR  17
#define R_BACK 18

#define XSHUT_L 12
#define XSHUT_F 13
#define XSHUT_R 14
#define SERVO_L  8
#define SERVO_R 20

#define LED_RED    36
#define LED_GREEN  38
#define LED_ORANGE 16

#define GYRO_AXIS 0x47

/* ============== Constantes ============== */
const char* AP_SSID = "AMR-ROBOT";
const char* AP_PASS = "amr12345";
const char* OTA_HOSTNAME = "AMR-OTA";
const char* OTA_PASS = "amr12345";

const int PWM_FREQ = 1000, PWM_RES = 8;
const int PWM_REJEU = 70;
const long MARGE_IMP = 18;

/* ============== Buzzer ============== */
void bip(int duree, int nombre = 1, int pause = 120) {
  for (int i = 0; i < nombre; i++) {
    digitalWrite(BUZZER, HIGH); delay(duree); digitalWrite(BUZZER, LOW);
    if (i < nombre - 1) delay(pause);
  }
}

/* ============== LEDs ============== */
volatile bool estopActif = false;
bool apOK = false;
unsigned long lastPoll = 0;   // dernier acces dashboard (LED verte)
volatile int robotCol=0, robotRow=0, robotHeadingMap=0, cellsVisitees=1;  // position carte
volatile bool enMouvement = false;   // true pendant le rejeu (pour dashboard réaliste)
volatile bool demandeNavigation = false;  // mis par /cmd?a=start, traité dans loop()
volatile bool demandeRejeu = false;        // mis par /cmd?a=replay, traité dans loop()

void setupLeds() {
  pinMode(LED_GREEN, OUTPUT);   digitalWrite(LED_GREEN, LOW);
  pinMode(LED_ORANGE,OUTPUT);   digitalWrite(LED_ORANGE,LOW);
  pinMode(LED_RED,   OUTPUT);   digitalWrite(LED_RED,  LOW);
  digitalWrite(LED_GREEN, HIGH); delay(200); digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_ORANGE,HIGH); delay(200); digitalWrite(LED_ORANGE,LOW);
  digitalWrite(LED_RED,   HIGH); delay(200); digitalWrite(LED_RED,  LOW);
}
void majLeds() {
  bool client = (millis() - lastPoll < 1500);
  digitalWrite(LED_GREEN, client ? HIGH : LOW);
  if (!apOK)         digitalWrite(LED_ORANGE, HIGH);
  else if (!client)  digitalWrite(LED_ORANGE, (millis()/400)%2);
  else               digitalWrite(LED_ORANGE, LOW);
  digitalWrite(LED_RED, estopActif ? ((millis()/200)%2) : LOW);
}

/* ============== Moteurs ============== */
void setMoteurs(int g, int d) {
  if (estopActif) { g = 0; d = 0; }   // ESTOP : coupe les moteurs
  if (g >= 0) { ledcWrite(L_BACK, g);  ledcWrite(L_FOR, 0); }
  else        { ledcWrite(L_BACK, 0);  ledcWrite(L_FOR, -g); }
  if (d >= 0) { ledcWrite(R_FOR, d);   ledcWrite(R_BACK, 0); }
  else        { ledcWrite(R_FOR, 0);   ledcWrite(R_BACK, -d); }
}
void stopMoteurs() { setMoteurs(0, 0); }

/* ============== Servos (lift) ============== */
volatile bool encGele = false;          // déclaré tôt : utilisé par les servos ET les ISR encodeurs
Servo servo1, servo2;
bool servoUp = false;

void servosUp()   {
  encGele = true;                       // gèle les encodeurs (anti-bruit servos)
  servo1.write(95); servo2.write(85);
  delay(500);                           // le temps que le pic de courant passe
  encGele = false;
  servoUp = true;  Serial.println("[SERVO] UP");
}
void servosDown() {
  encGele = true;
  servo1.write(30); servo2.write(30);
  delay(500);
  encGele = false;
  servoUp = false; Serial.println("[SERVO] DOWN");
}

void setupServos() {
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  servo1.attach(SERVO_L, 500, 2500);
  servo2.attach(SERVO_R, 500, 2500);
  servosDown();                       // position de repos
}

/* ============== Encodeurs (filtrés contre EMI) ============== */
volatile long encL = 0, encR = 0;
volatile unsigned long lastIsrL = 0, lastIsrR = 0;
const unsigned long MIN_INTERVAL_US = 800;   // <0.8 ms entre impulsions = bruit

void IRAM_ATTR isrL() {
  if (encGele) return;                        // gelé pendant servos
  unsigned long now = micros();
  if (now - lastIsrL < MIN_INTERVAL_US) return; // trop rapide = bruit
  lastIsrL = now;
  if (digitalRead(ENC_L_B)) encL++; else encL--;
}
void IRAM_ATTR isrR() {
  if (encGele) return;
  unsigned long now = micros();
  if (now - lastIsrR < MIN_INTERVAL_US) return;
  lastIsrR = now;
  if (digitalRead(ENC_R_B)) encR++; else encR--;
}
long encMoyenne() { noInterrupts(); long l=encL,r=encR; interrupts(); return (l+r)/2; }

// Remise à zéro des compteurs (utilisée au début de chaque segment / enreg)
void encReset() { noInterrupts(); encL = 0; encR = 0; interrupts(); }

/* ============== MPU ============== */
float gyroZbias = 0, heading = 0;
unsigned long tPrevImu = 0;

int16_t lireGyro() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(GYRO_AXIS); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read() << 8) | Wire.read();
}
float dernierRate = 0;        // vitesse de rotation mémorisée (pour le terme D)

// déclarations anticipées (utilisées par attendreSiEstop avant leur définition)
void lireBouton();
extern WebServer server;

void majCap() {
  unsigned long now = millis();
  float dt = (now - tPrevImu)/1000.0; tPrevImu = now;
  float rate = (lireGyro() - gyroZbias)/131.0;
  dernierRate = rate;         // mémorisé pour vitesseRotation()
  if (fabs(rate) > 0.2) heading += rate * dt;
}

/* ============== ToF VL53L0X ============== */
VL53L0X tofL, tofF, tofR;
uint16_t distL = 9999, distF = 9999, distR = 9999;
bool tofOK_L = false, tofOK_F = false, tofOK_R = false;

void setupToF() {
  // Tous éteints au départ
  pinMode(XSHUT_L, OUTPUT); digitalWrite(XSHUT_L, LOW);
  pinMode(XSHUT_F, OUTPUT); digitalWrite(XSHUT_F, LOW);
  pinMode(XSHUT_R, OUTPUT); digitalWrite(XSHUT_R, LOW);
  delay(20);

  // Gauche -> 0x30
  digitalWrite(XSHUT_L, HIGH); delay(30);
  tofL.setTimeout(200);
  if (tofL.init()) { tofL.setAddress(0x30); tofL.startContinuous(); tofOK_L = true;
                     Serial.println("[ToF] L OK -> 0x30"); }
  else Serial.println("[ToF] L INIT FAIL");

  // Avant -> 0x31
  digitalWrite(XSHUT_F, HIGH); delay(30);
  tofF.setTimeout(200);
  if (tofF.init()) { tofF.setAddress(0x31); tofF.startContinuous(); tofOK_F = true;
                     Serial.println("[ToF] F OK -> 0x31"); }
  else Serial.println("[ToF] F INIT FAIL");

  // Droite -> 0x32
  digitalWrite(XSHUT_R, HIGH); delay(30);
  tofR.setTimeout(200);
  if (tofR.init()) { tofR.setAddress(0x32); tofR.startContinuous(); tofOK_R = true;
                     Serial.println("[ToF] R OK -> 0x32"); }
  else Serial.println("[ToF] R INIT FAIL");
}

void lireToF() {
  if (tofOK_L) distL = tofL.readRangeContinuousMillimeters();
  if (tofOK_F) distF = tofF.readRangeContinuousMillimeters();
  if (tofOK_R) distR = tofR.readRangeContinuousMillimeters();
}

/* ============== Trajet (impulsions) ============== */
/* ============== Enregistrement MÉTHODE B (taps alternés) ==============
   Chaque "segment" mémorise une avance (impulsions) PUIS une rotation (deg).
   - 1er tap court  : démarre, on enregistre une LIGNE (on pousse tout droit)
   - tap court      : fin de la ligne -> on enregistre un VIRAGE (on tourne)
   - tap court      : fin du virage -> nouvelle LIGNE
   - ... alternance LIGNE <-> VIRAGE
   - tap LONG       : termine l'enregistrement complet
   ===================================================================== */
struct Segment { long impulsions; float rotation_deg; };
const int MAX_SEG = 40;
Segment trajet[MAX_SEG];
int  nbSeg = 0;
bool enReg = false;
int  phase = 0;                 // 0 = on enregistre une LIGNE, 1 = on enregistre un VIRAGE
long ligneImpulsions = 0;       // tampon de la ligne en cours
long encDebutSeg = 0; float capDebutSeg = 0;

void demarrerEnreg() {
  nbSeg = 0;
  encReset();
  encDebutSeg = encMoyenne();
  capDebutSeg = heading;
  phase = 0;                    // on commence par une ligne
  ligneImpulsions = 0;
  enReg = true;
  bip(80,1);
  Serial.println(">>> ENREG DEMARRE — pousse la 1ere LIGNE, puis tape pour le virage <<<");
}

// Appelé à chaque TAP COURT pendant l'enregistrement : termine la phase courante
void tapPhase() {
  if (phase == 0) {
    // fin d'une LIGNE -> on stocke les impulsions, on passe au virage
    ligneImpulsions = encMoyenne() - encDebutSeg;
    capDebutSeg = heading;       // référence pour mesurer le virage à venir
    phase = 1;
    bip(60,1);
    Serial.printf("   LIGNE = %ld imp -> maintenant tourne (VIRAGE)\n", ligneImpulsions);
  } else {
    // fin d'un VIRAGE -> on stocke le segment complet (ligne + rotation)
    float rot = heading - capDebutSeg;
    if (nbSeg < MAX_SEG) {
      trajet[nbSeg].impulsions  = ligneImpulsions;
      trajet[nbSeg].rotation_deg = rot;
      nbSeg++;
    }
    encReset();
    encDebutSeg = encMoyenne();
    phase = 0;                   // retour en mode ligne
    bip(60,1);
    Serial.printf("   VIRAGE = %.1f deg -> segment %d enregistre. Pousse la ligne suivante.\n", rot, nbSeg);
  }
}

void arreterEnreg() {
  // termine proprement selon la phase où on se trouve
  if (phase == 0) {
    // on finissait une ligne -> segment avec rotation 0
    long imp = encMoyenne() - encDebutSeg;
    if (nbSeg < MAX_SEG) { trajet[nbSeg].impulsions = imp; trajet[nbSeg].rotation_deg = 0; nbSeg++; }
  } else {
    // on finissait un virage -> stocke ligne précédente + ce virage
    float rot = heading - capDebutSeg;
    if (nbSeg < MAX_SEG) { trajet[nbSeg].impulsions = ligneImpulsions; trajet[nbSeg].rotation_deg = rot; nbSeg++; }
  }
  enReg = false;
  bip(80,2);
  Serial.println(">>> ENREG TERMINE — trajet : <<<");
  for (int i=0;i<nbSeg;i++)
    Serial.printf("  Seg %d : %ld imp, %.1f deg\n",i+1,trajet[i].impulsions,trajet[i].rotation_deg);
}

/* ============== Rejeu (avance avec correction ToF + gyro fallback) ============== */
void avancerImpulsions(long imp) {
  if (imp <= 0) return;
  encReset();                           // <<< reset (ignore bruit pré-segment)
  long cible = imp - MARGE_IMP; if (cible < 0) cible = 0;
  long depart = encMoyenne();
  float capCible = heading;
  const float Kp_gyro = 9.0;
  const float Kp_tof  = 0.5;          // correction par ToF (latérale)
  const uint16_t MUR_PROCHE = 250;    // mm : si distL et distR < 250 → murs détectés

  while ((encMoyenne() - depart) < cible) {
    if (estopActif) { stopMoteurs(); return; }
    majCap();
    lireToF();

    int corr;
    if (tofOK_L && tofOK_R && distL < MUR_PROCHE && distR < MUR_PROCHE) {
      // Correction par ToF : si distL > distR → robot dérive à droite → corrige vers gauche
      int delta = (int)distL - (int)distR;
      corr = (int)(Kp_tof * delta);
    } else {
      // Pas de murs latéraux → correction par gyro
      float erreur = heading - capCible;
      corr = (int)(Kp_gyro * erreur);
    }
    int g = constrain(PWM_REJEU + corr, 0, 255);
    int d = constrain(PWM_REJEU - corr, 0, 255);
    setMoteurs(g, d);
    delay(5);
    ArduinoOTA.handle();
    server.handleClient();   // garde le serveur web reactif (estop, donnees, carte)
  }
  stopMoteurs();
  delay(200);
}

void tournerAngle(float angle_deg) {
  if (fabs(angle_deg) < 3) return;
  majCap();
  float capCible = heading + angle_deg;
  if (angle_deg > 0) {
    while (heading < capCible) {
      if (estopActif) { stopMoteurs(); return; }
      majCap(); setMoteurs(-PWM_REJEU, PWM_REJEU); delay(5); ArduinoOTA.handle();
    }
  } else {
    while (heading > capCible) {
      if (estopActif) { stopMoteurs(); return; }
      majCap(); setMoteurs(PWM_REJEU, -PWM_REJEU); delay(5); ArduinoOTA.handle();
    }
  }
  stopMoteurs();
  delay(200);
}

/* =====================================================================
   NAVIGATION AUTONOME — AVANCE AU GYRO PD (ligne droite pure)
   - Va tout droit en maintenant le cap absolu (PD : P doux + D amortisseur)
   - Filet anti-collision : si un mur latéral < SEUIL_DANGER, petit écart
   - Stop quand distF <= SEUIL_DEVANT (mur devant) OU IMP_PAR_CASE atteint
   ===================================================================== */
const long     IMP_PAR_CASE  = 141;
const uint16_t SEUIL_DEVANT  = 60;     // distF <= 60 -> stop / recale sur la case
const uint16_t APPROCHE_MUR  = 150;    // distF < 150 -> commence à ralentir (plus tôt = plus sûr)
const int      PWM_APPROCHE   = 38;    // vitesse réduite près du mur (plus lent = arrêt net)
const int      PWM_FREIN      = 60;    // freinage actif : impulsion arrière
const int      FREIN_MS       = 45;    // durée du freinage actif (ms)
const uint16_t SEUIL_DANGER  = 58;     // mur latéral plus proche que ça -> on s'écarte (augmenté: évite de toucher)
const int      PWM_NAV        = 70;
const float    KP_CAP         = 3.0;   // P doux (cap)
const float    KD_CAP         = 0.8;   // D amortisseur (anti-zigzag)
const int      ECART_DANGER   = 16;    // correction si mur trop proche (renforcé)

float capCibleAbsolu = 0;

// Conservés comme valeurs neutres (le suivi de mur n'est plus utilisé,
// mais la séquence appelle encore avancerUneCase(MUR_G/MUR_D)).
enum { MUR_G, MUR_D, MUR_AUCUN };

// Si arrêt d'urgence actif : fige le robot (rouge clignote) jusqu'au réarmement.
// Permet de "reprendre depuis la case en cours" : on bloque ici, puis on continue.
void attendreSiEstop() {
  if (!estopActif) return;
  stopMoteurs();
  Serial.println("[ESTOP] Robot fige - attente rearmement...");
  while (estopActif) {
    stopMoteurs();
    digitalWrite(LED_RED, (millis()/200)%2);   // rouge clignote
    lireBouton();                              // permet de réarmer via bouton long
    server.handleClient();                     // permet de réarmer via dashboard
    ArduinoOTA.handle();
    delay(10);
  }
  Serial.println("[ESTOP] Rearme - reprise.");
  bip(60,1);
}

// le paramètre 'mur' n'est plus utilisé pour suivre, gardé pour compat d'appel
// ---------------------------------------------------------------------------
// AVANCE CONTINUE sur 'nCases' cases d'affilée (ligne droite fluide, sans stop).
// - Anti-blocage : si les encodeurs ne bougent plus pendant BLOCAGE_MS alors
//   qu'on pousse, on considère la roue calée -> on termine le segment.
// - Anti-collision renforcé : s'écarte AVANT de toucher (marge augmentée).
// - S'arrête si mur devant (distF) OU objectif d'impulsions atteint.
// ---------------------------------------------------------------------------
const unsigned long BLOCAGE_MS   = 700;    // si 0 impulsion pendant ce temps -> calé
const unsigned long TIMEOUT_CASE = 4500;   // garde-fou par case (ms)

void avancerCases(int nCases, int mur) {
  (void)mur;
  attendreSiEstop();
  encReset();
  long depart   = encMoyenne();
  long objectif = (long)nCases * IMP_PAR_CASE;

  unsigned long tDebut   = millis();
  unsigned long tDernMov = millis();
  long          encPrec  = encMoyenne();

  bool arretSurMur = false;        // pour savoir s'il faut freiner activement

  while (true) {
    if (estopActif) { stopMoteurs(); return; }
    majCap();
    lireToF();

    long parcouru = encMoyenne() - depart;

    // --- PRIORITÉ ABSOLUE : mur frontal (vérifié en premier) ---
    if (tofOK_F && distF <= SEUIL_DEVANT) { arretSurMur = true; break; }
    // fin sur distance parcourue (pas de mur atteint)
    if (parcouru >= objectif) break;

    // --- anti-blocage : détecter une roue calée ---
    long encNow = encMoyenne();
    if (labs(encNow - encPrec) >= 2) {        // ça bouge encore
      encPrec  = encNow;
      tDernMov = millis();
    } else if (millis() - tDernMov > BLOCAGE_MS) {
      Serial.println("[NAV] blocage detecte -> fin de segment");
      break;
    }
    if (millis() - tDebut > TIMEOUT_CASE * nCases) {
      Serial.println("[NAV] timeout segment");
      break;
    }

    // --- cap gyro PD (ligne droite) ---
    float erreurCap = heading - capCibleAbsolu;
    int corr = (int)(KP_CAP * erreurCap + KD_CAP * dernierRate);

    // --- anti-collision renforcé : s'écarter AVANT de toucher ---
    if (tofOK_L && distL < SEUIL_DANGER)      corr += ECART_DANGER;
    else if (tofOK_R && distR < SEUIL_DANGER) corr -= ECART_DANGER;

    // --- ralentissement à l'approche du mur frontal ---
    int pwmBase = PWM_NAV;
    if (tofOK_F && distF < APPROCHE_MUR) {
      pwmBase = map(distF, SEUIL_DEVANT, APPROCHE_MUR, PWM_APPROCHE, PWM_NAV);
      pwmBase = constrain(pwmBase, PWM_APPROCHE, PWM_NAV);
    }

    int g = constrain(pwmBase + corr, 0, 255);
    int d = constrain(pwmBase - corr, 0, 255);
    setMoteurs(g, d);
    delay(5);
    ArduinoOTA.handle();
    server.handleClient();
  }

  // --- FREINAGE ACTIF : tue la glissade pour s'arrêter net (surtout sur mur) ---
  if (arretSurMur) {
    setMoteurs(-PWM_FREIN, -PWM_FREIN);   // impulsion arrière brève
    delay(FREIN_MS);
  }
  stopMoteurs();
  delay(200);
}

// Compat : une seule case = avancerCases(1)
void avancerUneCase(int mur) { avancerCases(1, mur); }

// RECULE de 'nCases' cases (marche arrière), cap maintenu au gyro PD.
// Anti-blocage identique. Pas de capteur ToF arrière -> on se fie aux impulsions.
void reculerCases(int nCases) {
  attendreSiEstop();
  encReset();
  long depart   = encMoyenne();
  long objectif = (long)nCases * IMP_PAR_CASE;
  unsigned long tDernMov = millis();
  long encPrec = encMoyenne();

  while (true) {
    if (estopActif) { stopMoteurs(); return; }
    majCap();

    long parcouru = labs(encMoyenne() - depart);
    if (parcouru >= objectif) break;

    long encNow = encMoyenne();
    if (labs(encNow - encPrec) >= 2) { encPrec = encNow; tDernMov = millis(); }
    else if (millis() - tDernMov > BLOCAGE_MS) { Serial.println("[NAV] blocage recul"); break; }

    // cap PD, mais moteurs en arrière (valeurs négatives)
    float erreurCap = heading - capCibleAbsolu;
    int corr = (int)(KP_CAP * erreurCap + KD_CAP * dernierRate);
    int g = -constrain(PWM_NAV - corr, 0, 255);   // arrière + correction inversée
    int d = -constrain(PWM_NAV + corr, 0, 255);
    setMoteurs(g, d);
    delay(5);
    ArduinoOTA.handle();
    server.handleClient();
  }
  stopMoteurs();
  delay(200);
}

// Tourne de +90 (gauche) ou -90 (droite) en cap ABSOLU continu.
// On incrémente capCibleAbsolu de delta, puis on tourne jusqu'à l'atteindre.
// delta = +90 (gauche) ou -90 (droite).
// Virage précis : approche 2 vitesses + freinage actif. delta = +90 (gauche), -90 (droite)
const int   PWM_VIRAGE_RAPIDE = 70;   // gros du virage
const int   PWM_VIRAGE_LENT   = 32;   // approche finale (faible inertie)
const float ANGLE_LENT        = 25;   // on ralentit dans les 25 derniers degrés
const float MARGE_VIRAGE      = 3;    // on vise 90 - 3 pour compenser l'élan résiduel

void tournerRelatif(int delta) {
  // cible réduite d'une petite marge dans le sens du virage
  float signe = (delta > 0) ? 1.0 : -1.0;
  capCibleAbsolu += delta - signe * MARGE_VIRAGE;
  majCap();

  while (true) {
    if (estopActif) { stopMoteurs(); return; }
    majCap();
    float reste = fabs(heading - capCibleAbsolu);   // degrés restants
    if (reste <= 1.0) break;                         // arrivé (à 1° près)

    // vitesse selon la distance restante : rapide loin, lent près
    int pwm = (reste > ANGLE_LENT) ? PWM_VIRAGE_RAPIDE : PWM_VIRAGE_LENT;

    if (heading < capCibleAbsolu) setMoteurs(-pwm, pwm);   // gauche
    else                          setMoteurs(pwm, -pwm);   // droite
    delay(5);
    ArduinoOTA.handle();
    server.handleClient();   // garde le serveur web reactif (estop, donnees, carte)
  }

  // --- freinage actif : impulsion inverse brève pour stopper net la rotation ---
  float over = heading - capCibleAbsolu;             // de quel côté on finit
  if (fabs(over) > 0.5) {
    if (over > 0) setMoteurs(-PWM_VIRAGE_LENT, PWM_VIRAGE_LENT);  // on a trop tourné à droite -> pousse gauche
    else          setMoteurs(PWM_VIRAGE_LENT, -PWM_VIRAGE_LENT);
    delay(40);
  }
  stopMoteurs();
  delay(250);
}

// Séquence D1 -> A1. Départ orienté HAUT.
// Virages (table validée) : +90 = gauche, -90 = droite.
// Murs : GAUCHE de D1->C4, DROITE de C4->A2, GAUCHE de A2->A1.
// Met à jour la position carte (col,row) + cap carte, puis marque la case visitée.
void setCell(int col, int row, int capMap) {
  robotCol = col; robotRow = row; robotHeadingMap = capMap;
  cellsVisitees++;
}

void naviguerD1versA1() {
  if (estopActif) return;
  Serial.println(">>> NAVIGATION D1 -> A1 <<<");
  bip(80,1);
  enMouvement = true;
  servosUp();
  delay(300);

  capCibleAbsolu = 0;          // orienté haut au départ
  majCap();
  robotCol = 0; robotRow = 0; robotHeadingMap = 0; cellsVisitees = 1;

  // ===== ALLER : D1 -> A1 =====
  // ---- Zone mur GAUCHE (cap 0 = haut, +90 = gauche, -90 = droite) ----
  avancerCases(1, MUR_G);                        setCell(0,1,0);    // D1->C1
  tournerRelatif(-90);                                              // vers la droite (cap 270)
  avancerCases(3, MUR_G);                                           // C1->C2->C3->C4 EN CONTINU (ligne droite)
  setCell(1,1,270); setCell(2,1,270); setCell(3,1,270);

  // ---- Zone mur DROITE ----
  tournerRelatif(+90); avancerCases(1, MUR_D);   setCell(3,2,0);    // C4->B4
  tournerRelatif(+90); avancerCases(1, MUR_D);   setCell(2,2,90);   // B4->B3
  tournerRelatif(-90); avancerCases(1, MUR_D);   setCell(2,3,0);    // B3->A3
  tournerRelatif(+90); avancerCases(1, MUR_D);   setCell(1,3,90);   // A3->A2

  // ---- Zone mur GAUCHE ----
  tournerRelatif(+90); avancerCases(1, MUR_G);   setCell(1,2,180);  // A2->B2
  tournerRelatif(-90); avancerCases(1, MUR_G);   setCell(0,2,90);   // B2->B1
  tournerRelatif(-90); avancerCases(1, MUR_G);   setCell(0,3,0);    // B1->A1

  // ===== DÉPÔT à A1 =====
  stopMoteurs();
  servosDown();                                  // dépose le colis
  delay(400);
  bip(80,3);
  Serial.println(">>> ARRIVE A A1 - colis depose <<<");
  delay(300);

  // ===== RETOUR : A1 -> recule -> tourne droite dans B1 -> D1 =====
  Serial.println(">>> RETOUR (chemin inverse) vers D1 <<<");
  // Robot à A1 (0,3), cap 0 (haut). On RECULE d'une case -> B1 (0,2).
  reculerCases(1);                               setCell(0,2,0);    // A1 -> B1 (marche arrière)
  // Puis chemin inverse en marche avant :
  // B1->B2->A2->A3->B3->B4->C4->C3->C2->C1->D1
  // (cap : 0=haut, +90=gauche, -90/270=droite, 180=bas)

  tournerRelatif(-90); avancerCases(1, MUR_AUCUN); setCell(1,2,270); // B1->B2 (droite, cap270)
  tournerRelatif(+90); avancerCases(1, MUR_AUCUN); setCell(1,3,0);   // B2->A2 (gauche, cap0/haut)
  tournerRelatif(-90); avancerCases(1, MUR_AUCUN); setCell(2,3,270); // A2->A3 (droite, cap270)
  tournerRelatif(-90); avancerCases(1, MUR_AUCUN); setCell(2,2,180); // A3->B3 (droite, cap180/bas)
  tournerRelatif(+90); avancerCases(1, MUR_AUCUN); setCell(3,2,270); // B3->B4 (gauche, cap270)
  tournerRelatif(-90); avancerCases(1, MUR_AUCUN); setCell(3,1,180); // B4->C4 (droite, cap180/bas)
  tournerRelatif(-90); avancerCases(3, MUR_AUCUN);                   // C4->C3->C2->C1 EN CONTINU (cap90/gauche)
  setCell(2,1,90); setCell(1,1,90); setCell(0,1,90);
  tournerRelatif(+90); avancerCases(1, MUR_AUCUN); setCell(0,0,180); // C1->D1 (gauche depuis cap90 -> cap180/bas)

  stopMoteurs();
  enMouvement = false;
  bip(80,2);
  Serial.println(">>> RETOUR TERMINE - de retour a D1 <<<");
}

void rejouerTrajet() {
  if (nbSeg == 0) { Serial.println("Aucun trajet !"); bip(400,1); return; }
  Serial.println(">>> REJEU <<<");
  bip(80,1);
  // position de départ sur la carte
  robotCol = 0; robotRow = 0; robotHeadingMap = 0; cellsVisitees = 1;
  servosUp();                         // PICK UP le colis
  delay(400);
  enMouvement = true;                 // le robot roule -> dashboard animé
  for (int i = 0; i < nbSeg; i++) {
    if (estopActif) break;
    Serial.printf("  Rejeu seg %d : %ld imp, %.1f deg\n",i+1,trajet[i].impulsions,trajet[i].rotation_deg);
    avancerImpulsions(trajet[i].impulsions);
    // avance d'une case sur la carte selon le cap courant
    if (robotHeadingMap==0   && robotRow<3) robotRow++;
    else if (robotHeadingMap==90  && robotCol<3) robotCol++;
    else if (robotHeadingMap==180 && robotRow>0) robotRow--;
    else if (robotHeadingMap==270 && robotCol>0) robotCol--;
    cellsVisitees++;
    tournerAngle(trajet[i].rotation_deg);
    // met à jour le cap carte (arrondi au quart le plus proche)
    int delta = (int)round(trajet[i].rotation_deg / 90.0) * 90;
    robotHeadingMap = ((robotHeadingMap + delta) % 360 + 360) % 360;
  }
  stopMoteurs();
  enMouvement = false;                // arrêt -> dashboard à zéro
  servosDown();                       // DROP OFF le colis
  delay(400);
  bip(80,3);                          // 3 bips = fin de mission
  Serial.println(">>> REJEU TERMINE <<<");
}

/* ============== Bouton ============== */
bool btnPrec = HIGH;
unsigned long btnDownT = 0;
bool longDejaDeclenche = false;
const unsigned long SEUIL_LONG = 2000;

void lireBouton() {
  bool e = digitalRead(BTN_PIN);
  if (btnPrec == HIGH && e == LOW) { btnDownT = millis(); longDejaDeclenche = false; }

  // appui LONG = RÉARMER
  if (e == LOW && !longDejaDeclenche && (millis() - btnDownT >= SEUIL_LONG)) {
    longDejaDeclenche = true;
    while (digitalRead(BTN_PIN) == LOW) { delay(10); ArduinoOTA.handle(); }
    estopActif = false;
    Serial.println(">>> BOUTON LONG -> REARME <<<");
    bip(60,2);
  }

  // appui COURT = ARRÊT D'URGENCE
  if (btnPrec == LOW && e == HIGH) {
    unsigned long duree = millis() - btnDownT;
    if (!longDejaDeclenche && duree > 50 && duree < SEUIL_LONG) {
      estopActif = true;
      stopMoteurs();
      Serial.println(">>> BOUTON COURT -> ARRET URGENCE <<<");
    }
  }
  btnPrec = e;
}

/* ============== Dashboard léger ============== */
WebServer server(80);


// La page HTML (dashboard complet) est dans page_html.h

void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

// Position approximative du robot pour la carte (mise à jour pendant le rejeu)

void handleData() {
  lastPoll = millis();               // un client interroge -> il est connecté
  noInterrupts(); long l=encL,r=encR; interrupts();
  // gaz simulé : 0% le plus souvent, parfois 1-2%
  int gaz = 0; int rnd = random(100); if (rnd < 6) gaz = 1; else if (rnd < 9) gaz = 2;
  char buf[420];
  snprintf(buf,sizeof(buf),
    "{\"battery\":82,\"voltage\":11.7,"
    "\"heading\":%.1f,\"speed\":0,\"speed_pct\":0,\"heading_acc\":95,\"error\":0,"
    "\"distF\":%u,\"distR\":%u,\"distL\":%u,"
    "\"pwmL\":0,\"pwmR\":0,\"current\":0,"
    "\"temp\":29,\"gas\":%d,\"cpu\":30,"
    "\"eL\":%ld,\"eR\":%ld,"
    "\"cells\":%d,\"estop\":%d,\"reg\":%d,\"moving\":%d,"
    "\"robot\":{\"col\":%d,\"row\":%d,\"heading\":%d}}",
    heading, distF, distR, distL, gaz, l, r,
    cellsVisitees, estopActif?1:0, enReg?1:0, enMouvement?1:0,
    robotCol, robotRow, robotHeadingMap);
  server.send(200, "application/json", buf);
}

void handleCmd() {
  String a = server.arg("a");          // nouveau dashboard : /cmd?a=...
  if (a == "") a = server.arg("c");    // compat ancien : /cmd?c=...
  if      (a == "estop") { estopActif = true;  stopMoteurs(); Serial.println("[WEB] ESTOP"); }
  else if (a == "rearm") { estopActif = false; Serial.println("[WEB] REARM"); }
  else if (a == "liftup"  || a == "up")   servosUp();
  else if (a == "liftdown"|| a == "down") servosDown();
  else if (a == "start")  { demandeNavigation = true; Serial.println("[WEB] START demande"); }
  else if (a == "replay") { demandeRejeu = true;      Serial.println("[WEB] REPLAY demande"); }
  server.send(200, "text/plain", "OK");
}

/* ============== OTA ============== */
void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASS);
  ArduinoOTA.onStart([](){ stopMoteurs(); Serial.println("[OTA] START"); bip(50,5,50); });
  ArduinoOTA.onEnd([](){ Serial.println("[OTA] END"); bip(150,2); });
  ArduinoOTA.onError([](ota_error_t e){ Serial.printf("[OTA] ERR %u\n",e); bip(400,3); });
  ArduinoOTA.begin();
}

/* ============== SETUP / LOOP ============== */
void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\n=== BLOC 6 : OTA + ToF + Servos + Dashboard ===");

  setupLeds();
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);

  pinMode(ENC_L_A, INPUT_PULLUP); pinMode(ENC_L_B, INPUT_PULLUP);
  pinMode(ENC_R_A, INPUT_PULLUP); pinMode(ENC_R_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_L_A), isrL, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_R_A), isrR, RISING);

  ledcAttach(L_FOR, PWM_FREQ, PWM_RES);  ledcAttach(L_BACK, PWM_FREQ, PWM_RES);
  ledcAttach(R_FOR, PWM_FREQ, PWM_RES);  ledcAttach(R_BACK, PWM_FREQ, PWM_RES);
  stopMoteurs();

  setupServos();

  Wire.begin(I2C_SDA, I2C_SCL); Wire.setClock(400000);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission();
  Serial.println(">>> NE BOUGE PAS (calib gyro 2 s) <<<");
  delay(500);
  long s=0; for(int i=0;i<1000;i++){ s+=lireGyro(); delay(2);} gyroZbias=s/1000.0;
  Serial.printf("Biais gyro=%.1f\n", gyroZbias);
  tPrevImu = millis();

  setupToF();

  WiFi.mode(WIFI_AP);
  apOK = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("[WiFi] AP=%s IP=%s\n", apOK?"OK":"KO", WiFi.softAPIP().toString().c_str());

  setupOTA();
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/d", handleData);        // alias pour le nouveau dashboard
  server.on("/cmd", handleCmd);
  server.begin();
  Serial.println("[HTTP] http://192.168.4.1");

  bip(60,1);
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  majCap();
  static unsigned long tT = 0;
  if (millis()-tT > 80) { tT = millis(); lireToF(); }
  static unsigned long tL = 0;
  if (millis()-tL > 30) { tL = millis(); majLeds(); }
  lireBouton();

  // lancement de la navigation depuis le dashboard (hors handler web -> pas de réentrance)
  if (demandeNavigation) { demandeNavigation = false; naviguerD1versA1(); }
  if (demandeRejeu)      { demandeRejeu = false;      rejouerTrajet(); }

  if (Serial.available()) {
    char c = toupper(Serial.read());
    if (c=='E') { nbSeg=0; Serial.println("Trajet efface."); }
    else if (c=='S') {  // status
      Serial.printf("ToF L=%u F=%u R=%u cap=%.1f encL=%ld encR=%ld\n",
                    distL,distF,distR,heading,encL,encR);
    }
  }
}
