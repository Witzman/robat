/**
 * Steuersoftware für den Roboterbausatz RoBat
 * 
 * https://github.com/merlanura/robat
 * https://robot-workshop.de/
 * 
 * Copyright (C) 2018 Michael Ibsen
 *
 * ibsen@gmx.net
 *
 * 2019-04-12
 *
 * License: GNU GPLv3
 * http://www.gnu.de/documents/gpl.de.html
 * 
 * Dieses Programm ist freie Software. Sie können es unter den Bedingungen 
 * der GNU General Public License, wie von der Free Software Foundation 
 * veröffentlicht, weitergeben und/oder modifizieren, entweder gemäß 
 * Version 3 der Lizenz oder (nach Ihrer Option) jeder späteren Version.
 * 
 * Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, daß es 
 * Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, sogar ohne 
 * die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT FÜR 
 * EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public 
 * License.
 * 
 * Sie sollten ein Exemplar der GNU General Public License zusammen mit 
 * diesem Programm erhalten haben. Falls nicht, siehe <http://www.gnu.org/licenses/>.
 *
 */

// --- BEGIN INIT GENERAL ---

#define DEBUG false

// --- END INIT GENERAL ---


// --- BEGIN INIT AUTOMANUAL ---

// RoBat verfügt über drei verschiedene Betriebsmodi: 
// - manuelle Steuerung (MANUAL)
// - Hindernisvermeidung (AUTONOMOUS)
// - Hebocon- / Kampf (BATTLE)

#define MANUAL 0
#define AUTONOMOUS 1
#define BATTLE 2

// Standard-Modus ist MANUAL
int nMode = MANUAL;

// Zustand des Endlichen Automaten

// Im BATTLE Modus durchläuft das Programm verschiedene Zustände.
// Hier initialisieren wir die Zustandsvariablen

int nBattleState = 1; 
int nAttackCounter = 0;

// --- END INIT AUTOMANUAL ---


// --- BEGIN INIT BUMPER ---

// Mittels eines Schalters auf Pin BUMPER_PIN kann RoBat Kollisionen erkennen.
// Der Joystick-Button ist mit Pin 2 verbunden. Wenn BUMPER_PIN ebenfalls
// auf 2 gesetzt wird, wird ein Druck auf den Joystick-Button als Kollision
// interpretiert.

#define BUMPER_PIN 2

// --- END INIT BUMPER ---


// --- BEGIN INIT BUZZER ---

// Wir verwenden die TimerFreeTone Library von Tim Eckel in der Version 1.5
// https://bitbucket.org/teckel12/arduino-timer-free-tone/downloads/

#include <TimerFreeTone.h>

// Der Buzzer wird an TONE_PIN angeschlossen. Das muss ein PWM Pin sein,
// beim Arduino Nano also Pins 3, 5, 6, 9, 10 oder 11. Die RoBat-Platine
// benutzt Pin 11

#define TONE_PIN 11 

// Eine Zuordnung von Notennamen zu Frequenzen macht die Definition 
// von Melodien leichter lesbar.
// Quelle: https://de.wikipedia.org/wiki/Frequenzen_der_gleichstufigen_Stimmung

#define NOTE_C3 131
#define NOTE_D3 147
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_G3 196
#define NOTE_A3 220
#define NOTE_H3 247

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_H4 494

// Notenlängen (in ms)
#define GANZE 1000
#define HALBE 500
#define VIERTEL 250
#define ACHTEL 125

#define NOTE_PAUSE 0

// Einstellen einer Melodie
// Das Array melody[] enthält die Notenwerte, das gleich große Array
// duration[] die zugehörigen Notenlängen.
// Die Länge der Melodie wird durch melody_length festgelegt.

#define melody_length 2
int melody[]   = { NOTE_C4, NOTE_A4 };
int duration[] = { VIERTEL, ACHTEL  };

/*

// Eine andere Melodie

#define melody_length 8
int melody[]   = { NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_PAUSE, NOTE_H3, NOTE_C4 };
int duration[] = { VIERTEL, ACHTEL,  ACHTEL,  VIERTEL, VIERTEL, VIERTEL,    VIERTEL, VIERTEL };

*/

// --- END INIT BUZZER ---


// --- BEGIN INIT SERVO ---

// Die Servo Bibliothek von Michael Margolis ist standardmäßig installiert.
// Dieser Sketch setzt auf v1.1.3 der Servo Bibliothek auf.

#include <Servo.h>

// servos on A0 (D14) and A1 (D15)

// RoBat verfügt über zwei Servos. Servo 2 wird zum Drehen des 
// Ultraschallsensors verwendet. Servo 1 ist frei verwendbar und wird
// in diesem Sketch langsam hin- und her bewegt.

// Für jeden Servo wird die minimale und maximale Position angegeben.
// Die minimale Position sollte theoretisch 0, die maximale 180 Grad sein. 
// In der Praxis werden diese Werte oft nicht erreicht. Sie müssen 
// individuell bestimmt werden. 

#define SERVO_1_PIN 13
#define SERVO_1_MIN 10   // minimale Position 0-180
#define SERVO_1_MAX 170  // maximale Position 0-180

#define SERVO_2_PIN 12
#define SERVO_2_MIN 25   // minimale Position 0-180
#define SERVO_2_MAX 180  // maximale Position 0-180

// Der Servo benötigt eine gewisse Zeit für die Drehung. DETACH_DELAY gibt
// diese Zeit in ms an.

#define DETACH_DELAY_SERVO_1 300 
#define DETACH_DELAY_SERVO_2 150 

// Servo Objekte erstellen

Servo servo1;
Servo servo2;

bool bAttachedServo1 = false;
bool bAttachedServo2 = false;

// Das Bewegen eines Servos dauert einige Zeit. Der Programmablauf soll
// nicht durch das Warten auf das Ende der Bewegung ausgebremst werden.
// Daher führen wir die Bewegung in kleinen Teilen aus. Wir merken uns 
// die Systemzeit bei Beginn der Bewegung und jeweils bei der Ausführung
// einer Teilbewegung. In der Loop prüfen wir, ob der Zeitpunkt für die
// nächste Bewegung gekommen ist und führen die nächste Teilbewegung 
// (nur) dann aus.

