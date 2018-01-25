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
#include <Keypad.h>
#include <Hash.h>

/*
 *  Macros
 */
#define SERIAL_SPEED      9600
#define MAC_ADDRESS       {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
//#define IP_ADDRESS        {192, 168, 88, 88}
//#define GATEWAY           {192, 168, 88, 1}
//#define SUBNET            {255, 255, 255, 0}
#define BLACK             {0, 0, 0}       //Turn LED off
#define BLUE              {0, 0, 255}
#define GREEN             {0, 255, 0}
#define AQUA              {0, 255, 255}   //Light blue
#define RED               {255, 0, 0}
#define FUCHSIA           {255, 0, 255}   //Kinda purple
#define YELLOW            {255, 255, 0}
#define WHITE             {255, 255, 255}
#define KEYPAD_LINES      4
#define KEYPAD_COLUMNS    3
#define KEYS              {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}}
#define END_OF_PASSWORD   '#'
#define QUIT_TYPING       '*'

/*
 *  Pins
 */
#define SS_PIN            53
#define RST_PIN           49
#define LED_R             47
#define LED_G             45
#define LED_B             43
#define KEYPAD_LIN_PINS   {1, 2, 3, 4}
#define KEYPAD_COL_PINS   {5, 6, 7}

/*
 *  Declaring the RFID module
 */
MFRC522 rfid(SS_PIN, RST_PIN);

/*
 *  Declaring IP, MAC and the ethernet client itself
 */
//IPAddress ip(IP_ADDRESS);
#if defined(WIZ550io_WITH_MACADDRESS)
;
#else
byte mac[] = MAC_ADDRESS;
#endif
EthernetClient ethClient;

/*
 *  Declaring and initializing the keypad (4x3)
 */
char keys[KEYPAD_LINES][KEYPAD_COLUMNS] = KEYS;
byte keyPadLinPins[KEYPAD_LINES] = KEYPAD_LIN_PINS;
byte keyPadColPins[KEYPAD_COLUMNS] = KEYPAD_COL_PINS;
Keypad keyPad = Keypad(makeKeymap(keys), keyPadLinPins, keyPadColPins, KEYPAD_LINES, KEYPAD_COLUMNS);

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
  if ( ! rfid.PICC_IsNewCardPresent())
    return "";
  if ( ! rfid.PICC_ReadCardSerial())
    return "";
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
 *  String GetPassword ();
 *
 *  Description:
 *  - Function to get a sequence of characters from keypad until the END_OF_PASSWORD
 *  or QUIT_TYPING characters are pressed
 *
 *  Inputs/Outputs:
 *  -
 *
 *  Returns:
 *  [String] The password typed until the END_OF_PASSWORD character
 */
String GetPassword ()
{
  String aux = "";
  char c = keyPad.getKey();
  while (c != END_OF_PASSWORD)
  {
    if (c == QUIT_TYPING)
      return "";
    if (c)
    {
      aux.concat(c);
    }
    c = keyPad.getKey();
  }
  return aux;
}

/*
 *  String HashedPassword (String password);
 *
 *  Description:
 *  - Function to hash an existing password
 *
 *  Inputs/Outputs:
 *  [INPUT] String password: the plain text password to be hashed
 *
 *  Returns:
 *  [String] The hash function result
 */
String HashedPassword (String password)
{
  return (sha1(password));
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
  // Ethernet.begin(mac);
  // Prints messages indicating that all setup has been done
  Serial.println("All set!");
  Serial.println();
  // <<WARNING>> Remember to dump all read characters from keypad before asking for password!
}

/*
 *  Loop
 */
void loop() 
{
  String pass = GetPassword();
  if (pass == "")
    Serial.println("Invalid password");
  else
  {
    Serial.println(pass);
    Serial.print("Hash: ");
    Serial.println(HashedPassword(pass));
  }
}