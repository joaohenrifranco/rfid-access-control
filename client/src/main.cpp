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
#include <sha256.h>

/*
 *  Macros
 */
#define ROOM_ACCESS_LEVEL 2
#define SERIAL_SPEED      9600
#define MAC_ADDRESS       {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED} // Change MAC for individual modules
//#define IP_ADDRESS        {192, 168, 88, 88}    //MY_OWN_IP
//#define GATEWAY           {192, 168, 88, 1}
//#define SUBNET            {255, 255, 255, 0}
#define SERVER_IP         {192, 168, 88, 61}
#define REQUEST_UNLOCK    "http://192.168.88.61:8000/api/request-unlock"
#define AUTHENTICATE      "http://192.168.88.61:8000/api/authenticate"
#define AUTHORIZE_VISITOR "http://192.168.88.61:8000/api/authorize-visitor"
#define REQUEST_PORT      8000                  //Standard HTTP port
#define BLACK             {0, 0, 0}             //Turn LED off
#define BLUE              {0, 0, 255}
#define GREEN             {0, 255, 0}
#define AQUA              {0, 255, 255}         //Light blue
#define RED               {255, 0, 0}
#define FUCHSIA           {255, 0, 255}         //Kinda purple
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
#define SS_PIN_IN         53
#define SS_PIN_OUT        48
#define RST_PIN_IN        49
#define RST_PIN_OUT       47
#define LED_IN_R          27
#define LED_IN_G          29
#define LED_IN_B          31
#define LED_OUT_R         26
#define LED_OUT_G         28
#define LED_OUT_B         30
#define KEYPAD_LIN_PINS   {33, 35, 37, 39}
#define KEYPAD_COL_PINS   {41, 43, 45}

/*
 *  Declaring the RFID modules
 */