unsigned long timeOfLastChangeServo1 = 0;
unsigned long timeOfLastChangeServo2 = 0;

// Für die Bewegung erhält ein Servo eine Zielposition. Während der
// Teilbewegungen besitzt er jeweils eine aktuelle Position. Durch 
// Vergleich der beiden kann entschieden werden, ob eine weitere 
// Bewegung notwendig ist.

int actualPositionServo1 = 0;
int targetPositionServo1 = 0;
int actualPositionServo2 = 0;
int targetPositionServo2 = 0;

// --- END INIT SERVO ---


// --- BEGIN INIT ULTRASONIC ---

// Zur Entfernungsmessung wird ein Ultraschallsensor HC-SR04 eingesetzt.
// Der Messbereich liegt theoretisch zwischen 1cm und 400cm, in der Praxis
// sind Werte bis 200cm realistisch. 

// Wir verwenden die Ultrasonic Library von Erick Simões in der Version 3.0.0
// https://github.com/ErickSimoes/Ultrasonic

// Hinweis:
// Beim Compilieren kann es zu einer Fehlermeldung kommen:
// "as: unrecognized option '-mmcu=avr5'"
// Der Fehler lässt sich durch Verwendung der Version 1.6.9 des 
// Boardverwalters umgehen. 

#include <Ultrasonic.h>

// Pins für den Ultraschallsensor HC-SR04

#define TRIGGER_PIN  15  // Trigger Signal an den Ultraschallsensor
#define ECHO_PIN     14  // Echo Antwort vom Ultraschallsensor
#define MAX_DISTANCE 100 // Begrenzung der max. Distanz auf 100cm

unsigned long timeOfLastDistanceMeasurement = millis();

// Ultrasonic Objekt erzeugen

Ultrasonic ultrasonic(TRIGGER_PIN, ECHO_PIN);
// ultrasonic.setTimeout(10000UL); // max. 40000UL

// --- END INIT ULTRASONIC ---


// --- BEGIN INIT WS2811 ---

// Für die Ansteuerung der LEDs (P9812 bzw. WS8212) verwenden wir die 
// FastLED Library von Daniel Garcia in der Version 3.2.6
// https://github.com/FastLED/FastLED

#include <FastLED.h>

// Anzahl der LEDs. RoBat verwendet standardmäßig nur zwei, aber mit 
// dieser Angabe können bis zu 12 hintereinandergeschaltet werden.
#define NUM_LEDS 12

// Array zur Speicherung der Farben aller LEDs
CRGB leds[NUM_LEDS];

// Die LEDs werden auf der RoBat Platine über Pin 10 angesteuert
#define WS2811_PIN 10

// --- END INIT WS2811 ---


// --- BEGIN INIT MOTOR ---

// Auf dem Motortreiberboard sind die Eingänge für Motor A mit in1 und in2
// markiert und die Eingänge für Motor B mit in3 und in4. 

// Motor A
#define MOTOR_A_ENABLE_PIN 6
#define MOTOR_A_IN1_PIN 7
#define MOTOR_A_IN2_PIN 8

// Motor B
#define MOTOR_B_ENABLE_PIN 3
#define MOTOR_B_IN3_PIN 5
#define MOTOR_B_IN4_PIN 4

#define MOTOR_STANDBY_PIN 9


// Geschwindigkeiten der beiden Motoren 
int nMotorSpeed1 = 0;
int nMotorSpeed2 = 0;

// Vorherige Motorgeschwindigkeiten
int nPrevMotorSpeed1 = 0;
int nPrevMotorSpeed2 = 0;

// Maximale Geschwindigkeit der beiden Motoren
// MAX_SPEED 0..255
#define MAX_SPEED 196

// Motor A / B (linker Motor / rechter Motor)
#define MOTOR_A 0
#define MOTOR_B 1

// Richtungen vorwärts und rückwärts eines Motors
#define FORWARD 0
#define BACKWARD 1
#define STOP 2

// Richtung, in die der Roboter sich dreht
#define LEFT 0
#define RIGHT 1

// --- END INIT MOTOR ---


// --- BEGIN INIT JOYSTICK ---

// Die Stellung des Joysticks wird über die Pins A6 und A7 eingelesen.
#define JOY_VERTICAL_PIN A6
#define JOY_HORIZONTAL_PIN A7

// Joystickposition (0..1023). Die Mittelstellung hat den Wert 512.
int nJoyPosV = 512;
int nJoyPosH = 512;

// Werte zwischen JOY_MIDDLE_MIN und JOY_MIDDLE_MAX werden als 
// Mittelstellung interpretiert
#define JOY_MIDDLE_MIN 472
#define JOY_MIDDLE_MAX 552

// --- END INIT JOYSTICK ---


// --- BEGIN FUNCTIONS ULTRASONIC ---

/**
 * Ermittelt die Distanz vom Roboter zum nächsten Hindernis mit dem
 * Ultraschallsensor. 
 * Die Entfernung wird dabei mehrfach gemessen, um Fehler zu minimieren.
 * Negative Entfernungen werden ignoriert (Messfehler).
 * 
 * Gibt den Durchschnittswert der Messungen zurück.
 *
 * @param int nMeasurements: Anzahl der Messungen, Default 3
 * @return int nDistanceAvg: Entfernung zum nächsten Objekt
 *
 */
int getDistance(int nMeasurements = 3) {
    int nDistanceAvg = 0;

    int nDistance = 0;
    int nDistanceSum = 0;
    int nValidValues = 0; // Anzahl gültiger (positiver) Messwerte

    for (int i = 0; i <= nMeasurements; i++) {
        nDistance = ultrasonic.read();

        // Hinweis: in früheren Versionen der Ultrasonic Bibliothek wurde
        // distanceRead() verwendet:
        // nDistance = ultrasonic.distanceRead(CM);

        if (nDistance > 0) {
          nDistanceSum += nDistance;
          nValidValues++;
        }
    }

    if (nValidValues > 0) {
        nDistanceAvg = floor(nDistanceSum / nValidValues);
    }

    return nDistanceAvg;
}

