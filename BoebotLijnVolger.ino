/*******************************************
*  Arduino programma Boebot LijnVolger voor het project Project VEPROF1A-13, Periode A, studiejaar 19-20
*  Dit programma is opgebouwd in functies met een mogelijke structuur en dient ter inspiratie
*  Het is onvolledig en goede werking is niet gegarandeerd
*  
*  Auteur: Chris, Ferris, Youri en Roy
*  Datum: 8/11/2022
*  Versie 7
*  Taakverdeling: SensorRead = Youri, Obstakel_Verwijdering = Ferris, Overige code = Chris & Roy (peer to peer programming)
********************************************/
#include <Arduino.h>
#include "Servo.h"                // voegt de library's toe die nodig zijn om de servo's aan te sturen via PWM
#define LEVEL 720                 // Analoog grensniveau van de (lijn/geen lijn) detectie van de reflectiemeting met de CNY70 
#define STP 1500                  // waarde in micro seconden om de rotating servo's te laten stoppen

// maakt servo objects aan
Servo servoL;                     
Servo servoR;                     
Servo armServo;                      

// selecteer de input pins voor reflectiesensoren a met een vaste waarde (const)
const int Reflectpin_a = A0;      
const int Reflectpin_b = A1;      
const int Reflectpin_c = A2;      
const int Reflectpin_d = A3;      
const int Reflectpin_e = A4;      
const int afstandSensor= A5;      

int led1 = 2;                     
int led2 = 3;                     
int led3 = 4;                     
int led4 = 5;                     
int led5 = 6;                     

char outOfBounds = 0;             
char draaiOffset = 50;            

int ReflectValue_a = 0;           
int ReflectValue_b = 0;           
int ReflectValue_c = 0;           
int ReflectValue_d = 0;           
int ReflectValue_e = 0;           
int ReturnValue = 0;              

//Prototypes:
char SensorRead(void);                      // Prototype functie: returnwaarde:  0000DCBA  A, B, C, D    0 of 1 afhankelijk of sensorwaarde hoger of lager dan LEVEL is
void Actie(int);                            // Prototype functie: bepaald servoactie op basis returnwaarde SensorRead (bv rechts, links of vooruit)
void Servosturing(int Links, int Rechts);   // Prototype functie sturing sensors
void SensorStatus2LEDS(char);               // Maak status van sensoren zichtbaar door middel van LEDS
void Obstakel_Verwijdering(void);           // functie om obstakel te detecteren en te verwijderen
void Error(char);                           // Afvangen foutmeldingen, bijvoorbeeld als alle sensoren "zwart" zien

void setup()                                // initialisatie, wordt één keer uitgevoerd
{
  servoL.attach(11);                        
  servoR.attach(10);                        
  armServo.attach(12);  

  pinMode(afstandSensor,INPUT);             // maak de afstandsensor een input
  pinMode(led1, OUTPUT);                    // stel LED pins in als Output
  pinMode(led2, OUTPUT);                    
  pinMode(led3, OUTPUT);                    
  pinMode(led4, OUTPUT);                    
  pinMode(led5, OUTPUT);                    
  pinMode(SensorToggle, OUTPUT);            // Optie batterijbesparing stel pin "SensorToggle" in als Output
  
  Serial.begin(9600);                       //initiatie serial modem 9600 bps
  Serial.println("Lijvolger 3000 ready! \n Seriele Communicatie OK\n"); // test de seriële communicatie
}

void loop()                         //hoofdprogramma met een aantal functieaanroepen
{ 
char SensorOutput;                  
SensorOutput = SensorRead();        // Verander SensorOutput naar de return value van de SensorRead functie
//Serial.print("bitwaarde:");       // Print voor debugging bin waardes
//Serial.println(SensorOutput, BIN);// Print voor debugging bin waardes
Actie(SensorOutput);                
SensorStatus2LEDS(SensorOutput);    
Obstakel_Verwijdering();            
delay(300);                         // Delay die voorkomt dat er onnodig veel gelooped wordt (deze delay bepaalt (mede) de reactiesnelheid) 
}