MFRC522 rfid_in(SS_PIN_IN, RST_PIN_IN);
MFRC522 rfid_out(SS_PIN_OUT, RST_PIN_OUT);

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
void WriteRGB (byte color[], char in_out)
{
  if (in_out == 'i')
  {
    byte pins[] = {LED_IN_R, LED_IN_G, LED_IN_B};
    for (byte i = 0; i < 3; i++)
      digitalWrite(pins[i], color[i]);
  }
  else if (in_out == 'o')
  {
    byte pins[] = {LED_OUT_R, LED_OUT_G, LED_OUT_B};
    for (byte i = 0; i < 3; i++)
      digitalWrite(pins[i], color[i]);
  }
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
String ReadRFIDTag (MFRC522 rfid)
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
 *  String readableHash(uint8_t* hash);
 *
 *  Description:
 *  - Function to convert hashed password to a readable string
 *
 *  Inputs/Outputs:
 *  [INPUT] String hash: the hashed password
 *
 *  Returns:
 *  [String] A 64-character long string with containing the hashed password
 */
String readableHash(uint8_t* hash)
{
  String out = "";
  for (byte i = 0; i < 32; i++)
  {
    out.concat("0123456789abcdef"[hash [i] >> 4]);
    out.concat("0123456789abcdef"[hash [i] & 0xf]);
  }
  return out;
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
  Sha256 hash_function;
  uint8_t *hash;
  hash_function.init();
  hash_function.print(password);
  hash = hash_function.result();
  return readableHash(hash);
}

/*
 *  String GenerateUnlockPostData (String rfidTag, byte roomId, byte action);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP post
 *
 *  Inputs/Outputs:
 *  [INPUT] String rfidTag: the RFID Tag read by function above
 *  [INPUT] byte roomId: the ID of the room where this client is
 *  [INPUT] byte action: indicates if the person is entering or leaving the room (level 3 security purposes)
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateUnlockPostData (String rfidTag, byte roomId, byte action)
{
  String aux = "{\n\t\"rfidTag\":\"";
  aux.concat(rfidTag);
  aux.concat("\",\n\t\"roomId\":");
  aux.concat(String(roomId));
  aux.concat(",\n\t\"action\":");
  aux.concat(String(action));
  aux.concat("\n}");
  return aux;
}

String GenerateAuthenticatePostData (String rfidTag, String password)
{
  String aux = "{\n\t\"rfidTag\":\"";
  aux.concat(rfidTag);
  aux.concat("\",\n\t\"password\":\"");
  aux.concat(String(password));
  aux.concat("\"\n}");
  return aux;
}

String GenerateVisitorPostData (String rfidTag, String rfidTagVisitor)
{
  String aux = "{\n\t\"rfidTag\":\"";
  aux.concat(rfidTag);
  aux.concat("\",\n\t\"rfidTagVisitor\":\"");
  aux.concat(String(rfidTagVisitor));
  aux.concat("\"\n}");
  return aux;
}

/*
 *  String SendPostRequest (String rfidTag, byte roomId, byte action, byte status);
 *
 *  Description:
 *  - Does a POST Request
 *
 *  Inputs/Outputs:
 *  [INPUT] String rfidTag: the RFID Tag read by function above
 *  [INPUT] byte roomId: the ID of the room where this client is
 *  [INPUT] byte action: indicates if the person is entering or leaving the room (level 3 security purposes)
 *
 *  Returns:
 *  -
 */
String SendPostRequest(String postData, String requestFrom)
{
  String output = "";
  IPAddress serverIP(SERVER_IP);
  //IPAddress myIP(MY_OWN_IP);
  int requestPort = REQUEST_PORT;
  int connection = ethClient.connect("192.168.88.61", requestPort);
  Serial.print("Connection output: ");
  Serial.println(connection);
  if (connection)
  {
    ethClient.print("POST ");
    ethClient.print(requestFrom);
    ethClient.println(" HTTP/1.1");
    //ethClient.print("Host:  ");
    //ethClient.println(myIP);
    ethClient.println("User-Agent: Arduino/1.0");
    ethClient.println("Connection: close");
    ethClient.println("Content-Type: application/json;");
    ethClient.print("Content-Length: ");
    ethClient.println(postData.length());
    ethClient.println();
    ethClient.println(postData);
    Serial.println("Post sent!");
    //  Reading method (don't really know how to do it)
    Serial.println("Getting results...");
    output = ethClient.readString();
    Serial.print("Result: ");
    Serial.println(output);
    /*
    while (char c = ethClient.read())
    {
      output.concat(c);
    }
    */
  }
  return output;
}

/*
 *  void UnlockDoor (void);
 *
 *  Description:
 *  - Procedure to unlock the door
 */
void UnlockDoor (void)
{
  /*
   *  TODO
   */
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
  rfid_in.PCD_Init();
  rfid_out.PCD_Init();
  // Initializes the Ethernet module
  Serial.println("Initializing Ethernet module...");
  Ethernet.begin(mac);
  Serial.print("My IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("Ok!");
  // Prints messages indicating that all setup has been done
  Serial.println("All set!");
  Serial.println();
  // <<WARNING>> Remember to dump all read characters from keypad before asking for password!



  String postData = GenerateAuthenticatePostData ("12345", "senhasegura");
  Serial.println(postData);
  String output = SendPostRequest(postData, REQUEST_UNLOCK);
  Serial.println(output);




}

/*
 *  Loop
 */
void loop() 
{
  /*
  String tag = "";
  tag = ReadRFIDTag(rfid_in);
  while (tag == "")
  {
    Serial.println("Approach your card");
    delay(500);
  }
  Serial.print("Card tag: ");
  Serial.println(tag);
  String pw = GetPassword();
  Serial.print("Password: ");
  Serial.println(pw);
  String hashed = HashedPassword(pw);
  Serial.print("Hashed: ");
  Serial.println(hashed);
  */



  /*  Full code for Arduino Client (still in development) */
  

  /*
  // Reads tag until it's not NONE and check if the person is entering or leaving the room
  String tag = "";
  bool enteringRoom = false;
  while (tag == "")
  {
    tag = ReadRFIDTag(rfid_in);
    if (tag != "")
    {
      enteringRoom = true;
      break;
    }
    tag = ReadRFIDTag(rfid_out);
    if (tag != "")
    {
      enteringRoom = false;
      break;
    }
  }
  // Generates POST data, send request to server and waits for response
  bool authorized = false;
  // *** TODO ***
  // If authorized, check room's access level
  if (authorized == true)
    if (ROOM_ACCESS_LEVEL >= 3)
    {
      // If access level is higher or equal to 3, check if the person is leaving
      if (enteringRoom == false)
      {
        // If the person is leaving, unlocks the door
        UnlockDoor();
      }
      else
      {
        // If the person is entering, ask for password and crypt it with SHA-256 algorithm
        String password = HashedPassword(GetPassword());
        if (password != "")
        {
          // If the password isn't NONE, generates POST data, send request to server and waits for response
          bool passwordAccepted = false;
          // *** TODO ***
          if (passwordAccepted == true)
          {
            // If the password is OK, unlocks the door
            UnlockDoor();
          }
        }
      }
    }
    else
    {
      // If access level is lower or equal to 2, unlocks the door
      UnlockDoor();
    }
    */
}