// --- END FUNCTIONS ULTRASONIC ---


// --- BEGIN FUNCTIONS LED ---

/**
 * Zeigt die Entfernung zum nächsten Objekt mit den LEDs 0 und 1 durch
 * unterschiedliche Farben an.
 * 
 * 150cm - oo: schwarz 
 *  51cm - 150cm: grün
 *  31cm -  50cm: blau
 *  16cm -  30cm: türkis
 *   0cm -  15cm: rot
 *
 * @param int nDistance distance to object in this direction
 *
 */
void showDistance(int nDistance) {

    CRGB color = CRGB::Black;
    if (nDistance > 150) {
      color = CRGB::Black;
    }
    else if ((nDistance <= 150) && (nDistance > 50)) {
      color = CRGB::Red; // wird grün angezeigt
    }
    else if ((nDistance <= 50) && (nDistance > 30)) {
      color = CRGB::Blue;
    }
    else if ((nDistance <= 30) && (nDistance > 15)) {
      color = CRGB::DarkTurquoise;
    }
    else if (nDistance > 0) {
      color = CRGB::Green; // wird rot angezeigt
    }
    else {
      color = CRGB::Black;
    }

    // HINT: Diese Funktion ist zeitkritisch. Wenn mehr als 2 LEDs 
    // (bzw. deren Array-Elemente) gesetzt werden, laufen die Motoren 
    // nicht mehr. 

    leds[0] = color;
    leds[1] = color;
    
    FastLED.show();
}

// --- END FUNCTIONS LED ---


// --- BEGIN FUNCTIONS SERVO ---

/**
 * Stellt die Zielposition für Servo 1 ein und schaltet den Servomotor ein.
 * 
 */
void startServo1(int targetPos) {
    targetPositionServo1 = targetPos;

    // starte Servo
    servo1.attach(SERVO_1_PIN);
    bAttachedServo1 = true; // merken, dass der Servo eingeschaltet ist

    // Setzt die Servo-Position (0..180)
    // Dabei werden die minimale und maximale Position berücksichtigt.
    int targetPosCorrected = map(targetPositionServo1, 0, 180, SERVO_1_MIN, SERVO_1_MAX);
    servo1.write(targetPosCorrected);
        
    if (DEBUG) {
      Serial.print("actualPositionServo1: ");
      Serial.println(actualPositionServo1);
      Serial.print("targetPositionServo1: ");
      Serial.println(targetPositionServo1);
    }

    actualPositionServo1 = -1;
}


/**
 * Stellt die Zielposition für Servo 2 ein und schaltet den Servomotor ein.
 * 
 */
void startServo2(int targetPos) {
    targetPositionServo2 = targetPos;

    // starte Servo
    servo2.attach(SERVO_2_PIN);
    bAttachedServo2 = true;

    // Setzt die Servo-Position (0..180)
    // Dabei werden die minimale und maximale Position berücksichtigt.
    int targetPosCorrected = map(targetPositionServo2, 0, 180, SERVO_2_MIN, SERVO_2_MAX);
    servo2.write(targetPosCorrected);

    if (DEBUG) {
      Serial.print("actualPositionServo2: ");
      Serial.println(actualPositionServo2);
      Serial.print("targetPositionServo2: ");
      Serial.println(targetPositionServo2);
      Serial.print("targetPos (corrected): ");
      Serial.println(targetPosCorrected);
    }

    actualPositionServo2 = -1;
}


/**
 * Hält Servo 1 an.
 * 
 */
void stopServo1() {
   servo1.detach();
   bAttachedServo1 = false;
   actualPositionServo1 = targetPositionServo1;

   if (DEBUG) {
       Serial.println("stopping servo 1");
   }
}


/**
 * Hält Servo 2 an.
 * 
 */
void stopServo2() {
   servo2.detach();
   bAttachedServo2 = false;
   actualPositionServo2 = targetPositionServo2;

   if (DEBUG) {
       Serial.println("stopping servo 2");
   }
}


/**
 * Bewegt Servo 1 hin und her. Dabei werden nacheinander mehrere 
 * Positionen angesteuert.
 * 
 */
void moveServoBackForth() {

    unsigned long timeNow = millis();

    // Positionen sind 60, 90 und 130 Grad

    if (0 == actualPositionServo1) {
        targetPositionServo1 = 60;
        startServo1(targetPositionServo1);
        timeOfLastChangeServo1 = timeNow;
    }
    else if (60 == actualPositionServo1) {
        targetPositionServo1 = 90;
        startServo1(targetPositionServo1);
        timeOfLastChangeServo1 = timeNow;
    }
    else if (90 == actualPositionServo1) {
        targetPositionServo1 = 130;
        startServo1(targetPositionServo1);
        timeOfLastChangeServo1 = timeNow;
    }
    else if (130 == actualPositionServo1) {
        targetPositionServo1 = 60;
        startServo1(targetPositionServo1);
        timeOfLastChangeServo1 = timeNow;
    }

    // Servo anhalten, wenn die Position erreicht ist bzw. die Zeit für die 
    // Erreichung der Position abgelaufen ist.
    if (bAttachedServo1 && (timeNow - timeOfLastChangeServo1) > DETACH_DELAY_SERVO_1) {
        if (DEBUG) {
            Serial.print("servo time:");
            Serial.println(timeNow - timeOfLastChangeServo1);
        }

        stopServo1();

        actualPositionServo1 = targetPositionServo1;
    }
}

// --- BEGIN FUNCTIONS SERVO ---


// --- BEGIN FUNCTIONS MOTOR ---

/**
 * Setzt die Drehrichtung der Motoren ohne die Geschwindigkeit zu ändern.
 * 
 * @param int nMotor: 0 - motor A, 1 - motor B
 * @param int nDir: 0 - forward, 1 - backward
 */