//Functies:
char SensorRead(void) // Functie leest reflectiesensoren uit en returned de waardes
{
//digitalWrite(SensorToggle, HIGH); // Optie: batterij besparing: Schakel de sensoren in
// Reset waardes van de reflectiesensoren
ReflectValue_a = 0;           
ReflectValue_b = 0;           
ReflectValue_c = 0;           
ReflectValue_d = 0;           
ReflectValue_e = 0;           
ReturnValue = 0;              

// Koppel variabele aan de correcte sensoroutput
ReflectValue_a = analogRead(Reflectpin_a);                                   
ReflectValue_b = analogRead(Reflectpin_b);                                   
ReflectValue_c = analogRead(Reflectpin_c);                                   
ReflectValue_d = analogRead(Reflectpin_d);                                   
ReflectValue_e = analogRead(Reflectpin_e);                                   

if(ReflectValue_a < LEVEL) ReturnValue |= 0x01; else ReturnValue &= ~(0x00); // Check of de vooruit sensor onder de treshhold zit 
if(ReflectValue_b < LEVEL) ReturnValue |= 0x02; else ReturnValue &= ~(0x00); // Check of de linker sensor onder de treshhold zit 
if(ReflectValue_c < LEVEL) ReturnValue |= 0x04; else ReturnValue &= ~(0x00); // Check of de rechter sensor onder de treshhold zit 
if(ReflectValue_d < LEVEL) ReturnValue |= 0x08; else ReturnValue &= ~(0x00); // Check of de rechter achter sensor onder de treshhold zit 
if(ReflectValue_e < LEVEL) ReturnValue |= 0x10; else ReturnValue &= ~(0x00); // Check of de linker achter sensor onder de treshhold zit 
 
//digitalWrite(SensorToggle, LOW); // Optie batterijbesparing: Schakel de sensoren uit

return ReturnValue; // return uitgelezen reflectiesensor waarde
}

void Actie(char Waarde) // Deze functie laat de boebot bewegen met gebruik van de verkregen Waarde
{
  if ((Waarde & 0x01) && outOfBounds == 0) // Forceer vooruit als de vooruit sensor de lijn ziet
  { 
    ServoSturing(1000, 2000);              // Beweeg vooruit
  }
  else
  {
    switch (Waarde) // Check of er een case overeenkomt met Waarde // 00001 = vooruit  // 00010 = links // 00100 = rechts //01000 = links achter //10000 = rechts achter
    {
    case 0x00:
    {
      int draaiDuratie = 50;                        // Draai duratie default op 50                 
      //Serial.print("foutcode:");                  // Debug foutcode
      //Serial.println(fout);                       // Debug fout index
      if (outOfBounds == 2)                         // Check of bot 2 maal de fout in is gegaan
      {
        ServoSturing(1400, 1400);                   // Draai naar links
        delay(draaiDuratie);                        // Voeg delay toe met waarde van DraaiDuratie, zodat er een sweep ontstaat die naar de lijn zoekt
        if (ReflectValue_a < LEVEL){return;}        // Stop zodra de lijn gevonden is 
        while (true)
        {
          draaiDuratie = draaiDuratie + draaiOffset;         // Vergroot draaiDuratie (draaicirkel) van de sweep met draaiOffset
          ServoSturing(1400, 1400);                          // Draai naar links
          delay(draaiDuratie);                               // Voeg delay toe met waarde van DraaiDuratie, zodat er een sweep ontstaat die naar de lijn zoekt
          if (ReflectValue_a < 720){outOfBounds = 0;return;} // Reset de variabele die de fout bijhoud en stop met zoeken naar de lijn

          draaiDuratie = draaiDuratie + draaiOffset;
          ServoSturing(1600, 1600);                          // Draai naar Rechts
          delay(draaiDuratie);                               
          if (ReflectValue_a < 720){outOfBounds = 0;return;} 
        }
      }
      else{ ServoSturing(2000, 1000); }                     // Als de fout variabele niet gelijk staat aan 2, ga achteruit

      if (outOfBounds == 1) outOfBounds = 2;                // Verhoog fout variabele als deze al 1 is
      else outOfBounds = 1;                                 // Maak fout variabele 1, omdat 0x00 de 'ik zie niks' case is
    }
    break; 
           
           
    case 0x01:{ServoSturing(1000, 2000);}break;                                                                        // 00001 = ga vooruit           
    case 0x02:{ServoSturing(1400, 1400);outOfBounds = 0;}break;                                                        // 00010 = draai links       
    case 0x03:{if (outOfBounds == 1) {ServoSturing(1400, 1400);}else{ServoSturing(1000, 2000);}outOfBounds = 0;}break; // 00011 = ga vooruit
    case 0x04:{ServoSturing(1600, 1600);outOfBounds = 0;}break;                                                        // 00100 = draai rechts
    case 0x05:{if (outOfBounds == 1){ServoSturing(1600, 1600);}else{ServoSturing(1000, 2000);}outOfBounds = 0;}break;  // 00101 = ga vooruit
    case 0x06:{ServoSturing(1000, 2000);}break;                                                                        // 00110 = ga vooruit (komt in de praktijk nauwelijks voor i.v.m. positionisering)
    case 0x07:{ServoSturing(1000, 2000);}break;                                                                        // 00111 = ga vooruit

    case 0x08:{ServoSturing(1400, 1400);}break;                                                                        // 01000 = draai links 
    case 0x10:{ServoSturing(1600, 1600);}break;                                                                        // 10000 = draai rechts 

    default: ServoSturing(1000, 2000);                                                                                 // ga vooruit bij de overige cases
    }
  }
}

