// Ultrasonic control pin
const int Trig = 12;
const int Echo = 13;
// PWM control pin
#define PWM1_PIN            5
#define PWM2_PIN            6      
// 74HCT595N chip pin
#define SHCP_PIN            2                               // The displacement of the clock
#define EN_PIN              7                               // Can make control
#define DATA_PIN            8                               // Serial data
#define STCP_PIN            4                               // Memory register clock          

#define LEFT_LINE_TRACKING          A0
#define CENTER_LINE_TRACKING        A1
#define RIGHT_LINE_TRACKING         A2

const int Forward       = 92;                               // forward
const int Backward      = 163;                              // back
const int Turn_Left     = 149;                              // left translation
const int Turn_Right    = 106;                              // Right translation 
const int Top_Left      = 20;                               // Upper left mobile
const int Bottom_Left   = 129;                              // Lower left mobile
const int Top_Right     = 72;                               // Upper right mobile
const int Bottom_Right  = 34;                               // The lower right move
const int Stop          = 0;                                // stop
const int Contrarotate  = 172;                              // Counterclockwise rotation
const int Clockwise     = 83;                               // Rotate clockwise

int Left_Tra_Value;
int Center_Tra_Value;
int Right_Tra_Value;
int Black_Line = 500; 


float SR04(int Trig, int Echo)
{
    digitalWrite(Trig, LOW);
    delayMicroseconds(2);
    digitalWrite(Trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(Trig, LOW);
    float distance = pulseIn(Echo, HIGH) / 58.00;
    delay(10);
    return distance;
}

void setup() {
    pinMode(Trig, OUTPUT);
    pinMode(Echo, INPUT);

    pinMode(SHCP_PIN, OUTPUT);
    pinMode(EN_PIN, OUTPUT);
    pinMode(DATA_PIN, OUTPUT);
    pinMode(STCP_PIN, OUTPUT);
    pinMode(PWM1_PIN, OUTPUT);
    pinMode(PWM2_PIN, OUTPUT);

    pinMode(LEFT_LINE_TRACKING, INPUT);
    pinMode(CENTER_LINE_TRACKING, INPUT);
    pinMode(RIGHT_LINE_TRACKING, INPUT);
    Serial.begin(9600);

}

// Stările posibile ale mașinuței
enum State {
    LINE_TRACKING,
    AVOID_OBSTACLE,
    RETURN_TO_LINE
};

State currentState = LINE_TRACKING; // Starea inițială este IDLE

// Variabile pentru temporizări folosind millis
unsigned long previousMillis = 0;
const long lineTrackingInterval = 100; // Interval pentru actualizarea senzorilor (100 ms)

bool ledState = LOW; // Starea LED-ului pentru intermitență

void loop() {
    unsigned long currentMillis = millis();
    int Avoidance_distance = 0;

    // Citim valorile senzorilor de linie doar dacă a trecut timpul pentru line tracking
    if (currentMillis - previousMillis >= lineTrackingInterval) {
        previousMillis = currentMillis;

        Left_Tra_Value = analogRead(LEFT_LINE_TRACKING);
        Center_Tra_Value = analogRead(CENTER_LINE_TRACKING);
        Right_Tra_Value = analogRead(RIGHT_LINE_TRACKING);

        // Gestionăm logica state machine-ului
        switch (currentState) {
            case LINE_TRACKING:
                // În starea FOLLOW_LINE, urmărim linia și verificăm obstacolele
                // Citim valoarea senzorului ultrasonic
                Avoidance_distance = SR04(Trig, Echo);
                if (Avoidance_distance <= 25) {
                    Serial.println("Obstacol detectat! Oprire.");
                    Motor(Stop, 0);
                    currentState = AVOID_OBSTACLE;  // Trecem la starea de oprire
                } else {
                    // Urmărirea liniei negre
                    if (Center_Tra_Value >= Black_Line) {
                        Motor(Forward, 250);  // Mergi înainte
                    } else if (Left_Tra_Value >= Black_Line) {
                        Motor(Contrarotate, 200);  // Virează stânga
                    } else if (Right_Tra_Value >= Black_Line) {
                        Motor(Clockwise, 200);  // Virează dreapta
                    } else {
                        // Dacă linia nu este detectată, execută mișcări scurte stânga-dreapta
                        Serial.println("Linia nu este detectată. Mișcări de căutare stânga-dreapta.");

                        Motor(Contrarotate, 150); // Mică rotire spre stânga
                        delay(300);

                        Motor(Clockwise, 150);    // Mică rotire spre dreapta
                        delay(300);

                        Motor(Stop, 0);           // Pauză scurtă pentru stabilizare
                        delay(200);
                        currentState = RETURN_TO_LINE;
                    }
                }
                break;

            case AVOID_OBSTACLE:
                // În starea STOPPED_BY_OBSTACLE, așteptăm culoarea verde pentru a continua
                 Serial.println("Stare: AVOID_OBSTACLE");

                // Se retrage mai mult în spate
                Motor(Backward, 180);
                delay(1000);  // Retragere timp mai lung
                
                // Mișcă-te circular (în sens orar sau anti-orar) pentru a evita obstacolul
                Motor(Clockwise, 180);
                delay(800); // Rotire completă

                Motor(Stop, 0); // Oprește înainte de a căuta linia
                delay(200);

                currentState = RETURN_TO_LINE;  // După evitarea obstacolului, trecem la căutarea liniei
                break;

            case RETURN_TO_LINE:
                // În starea RETURN_TO_LINE, mașina încearcă să găsească linia prin mișcări de căutare
                Serial.println("Stare: RETURN_TO_LINE");

                unsigned long returnStart = millis(); // Timpul de început pentru căutare
                bool lineFound = false;

                while (!lineFound && millis() - returnStart < 5000) {  // Caută linia timp de 5 secunde
                    Left_Tra_Value = analogRead(LEFT_LINE_TRACKING);
                    Center_Tra_Value = analogRead(CENTER_LINE_TRACKING);
                    Right_Tra_Value = analogRead(RIGHT_LINE_TRACKING);

                    if (Center_Tra_Value >= Black_Line) {
                        lineFound = true;
                        currentState = LINE_TRACKING;
                        //Motor(Forward, 200); // Mergi înainte pe linie
                        break;
                    } else if (Left_Tra_Value >= Black_Line) {
                        lineFound = true;
                        currentState = LINE_TRACKING;
                        //Motor(Contrarotate, 200); // Găsește linia și virează stânga
                        break;
                    } else if (Right_Tra_Value >= Black_Line) {
                        lineFound = true;
                        currentState = LINE_TRACKING;
                        //Motor(Clockwise, 200); // Găsește linia și virează dreapta
                        break;
                    }

                    // Dacă nu găsește linia, continuă să facă mișcări circulare
                    Motor(Clockwise, 150);
                    delay(500);
                    Motor(Backward, 150);
                    delay(700);
                    Motor(Contrarotate, 150);
                    delay(500);
                    Motor(Forward, 150);
                    delay(700);
                }

                if (!lineFound) {
                    Serial.println("Linia nu a fost găsită. Repetăm căutarea.");
                } else {
                    Serial.println("Linia a fost găsită. Revenim la LINE_TRACKING.");
                }

                currentState = LINE_TRACKING;  // Revenim la urmărirea liniei
                break;

            default:
                currentState = LINE_TRACKING;
                break;
        }

            // Afișăm valorile senzorilor pentru diagnosticare
            Serial.print("Left tracking value: ");
            Serial.println(Left_Tra_Value);
            Serial.print("Center tracking value: ");
            Serial.println(Center_Tra_Value);
            Serial.print("Right tracking value: ");
            Serial.println(Right_Tra_Value);
            Serial.print("Avoidance distance: ");
            Serial.println(Avoidance_distance);
    }
    
}

void Motor(int Dir, int Speed) {
    digitalWrite(EN_PIN, LOW);
    analogWrite(PWM1_PIN, Speed);
    analogWrite(PWM2_PIN, Speed);

    digitalWrite(STCP_PIN, LOW);
    shiftOut(DATA_PIN, SHCP_PIN, MSBFIRST, Dir);
    digitalWrite(STCP_PIN, HIGH);
}