void setMotorDir(int nMotor, int nDir) {
    if (MOTOR_A == nMotor) {
        if (FORWARD == nDir) {
            // Motor A vorwärts
            digitalWrite(MOTOR_A_IN1_PIN, HIGH);
            digitalWrite(MOTOR_A_IN2_PIN, LOW);
        }
        else if (BACKWARD == nDir) {
            // Motor A rückwärts
            digitalWrite(MOTOR_A_IN1_PIN, LOW);
            digitalWrite(MOTOR_A_IN2_PIN, HIGH);
        }
    }
    else {
        if (FORWARD == nDir) {
            // Motor B vorwärts
            digitalWrite(MOTOR_B_IN3_PIN, HIGH);
            digitalWrite(MOTOR_B_IN4_PIN, LOW);
        }
        else if (BACKWARD == nDir) {
            // Motor B rückwärts
            digitalWrite(MOTOR_B_IN3_PIN, LOW);
            digitalWrite(MOTOR_B_IN4_PIN, HIGH);
        }
    }
}
  

/**
 * Einen der beiden Motoren einschalten. 
 *
 * @param int nMotor: Auswahl des Motors, 0 - Motor A, 1 - Motor B
 * @param int nDir: Drehrichtung, 0 - vorwärts, 1 - rückwärts
 * @param int nSpeed: Geschwindigkeit (0..255), wird durch MAX_SPEED begrenzt
 *
 */
void startMotor(int nMotor, int nDir, int nSpeed) {

    if (DEBUG) {
        Serial.print(millis());
        Serial.print(", ");
        Serial.print("Starting motor ");
        Serial.print(nMotor);
        Serial.print(", dir: ");
        Serial.print(nDir);
        Serial.print(", speed: ");
        Serial.println(nSpeed);
    }

    // Geschwindigkeit auf 0 .. MAX_SPEED begrenzen
    nSpeed = max(0, min(nSpeed, MAX_SPEED));

    setMotorDir(nMotor, nDir);

    if (MOTOR_A == nMotor) {
        analogWrite(MOTOR_A_ENABLE_PIN, nSpeed);
    }
    else {
        analogWrite(MOTOR_B_ENABLE_PIN, nSpeed);
    }   
}


/**
 * Einen der Motoren anhalten
 *
 * @param int nMotor: Auswahl des Motors, 0 - Motor A, 1 - Motor B
 */
void stopMotor(int nMotor) {

    if (DEBUG) {
        Serial.print(millis());
        Serial.print(", ");
        Serial.print("stopping motor ");
        Serial.println(nMotor);
    }

    if (MOTOR_A == nMotor) {
        digitalWrite(MOTOR_A_IN1_PIN, LOW);
        digitalWrite(MOTOR_A_IN2_PIN, LOW);
        
        // HIGH, um den Motor auszuschalten, 0 zum Bremsen
        analogWrite(MOTOR_A_ENABLE_PIN, 0); 
    }
    else {
        digitalWrite(MOTOR_B_IN3_PIN, LOW);
        digitalWrite(MOTOR_B_IN4_PIN, LOW);
        
        // HIGH, um den Motor auszuschalten, 0 zum Bremsen
        analogWrite(MOTOR_B_ENABLE_PIN, 0); 
    }
}


/**
 * Beide Motoren anhalten
 *
 * @param nDir int Richtung, FORWARD | BACKWARD
 */
void stopMotors(int nDir) {

    startMotors(nDir, 0, nDir, 0);

    digitalWrite(MOTOR_A_IN1_PIN, LOW);
    digitalWrite(MOTOR_A_IN2_PIN, LOW);
    
    // HIGH, um den Motor auszuschalten, 0 zum Bremsen
    analogWrite(MOTOR_A_ENABLE_PIN, 0);

    digitalWrite(MOTOR_B_IN3_PIN, LOW);
    digitalWrite(MOTOR_B_IN4_PIN, LOW);
    
    // HIGH, um den Motor auszuschalten, 0 zum Bremsen
    analogWrite(MOTOR_B_ENABLE_PIN, 0); 

    nPrevMotorSpeed1 = 0;
    nPrevMotorSpeed2 = 0;
}

/**
 * Setzt die Motorgeschwindigkeiten. Dabei wird die Zielgeschwindigkeit 
 * langsam angesteuert, um den Einschaltstrom zu minimieren.
 * 
 * @param int Richtung Motor A
 * @param int Geschwindigkeit Motor A
 * @param int Richtung Motor B
 * @param int Geschwindigkeit Motor B
 * 
 * Die Funktion benutzt die globalen Variablen nPrevMotorSpeed1 und nPrevMotorSpeed2
 */
void startMotors(int nDirA, int nSpeedA, int nDirB, int nSpeedB){
  
    // Drehrichtungen setzen
    setMotorDir(MOTOR_A, nDirA);
    setMotorDir(MOTOR_B, nDirB);

    // MotorSpeed1 und MotorSpeed2 sind die Zielgeschwindigkeiten für
    // jeden Motor.

    // Die Geschwindigkeit wird mit einer Verzögerung erreicht, um den
    // Einschaltstrom gering zu halten. Ohne diese Maßnahme kann der 
    // Einschaltstrom so groß werden, dass die Batterien überlastet 
    // werden und die Spannung zusammenbricht. In der Folge startet
    // der Arduino neu.
      
    // nPrevMotorSpeed1 und nPrevMotorSpeed2 speichern die vorherige Geschwindigkeit
    // der beiden Motoren. Diese kann kleiner, größer oder gleich der 
    // gewünschten Zielgeschwindigkeit sein.

    // nDelay hängt von der Differenz zwischen aktueller und Zielgeschwindigkeit
    // ab.
    
    int nDelay = 0;
    int nDiffA = abs(nSpeedA - nPrevMotorSpeed1);
    int nDiffB = abs(nSpeedB - nPrevMotorSpeed2);
    if ((nDiffA > 50) || (nDiffB > 50)) {
        nDelay = 1;
    }
    else if ((nDiffA > 120) || (nDiffB > 120)) {
        nDelay = 2;
    }
    else if ((nDiffA > 180) || (nDiffB > 180)) {
        nDelay = 3;
    }
    else {
        nDelay = 4;
    }

    int nCurMotorSpeed1 = nPrevMotorSpeed1;
    int nCurMotorSpeed2 = nPrevMotorSpeed2;

    while ((nCurMotorSpeed1 != nSpeedA) || (nCurMotorSpeed2 != nSpeedB)) {
        if (nCurMotorSpeed1 > nSpeedA) {
            nCurMotorSpeed1--;
        }
        if (nCurMotorSpeed1 < nSpeedA) {
            nCurMotorSpeed1++;
        }
        if (nCurMotorSpeed2 > nSpeedB) {
            nCurMotorSpeed2--;
        }
        if (nCurMotorSpeed2 < nSpeedB) {
            nCurMotorSpeed2++;
        }
        analogWrite(MOTOR_A_ENABLE_PIN, nCurMotorSpeed1);
        analogWrite(MOTOR_B_ENABLE_PIN, nCurMotorSpeed2);

        // warten
        delay(nDelay);
    }

    delay(20);

    nPrevMotorSpeed1 = nCurMotorSpeed1;
    nPrevMotorSpeed2 = nCurMotorSpeed2;
}