void ServoSturing(int Links, int Rechts){  // Stuur servo's aan met meegegeven waardes
      servoL.writeMicroseconds(Links);
      servoR.writeMicroseconds(Rechts);
}


void SensorStatus2LEDS(char LEDs){ // Maak de status van de sensoren zichtbaar door middel van leds
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);

  switch(LEDs){
  case 0x01: digitalWrite(led1, HIGH);
  break;
  
  case 0x02: digitalWrite(led2, HIGH);
  break;
  
  case 0x03: {digitalWrite(led1, HIGH); digitalWrite(led2, HIGH);}
  break;
  
  case 0x04: {digitalWrite(led3, HIGH); digitalWrite(led3, HIGH);}
  break;
  
  case 0x05: {digitalWrite(led1, HIGH); digitalWrite(led3, HIGH);}
  break;
  
  case 0x06: {digitalWrite(led2, HIGH); digitalWrite(led3, HIGH);}
  break;
  
  case 0x07: {digitalWrite(led1, HIGH); digitalWrite(led2, HIGH); digitalWrite(led3, HIGH);}
  break;
  
  case 0x08: digitalWrite(led4, HIGH);
  break;
  
  case 0x09: {digitalWrite(led1, HIGH); digitalWrite(led4, HIGH);}
  break;
 
  case 0x0A: {digitalWrite(led2, HIGH); digitalWrite(led4, HIGH);}
  break;
  
  case 0x0B: {digitalWrite(led1, HIGH); digitalWrite(led2, HIGH);digitalWrite(led4, HIGH);}
  break;
  
  case 0x0C: {digitalWrite(led3, HIGH); digitalWrite(led4, HIGH);}
  break;
  
  case 0x0D: {digitalWrite(led1, HIGH); digitalWrite(led3, HIGH); digitalWrite(led4, HIGH);}
  break;
  
  case 0x0E: {digitalWrite(led2, HIGH); digitalWrite(led3, HIGH); digitalWrite(led4, HIGH);}
  break;
 
  case 0x0F: {digitalWrite(led1, HIGH); digitalWrite(led2, HIGH); digitalWrite(led3, HIGH); digitalWrite(led4, HIGH);}
  break;
  }
}

void Obstakel_Verwijdering(void) // Deze functie checked voor obstakels en past de nodige handelingen toe
{
  int armPositie = 0;                                      // maakt een variabele aan waarmee de servo heen en weer bewogen kan worden
  int afstandWaarde = analogRead(afstandSensor);           // maakt een variabele aan en sla output afstandSensor erin op
  if(afstandWaarde > 450){                                 // Check of de sensor binnen de gegeven afstand een object detecteerd
  ServoSturing(1462, 1500);                                // Stop met rijden
    for(armPositie = 0; armPositie <180; armPositie += 1){ // Beweeg de servo heen en weer (sla het obstakel weg)
    armServo.write(armPositie);                            // Beweeg servo met de waarde van armPositie
    delay(10);
    }
  }
}

void Error(char w){ // Geef errors weer voor debugging
  switch(w){
  case 1: Serial.println("Sensors zien geen lijn");
  break;
  case 2: Serial.println("Alle sensors zien een lijn");
  break;
  case 3: Serial.println("Systeemfout");
  break;
  }
}
