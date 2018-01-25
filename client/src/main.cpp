/*
 *
 *  This code is being compiled for Arduino Pro Mini 16MHz
 *
 */

/*
 *  Libraries
 */
#include <SPI.h>
#include <MFRC522.h>
#include <Ethernet.h>

/*
 *  Constants
 */
#define SERIAL_SPEED      9600
#define MAC_ADDRESS       {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
#define IP_ADDRESS        {192, 168, 88, 88}
#define GATEWAY           {192, 168, 88, 1}
#define SUBNET            {255, 255, 255, 0}
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
 *  Declaring IP, MAC and the ethernet client itself
 */
IPAddress ip(IP_ADDRESS);
#if defined(WIZ550io_WITH_MACADDRESS)
;
#else
byte mac[] = MAC_ADDRESS;
#endif
EthernetClient ethClient;

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
  // Starts serial communication for debugging purposes
  Serial.begin(SERIAL_SPEED);
  // Starts SPI communication for devices
  SPI.begin();
  // Initializes the RFID module
  rfid.PCD_Init();
  // Initializes the Ethernet module
  Ethernet.begin(mac, ip);
  // Prints messages indicating that all setup has been done
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