/**
 * Dreht den Roboter nach links oder rechts, indem ein Motor für nDelay 
 * Millisekunden vorwärts und der andere rückwärts gedreht wird.
 *
 * @param int nDir: Drehrichtung, 0 - links / 1 - rechts
 * @param int nDelay: Dauer der Drehung in ms
 *
 */
void turnRobot(int nDir, int nDelay) {
    stopMotors(FORWARD);
    delay(100);

    int nSpeed = 128;

    if (LEFT == nDir) {
        startMotors(FORWARD, nSpeed, BACKWARD, nSpeed);
    }
    else {
        startMotors(BACKWARD, nSpeed, FORWARD, nSpeed);
    }

    delay(nDelay); 
    stopMotors(FORWARD);
}

// --- END FUNCTIONS MOTOR ---


// --- BEGIN FUNCTIONS DO_BATTLE ---

// Diese Funktionen sind abhängig von den Abschnitten MOTOR, SERVO und ULTRASONIC

/**
 * Misst den Abstand zum nächsten Objekt in Richtung nAngle
 *
 * @param int nAngle: Winkel
 * @return int nDistance: Abstand zum Objekt in cm
 *
 */
int getDistanceDir(int nAngle) {

    // Servo auf den Winkel ausrichten
    startServo2(nAngle);
    delay(DETACH_DELAY_SERVO_2);
    stopServo2();

    // Abstand messen
    int nDistance = getDistance();

    return nDistance;
}


/**
 * Misst den Abstand zum nächsten Objekt in verschiedenen Richtungen. 
 * Liefert die Richtung (Winkel) des nächsten Objekts zurück.
 * 
 * Falls es mehrere nächste Objekte gibt, wird der kleinere Winkel 
 * zurückgegeben.
 *
 * Falls kein Objekt weniger als nMaxDistance cm entfernt ist, wird ein 
 * Winkel von 90 Grad zurückgeliefert.
 * 
 * @param int nMaxDistance: max. Entfernung eines Objekts, Default 100cm
 * @return int nAngle: Winkel zum nächsten Objekt oder 90 Grad, falls kein Objekt in der Nähe ist.
 *
 */
int getDirectionOfNearestObject(int nMaxDistance = 100) {
    int nMinAngle = 30; // 20
    int nMaxAngle = 150; // 160
    int nNearestObjectAngle = 90; // default
    int nNearestObjectDistance = 999;
    int nDistance = 0;

    // mehrer Winkel durchlaufen und jeweils den Abstand messen
    for (int nAngle = nMinAngle; nAngle < nMaxAngle; nAngle += 30) { // 15
        startServo2(nAngle);
        delay(DETACH_DELAY_SERVO_2);
        stopServo2();
        nDistance = getDistance();

        if (nDistance < nNearestObjectDistance) {
            nNearestObjectDistance = nDistance;
            nNearestObjectAngle = nAngle;
        }
    }

    return nNearestObjectAngle;
}


/**
 * Die Funktion sucht nach Objekten in der Nähe und greift sie an. 
 * 
 * Die Funktion durchläuft verschiedene Zustände ("Endlicher Automat"). 
 * Jeder Zustand ist mit einer Handlung verbunden.
 * Der Übergang in einen anderen Zustand wird über interne und externe
 * Bedingungen bestimmt.
 * 
 */
