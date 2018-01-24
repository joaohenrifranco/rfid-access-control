/*
 *  Libraries
 */
#include <SPI.h>
#include <MFRC522.h>

/*
 *  Constants
 */
#define SERIAL_SPEED      9600

/*
 *  Pins
 */
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

/*
 *  Setup
 */
void setup() 
{
  Serial.begin(SERIAL_SPEED);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("All set!");
  Serial.println("Please approach your card...");
  Serial.println();
}

/*
 *  Loop
 */
void loop() 
{
  if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if ( ! rfid.PICC_ReadCardSerial())
    return;
  Serial.print("Tag UID: ");
  String rfid_read= "";
  for (byte i = 0; i < rfid.uid.size; i++) 
  {
     rfid_read.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
     rfid_read.concat(String(rfid.uid.uidByte[i], HEX));
     Serial.println(rfid_read);
  }
  Serial.println();
  Serial.print("Mensagem : ");
  rfid_read.toUpperCase();
}