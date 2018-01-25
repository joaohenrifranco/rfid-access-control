/*
 *
 *  This code is for Arduino MEGA 2560, for testing purposes;
 *
 */

/*
 *  Libraries
 */
#include <SPI.h>
#include <MFRC522.h>

/*
 *  Constants
 */
#define SERIAL_SPEED      9600
#define BLACK             {0, 0, 0}       //Turn LED off
#define BLUE              {0, 0, 255}
#define GREEN             {0, 255, 0}
#define AQUA              {0, 255, 255}   //Light blue
#define RED               {255, 0, 0}
#define FUCHSIA           {255, 0, 255}   //Kinda purple
#define YELLOW            {255, 255, 0}
#define WHITE             {255, 255, 255}

/*
 *  Pins
 */
#define SS_PIN            22
#define RST_PIN           24
#define LED_R             49
#define LED_G             51
#define LED_B             53

/*
 *  Declaring the RFID module
 */
MFRC522 rfid(SS_PIN, RST_PIN);

/*
 *  void WriteRGB (byte color[]);
 *
 *  Description:
 *  - Procedure that changes LED color
 *
 *  Inputs/Outputs:
 *  [INPUT] byte color[]: array that contatins red, green and blue values
 *
 *  Returns:
 *  -
 */
void WriteRGB (byte color[])
{
  byte pins[] = {LED_R, LED_G, LED_B};
  for (byte i = 0; i < 3; i++)
    digitalWrite(pins[i], color[i]);
}

/*
 *  String ReadRFIDTag ();
 *
 *  Description:
 *  - Function to read an UID Tag from the RFID Module
 *
 *  Inputs/Outputs:
 *  -
 *
 *  Returns:
 *  [String] The UID tag itself in uppercase
 */
String ReadRFIDTag ()
{
  String aux = "";
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    aux.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
    aux.concat(String(rfid.uid.uidByte[i], HEX));
  }
  aux.toUpperCase();
  return aux;
}

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
  rfid_read.toUpperCase();
}