void doBattle() {
    // Standardeinstellungen
    int nDistance = getDistance(); 
    int nAngleOfOpponent = 90;
    int nSpeed = 0;

    // Entfernung zum nächsten Objekt mit LEDs anzeigen
    showDistance(nDistance);

    // Zustand setzen

    // Entfernung zum Ziel < 30cm? Dann Attacke
    // sonst: links und rechts gucken und Ziel suchen.
    // In Richtung auf das nächste Ziel drehen
    // langsame Fahrt voraus

    // nBattleState:
    // 0 - lauern, Gegner suchen, Robat auf Gegner ausrichten
    // 1 - Angriff geradeaus
    // 2 - Angriff mit Schlenker links
    // 3 - Angriff mit Schlenker rechts
    // 4 - Zurückweichen
    // 5 - Suchfahrt geradeaus starten
    // 6 - Stop
    // 7 - Suchfahrt geradeaus

    switch (nBattleState) {
        case 0:
            // lauern, Gegner suchen, Robat auf Gegner ausrichten
            
            // Abstand in verschiedene Richtungen messen und zum nächsten 
            // Objekt drehen
            nAngleOfOpponent = getDirectionOfNearestObject();
            nDistance = getDistanceDir(nAngleOfOpponent);

            // Servo zurück auf 90 Grad (geradeaus)
            startServo2(90);
            delay(DETACH_DELAY_SERVO_2);
            stopServo2();

            if (nAngleOfOpponent < 90) {
                // drehe den Roboter nach links
                turnRobot(LEFT, 4 * (90 - nAngleOfOpponent));
            }
            else if (nAngleOfOpponent > 90) {
                // drehe den Roboter nach rechts
                turnRobot(RIGHT, 4 * (nAngleOfOpponent - 90));
            }
            else {
                // nächstes Objekt liegt recht voraus, Roboter nicht drehen
            }

            if (nDistance < 20) {
                // Angriff
                nBattleState = 1;
            }
            else {
                // Suchfahrt geradeaus
                nBattleState = 5;
            }
            break;
        case 1:
            // Angriff geradeaus
            
            nAttackCounter++; // Anzahl durchgeführter Angriffe
      
            nSpeed = MAX_SPEED; // volle Kraft voraus
            startMotors(FORWARD, nSpeed, FORWARD, nSpeed);
            delay(1000);
      
            stopMotors(FORWARD);

            if (1 == nAttackCounter) {
                // beim ersten Angriff nur vorwärts fahren
                nBattleState = 0; // lauern
            }
            else {
                nBattleState = 4; // rückwärts fahren
            }

            break;

        case 2:
            // Angriff mit Schlenker links
            // noch nicht implementiert
            break;

        case 3:
            // Angriff mit Schlenker rechts
            // noch nicht implementiert
            break;

        case 4:
            // Zurückweichen

            nSpeed = 100;
            startMotors(BACKWARD, nSpeed, BACKWARD, nSpeed);
            delay(800);
            
            stopMotors(BACKWARD);

            nBattleState = 0;
            break;

        case 5:
            // Suchfahrt geradeaus starten

            // Beginn der Suchfahrt merken
            timeOfLastDistanceMeasurement = millis();
            
            nSpeed = 80;
            startMotors(FORWARD, nSpeed, FORWARD, nSpeed);

            nBattleState = 7;
            break;

        case 6:
            // Stop
            stopMotors(FORWARD);

            break;

        case 7:
            // Suchfahrt geradeaus

            nDistance = getDistanceDir(90);

            if (nDistance < 20) {
                // Angriff
                nBattleState = 1;
            }

            // Zustandsübergang, wenn "lange" kein Gegner gesehen wurde
            if (millis() - timeOfLastDistanceMeasurement > 1000) {
                // anhalten und erneut nach Gegnern suchen
                stopMotors(FORWARD);

                nBattleState = 0;
            }
            break;
    }
}

// --- END FUNCTIONS DO_BATTLE ---


// --- BEGIN FUNCTIONS AVOID_OBSTACLES ---

// Diese Funktionen sind abhängig von den Abschnitten MOTOR, SERVO und ULTRASONIC

/**
 * Die Funktion versucht, den Roboter so zu steuern, dass er nicht 
 * gegen Hindernisse prallt.
 * 
 */
void avoidObstacles() {

    int nDistance = getDistance(); 

    if (DEBUG) {
        // Serial.print("Distance to object: ");
        Serial.println(nDistance);
    }

    showDistance(nDistance);

    int nSpeed = 0;
    if (digitalRead(BUMPER_PIN) == LOW) {
        // Zusammenstoß mit Hindernis erkannt. Ausweichen

        stopMotors(FORWARD);

        delay(250);
        startMotors(BACKWARD, 128, BACKWARD, 128); 
        delay(750);
        stopMotor(MOTOR_A);
        delay(250);
        stopMotor(MOTOR_B);
    }
    else if (nDistance > 10) {
        // ausreichender Abstand zum nächsten Objekt
        // geradeaus fahren
        nSpeed = map(nDistance, 15, 150, 80, 128);
        startMotors(FORWARD, nSpeed, FORWARD, nSpeed);
    }
    else {
        // Kollisionsgefahr. Anhalten
        stopMotors(FORWARD);

        // Zustand durch Töne signalisieren
        TimerFreeTone(TONE_PIN, NOTE_A4, 80);
        TimerFreeTone(TONE_PIN, NOTE_PAUSE, 80);
        TimerFreeTone(TONE_PIN, NOTE_A3, 200);

        // Abstand in verschiedene Richtungen messen und in eine freie
        // Richtung drehen.
        
        // links frei?
        startServo2(60);
        delay(DETACH_DELAY_SERVO_2);
        stopServo2();
        nDistance = getDistance();

        if (nDistance > 15) {
            // nach links drehen
            turnRobot(LEFT, 200);
        }
        else {
            // rechts frei?
            startServo2(120);
            delay(DETACH_DELAY_SERVO_2);
            stopServo2();
            nDistance = getDistance();

            if (nDistance > 15) {
                // nach rechts drehen
                turnRobot(RIGHT, 200);
            }
            else {
                // umdrehen
                turnRobot(RIGHT, 1000);
            }
        }

        // Servo mit Ultraschallsensor wieder nach vorne ausrichten
        startServo2(90);
        delay(DETACH_DELAY_SERVO_2);
        stopServo2();
    }
}

// --- END FUNCTIONS AVOID_OBSTACLES ---


// --- BEGIN FUNCTIONS MANUAL_CONTROL ---

// Diese Funktionen sind abhängig von den Abschnitten MOTOR und JOYSTICK

/**
 * Manuelle Steuerung des RoBat, über die Joystick-Positionen wird die
 * Geschwindigkeit und Drehrichtung der Motoren gesteuert.
 * 
 */
void manualControl() {

    // Joystick Position einlesen

    nJoyPosV = analogRead(JOY_VERTICAL_PIN);
    nJoyPosH = analogRead(JOY_HORIZONTAL_PIN);

    // Der Joystick liefert Werte zwischen 0 und 1023 in X- und Y-Richtung.
    // Wenn der Joystick in der Mittelstellung ist, müsste er theoretisch
    // einen Wert von 512 für beide Richtungen liefern. In der Praxis
    // weichen die Werte davon ab. Wir definieren deshalb alle Werte
    // zwischen JOY_MIDDLE_MIN und JOY_MIDDLE_MAX als Mittelstellung.

    int nMotorDir = STOP; // default

    // Wird der Joystick nach hinten gedrückt (nJoyPosV < JOY_MIDDLE_MIN),
    // soll sich der Roboter rückwärts bewegen. WIrd der Joystick nach
    // vorne gedrückt (nJoyPosV > JOY_MIDDLE_MAX), bewegt sich der Roboter 
    // vorwärts.

    if (nJoyPosV < JOY_MIDDLE_MIN) {
        // rückwärts
        nMotorDir = BACKWARD;

        // Motorgeschwindigkeit ist abhängig davon, wie weit der Joystick
        // gedrückt wird.
        
        // für die Rückwärtsfahrt werden die Werte umgekehrt
        
        nJoyPosV = nJoyPosV - JOY_MIDDLE_MIN; // negative Zahlen
        nJoyPosV = nJoyPosV * -1;  // positiv machen

        nMotorSpeed1 = map(nJoyPosV, 0, JOY_MIDDLE_MIN, 0, MAX_SPEED);
        nMotorSpeed2 = map(nJoyPosV, 0, JOY_MIDDLE_MIN, 0, MAX_SPEED);
    }
    else if (nJoyPosV > JOY_MIDDLE_MAX) {
        // vorwärts
        nMotorDir = FORWARD;

        // Motorgeschwindigkeit ermitteln
        nMotorSpeed1 = map(nJoyPosV, JOY_MIDDLE_MAX, 1023, 0, MAX_SPEED);
        nMotorSpeed2 = map(nJoyPosV, JOY_MIDDLE_MAX, 1023, 0, MAX_SPEED);
    }
    else {
        // anhalten
        
        nMotorSpeed1 = 0;
        nMotorSpeed2 = 0;
    }

    // Steuerung links und rechts

    // Die horizontale Joystickposition beeinflusst die Motorgeschwindigkeit    

    if (nJoyPosH < JOY_MIDDLE_MIN) {
        // links
        
        // Bei der Bewegung nach links werden die Werte umgekehrt
        
        nJoyPosH = nJoyPosH - JOY_MIDDLE_MIN; // negative Zahlen
        nJoyPosH = nJoyPosH * -1;  // positiv machen

        // Die Werte vom Joystick liegen zwischen 0 und 1023. Sie werden 
        // auf den Bereich der Motorgeschwindigkeit von 0 bis MAX_SPEED
        // abgebildet.
        
        nJoyPosH = map(nJoyPosH, 0, JOY_MIDDLE_MIN, 0, MAX_SPEED);

        // Motorgeschwindigkeiten setzen
        nMotorSpeed1 = nMotorSpeed1 - nJoyPosH;
        nMotorSpeed2 = nMotorSpeed2 + nJoyPosH;

        // Motorgeschwindigkeit im erlaubten Bereich halten
        if (nMotorSpeed1 < 0) {
            nMotorSpeed1 = 0;
        }
        if (nMotorSpeed2 > MAX_SPEED) {
            nMotorSpeed2 = MAX_SPEED;
        }
    }
    else if (nJoyPosH > JOY_MIDDLE_MAX) {
        // rechts

        // Die Werte vom Joystick liegen zwischen 0 und 1023. Sie werden 
        // auf den Bereich der Motorgeschwindigkeit von 0 bis MAX_SPEED
        // abgebildet.

        nJoyPosH = map(nJoyPosH, JOY_MIDDLE_MAX, 1023, 0, MAX_SPEED);

        // Motorgeschwindigkeiten setzen
        nMotorSpeed1 = nMotorSpeed1 + nJoyPosH;
        nMotorSpeed2 = nMotorSpeed2 - nJoyPosH;

        // Motorgeschwindigkeit im erlaubten Bereich halten
        if (nMotorSpeed1 > MAX_SPEED) {
            nMotorSpeed1 = MAX_SPEED;
        }
        if (nMotorSpeed2 < 0) {
            nMotorSpeed2 = 0;
        }
    }

    // bei sehr niedrigen Motorgeschwindigkeiten laufen die Motoren 
    // nicht an, manchmal entsteht ein summendes Geräusch. Das wird hier
    // unterdrückt.

    // Motorgeschwindigkeit mit den LEDs anzeigen
    if (nMotorSpeed1 < 8) {
        nMotorSpeed1 = 0;
    }
    if (nMotorSpeed2 < 8) {
        nMotorSpeed2 = 0;
    }
    if ((nMotorSpeed1 == 0) || (nMotorSpeed2 == 0)) {
        leds[0] = CRGB::White;
        leds[1] = CRGB::White;
    }
    else if (FORWARD == nMotorDir) {
        leds[0] = CRGB::Blue;
        leds[1] = CRGB::Blue;
    }
    else if (BACKWARD == nMotorDir) {
        leds[0] = CRGB::Green;
        leds[1] = CRGB::Green;
    }

    FastLED.show();

    // Motorgeschwindigkeiten setzen    
    
    // startMotors(int nDirA, int nSpeedA, int nDirB, int nSpeedB);
    startMotors(nMotorDir, nMotorSpeed1, nMotorDir, nMotorSpeed2);

}

// --- END FUNCTIONS MANUAL_CONTROL ---


void setup() {

// --- BEGIN SETUP GENERAL ---

    // Serielle Schnittstelle konfigurieren
    Serial.begin(115200);

// --- END SETUP GENERAL ---


// --- BEGIN SETUP BUMPER ---

    // Kollisionserkennung
    pinMode(BUMPER_PIN, INPUT_PULLUP);

// --- END SETUP BUMPER ---


// --- BEGIN SETUP SERVO ---

    pinMode(SERVO_1_PIN, OUTPUT);
    pinMode(SERVO_2_PIN, OUTPUT);

    // Servos beim Start hin und her bewegen (Funktionstest)
    
    startServo2(90);
    startServo1(90);
    delay(500);
    stopServo2();
    startServo2(45);
    stopServo1();
    startServo1(45);
    delay(500);
    stopServo2();
    startServo2(135);
    stopServo1();
    startServo1(135);
    delay(500);
    stopServo2();
    stopServo1();

    // in Mittelstellung ausrichten
    delay(500);
    startServo2(90);
    startServo1(90);
    delay(500);
    stopServo2();
    stopServo1();

    targetPositionServo2 = 90;
    actualPositionServo2 = 90;
    targetPositionServo1 = 90;
    actualPositionServo1 = 90;

// --- END SETUP SERVO ---


// --- BEGIN SETUP BUZZER ---

    // Startmelodie abspielen
    for (int thisNote = 0; thisNote < melody_length; thisNote++) { // alle Noten im Array durchlaufen
        TimerFreeTone(TONE_PIN, melody[thisNote], duration[thisNote]); // Note abspielen
        delay(50); // kurze Pause zwischen den Noten
    }

// --- END SETUP BUZZER ---


// --- BEGIN SETUP MOTOR ---

    // Pins für die Motorkontrolle als OUTPUT definieren
    pinMode(MOTOR_A_ENABLE_PIN, OUTPUT);
    pinMode(MOTOR_B_ENABLE_PIN, OUTPUT);
    pinMode(MOTOR_A_IN1_PIN, OUTPUT);
    pinMode(MOTOR_A_IN2_PIN, OUTPUT);
    pinMode(MOTOR_B_IN3_PIN, OUTPUT);
    pinMode(MOTOR_B_IN4_PIN, OUTPUT);

    pinMode(MOTOR_STANDBY_PIN, OUTPUT);

    // Motor Treiber einschalten
    digitalWrite(MOTOR_STANDBY_PIN, HIGH);

    // Motoren aus und Richtung auf vorwärts einstellen

    // Motor A
    digitalWrite(MOTOR_A_ENABLE_PIN, LOW);
    digitalWrite(MOTOR_A_IN1_PIN, HIGH);
    digitalWrite(MOTOR_A_IN2_PIN, LOW);

    // Motor B
    digitalWrite(MOTOR_B_ENABLE_PIN, LOW);
    digitalWrite(MOTOR_B_IN3_PIN, HIGH);
    digitalWrite(MOTOR_B_IN4_PIN, LOW);

// --- END SETUP MOTOR ---


// --- BEGIN SETUP AUTOMANUAL ---

    // Betriebsmodus über den Abstand zum nächsten Objekt bestimmen
    
    // Abstand < 10cm: MANUAL
    // Abstand >= 10cm und <= 30cm: Hindernisvermeidung
    // Abstand > 30cm: Battle-Modus

    // falls der Joystick Button beim Einschalten gedrückt wird, ist der 
    // Modus ebenfalls MANUAL    
    
    int nDistance = getDistance(); 
    int bButtonPressed = digitalRead(BUMPER_PIN);

    if (LOW == bButtonPressed) {
        nMode = MANUAL; 
    }
    else if (nDistance < 10) {
        nMode = MANUAL;
    }
    else if (nDistance < 30) {
        nMode = AUTONOMOUS;
    }
    else {
        nMode = BATTLE;
    }


// --- BEGIN SETUP AUTOMANUAL ---


// --- BEGIN SETUP WS2811 ---

    pinMode(WS2811_PIN, OUTPUT);

    // LEDS initialisieren
    FastLED.addLeds<WS2811, WS2811_PIN, RGB>(leds, NUM_LEDS);

    // alle LEDs ausschalten
    // fill_solid(leds, NUM_LEDS, CRGB::Black);

    // verschiedene Farben einstellen
    leds[0] = CRGB::Aqua;
    leds[1] = CRGB::Aqua;
    leds[2] = CRGB::White;
    leds[3] = CRGB::White;
    leds[4] = CRGB::Green;
    leds[5] = CRGB::Green;
    leds[6] = CRGB::White;
    leds[7] = CRGB::Red;
    leds[8] = CRGB::Blue;
    leds[9] = CRGB::Blue;
    leds[10] = CRGB::White;
    leds[11] = CRGB::White;

    // Helligkeit
    FastLED.setBrightness(50);

    // Einstellungen setzen
    FastLED.show();

    // alle LEDs nacheinander ausschalten
    delay(500);
    for (int i=0; i<NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
        delay(50);
    }

    // Betriebsmodus mit Farben anzeigen 
    switch (nMode) {
        case AUTONOMOUS:
            leds[1] = CRGB::Red;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Black;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Red;
            FastLED.show();
            break;

        case BATTLE:
            leds[1] = CRGB::Green;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Black;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Green;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Black;
            FastLED.show();
            delay(500);
            leds[1] = CRGB::Green;
            FastLED.show();
            break;

        case MANUAL:
        default:
            // LED Farbe abhängig von der Joystickposition setzen
            int nJoyHPos = analogRead(JOY_HORIZONTAL_PIN);
            int nJoyColorValue = map(nJoyHPos, 0, 1023, 0, 255);

            // feste LED Farbe einstellen
            // leds[0] = CRGB::Blue;
            // leds[1] = CRGB::Blue;

            // Farben für jeden Kanal einzeln einstellen
            leds[0].red = nJoyColorValue;
            leds[0].green = 255 - nJoyColorValue;
            leds[0].blue =  150;

            // Farben für alle Kanäle gleichzeitig setzen
            leds[1] = CRGB( nJoyColorValue, 255 - nJoyColorValue, 150);

            FastLED.show();
    }

// --- END SETUP WS2811 ---


// --- BEGIN SETUP JOYSTICK ---

    // Das Setzen der Pins für den Joystick auf den Default INPUT 
    // stört die Ansteuerung der Motoren, daher auf Default lassen.

    // pinMode(joyVert, INPUT);
    // pinMode(joyHorz, INPUT);

// --- END SETUP JOYSTICK ---

}

void loop() {

// --- BEGIN LOOP MOTOR ---

    // Handlung entsprechend dem Betriebsmodus ausführen
    
    switch (nMode) {
        case AUTONOMOUS:
            avoidObstacles();
            break;

        case BATTLE:
            doBattle();
            break;

        case MANUAL:
            default:
            manualControl();
    }

// --- END LOOP MOTOR ---


// --- BEGIN LOOP SERVO ---
    
    // Servo 1 hin und her bewegen
    moveServoBackForth();

// --- END LOOP SERVO ---


}
