/*
 *
 *  This code is being compiled for Arduino Pro Mini 16MHz
 *
 */

/*
 *  Libraries
 */
#include <SPI.h>
#include <stdio.h>
#include <string.h>
#include <MFRC522.h>
#include <Ethernet.h>
#include <Keypad.h>
#include <sha256.h>

/*
 *  Macros
 */
#define WHO_AM_I							"corredor"
#define MEASURE_NUMBERS						10
#define DOOR_TIMEOUT						15000
#define MAX_VISITOR_NUM						20
#define SERIAL_SPEED      					9600
#define MAC_ADDRESS       					{0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED} 	// Change MAC for individual modules
#define SERVER_IP         					{192, 168, 88, 64}
#define REQUEST_UNLOCK    					"/api/request-unlock"
#define AUTHENTICATE      					"/api/authenticate"
#define AUTHORIZE_VISITOR 					"/api/authorize-visitor"
#define REQUEST_PORT      					8000                  					//Standard HTTP port
#define KEYPAD_LINES      					4
#define KEYPAD_COLUMNS    					3
#define KEYS              					{{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}}
#define END_OF_PASSWORD   					'#'
#define QUIT_TYPING       					'*'
#define NUM_READERS       					2
byte BLACK [] =								{0, 0, 0};								//Turn LED off
byte BLUE [] =								{0, 0, HIGH};
byte GREEN [] =								{0, HIGH, 0};
byte AQUA [] =								{0, HIGH, HIGH};						//Light blue
byte RED [] =								{HIGH, 0, 0};
byte FUCHSIA [] =							{HIGH, 0, HIGH};						//Kinda purple
byte YELLOW [] =							{HIGH, HIGH, 0};
byte WHITE [] =								{HIGH, HIGH, HIGH};
#define WAITING_COLOR						YELLOW
#define OK_COLOR							GREEN
#define ERROR_COLOR							RED
#define STANDBY_COLOR						WHITE
#define DO_SOMETHING_COLOR					BLUE

/*
 *	Server Error Codes
 */
#define UNKNOWN_ERROR						-1
#define AUTHORIZED							0
#define RFID_NOT_FOUND						1
#define INSUFFICIENT_PRIVILEGES				2
#define WRONG_PASSWORD						3
#define PASSWORD_REQUIRED					4
#define VISITOR_RFID_FOUND					5
#define VISITOR_AUTHORIZED					6
#define VISITOR_RFID_NOT_FOUND				7
#define ROOM_NOT_FOUND						8
#define OPEN_DOOR_TIMEOUT					9

/*
 *  Pins
 */
#define LED_IN_R							A0
#define LED_IN_G							A1
#define LED_IN_B							A2
#define LED_OUT_R							A3
#define LED_OUT_G							A4
#define LED_OUT_B							A5
#define PIN_SENSOR							A6
#define PIN_BUZZER							A7
const byte SS_PIN_INSIDE = 					0;
const byte SS_PIN_OUTSIDE = 				1;
#define RST_PIN								2
#define KEYPAD_LIN_PINS						{3, 4, 5, 6}
#define KEYPAD_COL_PINS						{7, 8, 9}
#define SS_PIN_ETHERNET						10
#define MOSI_PIN							11
#define MISO_PIN							12
#define SCK_PIN								13

/*
 *  Declaring the RFID modules
 */
const byte ssPins [] = {SS_PIN_OUTSIDE, SS_PIN_INSIDE};
MFRC522 readers [NUM_READERS];

/*
 *  Declaring IP, MAC and the ethernet client itself
 */
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
 *  void WriteRGB (byte color[], char inside_outside);
 *
 *  Description:
 *  - Procedure that changes the inside or outside LED color
 *
 *  Inputs/Outputs:
 *  [INPUT] byte color[]: array that contatins red, green and blue values
 *
 *  Returns:
 *  -
 */
void WriteRGB (byte color[], char inside_outside)
{
	if (inside_outside == 'i')
	{
		byte pins[] = {LED_IN_R, LED_IN_G, LED_IN_B};
		for (byte i = 0; i < 3; i++)
			digitalWrite(pins[i], color[i]);
	}
	else if (inside_outside == 'o')
	{
		byte pins[] = {LED_OUT_R, LED_OUT_G, LED_OUT_B};
		for (byte i = 0; i < 3; i++)
			digitalWrite(pins[i], color[i]);
	}
}

/*
 *	void BlinkRGB (byte n_times, byte delay_time, byte blink_color [], byte end_color [], char action);
 *
 *  Description:
 *  - Blinks RGB LED in "action" from "blink_color" to "end_color" "n_times" times within a "delay_time" time
 *
 *  Inputs/Outputs:
 *  [INPUT] byte n_times: number of times the LED will blink
 * 	[INPUT] byte delay_time: time between blinks
 *  [INPUT] byte blink_color []: the LED color when blinking
 *  [INPUT] byte end_color []: the LED color when it ends
 *  [INPUT] char action: which LED should blink
 *
 *  Returns:
 *  -
 */
void BlinkRGB (byte n_times, byte delay_time, byte blink_color [], byte end_color [], char action)
{
	for (byte i = 0; i < n_times; i ++)
	{
		WriteRGB(blink_color, action);
		delay(delay_time);
		WriteRGB(end_color, action);
		delay(delay_time);
	}
}

/*
 *  String UID_toStr (byte *buffer, byte bufferSize);
 *
 *  Description:
 *  - Function that converts the UID read from the RFID module to hex string
 *
 *  Inputs/Outputs:
 *  [INPUT] byte *buffer: from RFID module
 * 	[INPUT] byte bufferSize: size of UID
 *
 *  Returns:
 *  [String] The UID tag itself
 */
String UID_toStr (byte *buffer, byte bufferSize)
{
	String aux = "";
  	for (byte i = 0; i < bufferSize; i++)
	{
		aux.concat(String(buffer[i] < 0x10 ? "0" : ""));
		aux.concat(String(buffer[i], HEX));
  	}
	return aux;
}

/*
 *  String ReadRFIDTags (char *entering_or_leaving);
 *
 *  Description:
 *  - Function to read UID tags from all readers
 *
 *  Inputs/Outputs:
 *  [OUTPUT] char *entering_or_leaving: indicates if the person is entering or leaving the room
 *
 *  Returns:
 *  [String] The UID tag itself in uppercase or empty string
 */
String ReadRFIDTags (char *entering_or_leaving)
{
	*entering_or_leaving = 255;
  	String aux = "";
  	for (byte i = 0; i < NUM_READERS; i++)
	{
		aux = "";
		if (readers[i].PICC_IsNewCardPresent() && readers[i].PICC_ReadCardSerial())
		{
			aux = UID_toStr(readers[i].uid.uidByte, readers[i].uid.size);
			*entering_or_leaving = i;
		}
		if (aux != "")
			return aux;
  	}
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
		BlinkRGB(1, 75, BLACK, DO_SOMETHING_COLOR, 'o');
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
 *  - Generates a JSON format text to send through HTTP POST to REQUEST_UNLOCK
 *
 *  Inputs/Outputs:
 *  [INPUT] String rfidTag: the RFID Tag read by RFID module
 *  [INPUT] byte roomId: the ID of the room where this client is
 *  [INPUT] byte action: indicates if the person is entering or leaving the room
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateUnlockPostData (String rfidTag, String roomId, byte action)
{
	String aux = "{\n\t\"rfidTag\":\"";
	aux.concat(rfidTag);
	aux.concat("\",\n\t\"roomId\":\"");
	aux.concat(roomId);
	aux.concat("\",\n\t\"action\":");
	aux.concat(String(action));
	aux.concat("\n}");
	return aux;
}

/*
 *  String GenerateAuthenticatePostData (String rfidTag, String password);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP POST to REQUEST_UNLOCK
 *
 *  Inputs/Outputs:
 *  [INPUT] String rfidTag: the RFID Tag read by RFID module
 *	[INPUT] String password: the user's read password
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateAuthenticatePostData (String rfidTag, String password)
{
	String aux = "{\n\t\"rfidTag\":\"";
	aux.concat(rfidTag);
	aux.concat("\",\n\t\"password\":\"");
	aux.concat(String(password));
	aux.concat("\"\n}");
	return aux;
}

/*
 *  String GenerateVisitorPostData (String rfidTag, String rfidTagsVisitors []);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP POST to AUTHORIZE_VISITOR
 *
 *  Inputs/Outputs:
 *  [INPUT] String rfidTag: an employee RFID
 *	[INPUT] String rfidTagsVisitors []: an array with all the visitors' RFIDs
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateVisitorPostData (String rfidTag, String rfidTagsVisitors [])
{

	String aux = "{\n\t\"rfidTag\":\"";
	aux.concat(rfidTag);
	aux.concat("\",\n\t\"rfidTagsVisitors\":[");
	for (byte i = 0; i < ((sizeof(rfidTagsVisitors) / sizeof(rfidTagsVisitors[0])) - 1); i++)
	{
		aux.concat("\n\t\t\"");
		aux.concat(rfidTagsVisitors[i]);
		aux.concat("\",");
	}
	aux.concat("\n\t\t\"");
	aux.concat(rfidTagsVisitors[(sizeof(rfidTagsVisitors) / sizeof(rfidTagsVisitors[0])) - 1]);
	aux.concat("\"\n\t]\n}");
	return aux;
}

/*
 *  String SendPostRequest (String postData, String requestFrom);
 *
 *  Description:
 *  - Does a POST Request
 *
 *  Inputs/Outputs:
 *  [INPUT] String postData: the JSON format POST data
 *  [INPUT] String requestFrom: the API URL
 *
 *  Returns:
 *  [String] The server's response
 */
String SendPostRequest(String postData, String requestFrom)
{
	String output = "";
	IPAddress serverIP(SERVER_IP);
	IPAddress myIP(Ethernet.localIP());
	int requestPort = REQUEST_PORT;
	int connection = ethClient.connect(serverIP, requestPort);
	Serial.print("Connection: ");
	Serial.println(connection);
	if (connection)
	{
		ethClient.print("POST ");
		ethClient.print(requestFrom);
		ethClient.println(" HTTP/1.1");
		ethClient.println("Content-Type: application/json;");
		ethClient.println("Connection: close");
		ethClient.println("User-Agent: Arduino/1.0");
		ethClient.print("Content-Length: ");
		ethClient.println(postData.length());
		ethClient.println();
		ethClient.println(postData);
		ethClient.println();
		output = ethClient.readString();
		Serial.print("out: ");
		Serial.println(output);
	}
	ethClient.stop();
	return output;
}

/*
 *  byte ParseResponse (String response);
 *
 *  Description:
 *  - Parse the server's response to return numeric status
 *
 *  Inputs/Outputs:
 *  [INPUT] String response: Server's response
 *
 *  Returns:
 *  [byte] Numeric error code from server
 */
byte ParseResponse (String response)
{
	byte status = 255;
	String status_str = "";
	char cResponse [200];
	response.toCharArray(cResponse, 200);
	char *teste = strstr(cResponse, "status");
	for (byte i = 0; i < strlen(teste); i++)
	{
		if (teste[i] == '}')
			break;
		if ((teste[i] >= '0') && (teste[i] <= '9'))
		{
			status_str.concat(teste[i]);
		}
	}
	//Serial.print("DEBUG: ");
	//Serial.println(status_str);
	if (status_str != "")
		status = status_str.toInt();
	return status;
}

/*
 *  bool BooleanMode (bool *array);
 *
 *  Description:
 *  - Returns the mode of a boolean array
 *
 *  Inputs/Outputs:
 *  [INPUT] bool *array: an array of booleans
 *
 *  Returns:
 *  [bool] The array's mode
 */
bool BooleanMode (bool *array)
{
	byte countTrue = 0;
	byte countFalse = 0;
	for (byte i = 0; i < MEASURE_NUMBERS; i++)
	{
		if (array[i] == true)
			countTrue++;
		else
			countFalse++;
	}
	return countTrue > countFalse;
}

/*
 *  bool DoorOpened ();
 *
 *  Description:
 *  - Checks if the door is opened
 *
 *  Inputs/Outputs:
 *  -
 *
 *  Returns:
 *  [bool] Is the door opened?
 */
bool DoorOpened ()
{
	bool measures [MEASURE_NUMBERS];
	for (byte i = 0; i < MEASURE_NUMBERS; i++)
	{
		measures[i] = digitalRead(PIN_SENSOR);
	}
	return BooleanMode(measures);
}

/*
 *	void Buzz (bool activate);
 *
 *  Description:
 *  - Switches buzzer on or off
 *
 *  Inputs/Outputs:
 *  [INPUT] bool activate: tells if it should be switched on or off
 *
 *  Returns:
 *  -
 */
void Buzz (bool activate)
{
	digitalWrite(PIN_BUZZER, activate);
}

/*
 *	void OperateBuzzer ();
 *
 *  Description:
 *  - Operates buzzer according to the door state
 *
 *  Inputs/Outputs:
 *  -
 *
 *  Returns:
 *  -
 */
void OperateBuzzer ()
{
	bool opened = DoorOpened();
	if (opened == false)
	{
		Buzz(false);
		WriteRGB(DO_SOMETHING_COLOR, 'i');
	}
	else
	{
		long initial_timer = millis();
		while (opened == true)
		{
			if (millis() >= initial_timer + DOOR_TIMEOUT)
			{
				Buzz(true);
				WriteRGB(RED, 'i');
			}
			else
			{
				Buzz(false);
				WriteRGB(YELLOW, 'i');
			}
			opened = DoorOpened();
		}
	}
}

/*
 *  void UnlockDoor (void);
 *
 *  Description:
 *  - Procedure to unlock the door
 */
void UnlockDoor (void)
{
	//	TODO
}

/*
 *  Setup
 */
void setup() 
{
	// Starts serial communication for debugging purposes
	Serial.begin(SERIAL_SPEED);
	Serial.println();
	Serial.println("=== Beginning Setup...");
	// Starts SPI communication for devices
	Serial.println();
	Serial.println("-- Starting SPI...");
	SPI.begin();
	// Initializes LED pins as output
	Serial.println();
	Serial.println("-- Setting LEDs pins as output...");
	pinMode(LED_IN_R, OUTPUT);
	pinMode(LED_IN_G, OUTPUT);
	pinMode(LED_IN_B, OUTPUT);
	pinMode(LED_OUT_R, OUTPUT);
	pinMode(LED_OUT_G, OUTPUT);
	pinMode(LED_OUT_B, OUTPUT);
	// Initializes the RFID modules
	Serial.println();
	Serial.println("-- Initializing RFID modules...");
	for (byte i = 0; i < NUM_READERS; i++)
	{
		readers[i].PCD_Init(ssPins[i], RST_PIN);
	Serial.print("Reader ");
	Serial.print(i + 1);
		Serial.print(" initialized!\tVersion: ");
		readers[i].PCD_DumpVersionToSerial();
	}	
	// Initializes the Ethernet module
	Serial.println();
	Serial.println("-- Initializing Ethernet module...");
	Ethernet.begin(mac);
	Serial.print("- My IP: ");
	Serial.println(Ethernet.localIP());

	// Initializes the sensor
	Serial.println();
	Serial.println("-- Setting sensor pin as input...");
	pinMode(PIN_SENSOR, INPUT);

	// Initializes the buzzer
	Serial.println();
	Serial.println("-- Setting buzzer pin as output...");
	pinMode(PIN_BUZZER, OUTPUT);
}

/*
 *  Loop
 */
void loop() 
{
	// char entering_or_leaving = 'i';
	// String tag = "";
	// tag = ReadRFIDTags(&entering_or_leaving);
	// while (tag == "")
	// {
	// 	Serial.println("Approach your card");
	// 	delay(500);
	// 	tag = ReadRFIDTags(&entering_or_leaving);
	// }
	// Serial.print("Card tag: ");
	// Serial.println(tag);

	/*  Full code for Arduino Client (still in development) */
	String pw = "";
	String tag = "";
	byte status = 255;
	String hashed = "";
	String output = "";
	char action = 'z';
	char not_action = 'z';
	String postData = "";
	String tagsArray [MAX_VISITOR_NUM];
	char entering_or_leaving = 255; //0 (ZERO) indicates entering and 1 (ONE) indicates leaving

	// Resets the tagsArray
	for (byte i = 0; i < MAX_VISITOR_NUM; i++)
		tagsArray[i] = "";

	// Standby mode
	WriteRGB(STANDBY_COLOR, 'i');
	WriteRGB(STANDBY_COLOR, 'o');
	// Starts to read
	Serial.println("=== Starting to read...");
	tag = ReadRFIDTags(&entering_or_leaving);
	while (tag == "")
	{
		Serial.println("-- Reading...");
		delay (50);
		tag = ReadRFIDTags(&entering_or_leaving);
	}
	Serial.print("UID Tag: ");
	Serial.println(tag);
	// Found an UID. Turns one side to WAITING_MODE and the other to BLOCKED_MODE
	if (entering_or_leaving == 0)
	{
		action = 'o';
		not_action = 'i';
		Serial.println("-- User in ENTERING the room");
	}
	else
	{
		action = 'i';
		not_action = 'o';
		Serial.println("-- User in LEAVING the room");
	}
	WriteRGB(WAITING_COLOR, action);
	WriteRGB(ERROR_COLOR, not_action);
	// Generates POST data
	Serial.println("-- Generating POST data...");
	postData = GenerateUnlockPostData (tag, WHO_AM_I, entering_or_leaving);
	Serial.println(postData);
	// Sends request to REQUEST_UNLOCK and gets response
	output = SendPostRequest(postData, REQUEST_UNLOCK);
	Serial.print ("-- Server response: ");
	Serial.println(output);
	// Parse response's status
	status = ParseResponse(output);
	Serial.print("-- Status: ");
	Serial.println(status);
	// If already authorized, unlocks door
	if (status == AUTHORIZED)
	{
		UnlockDoor();
		WriteRGB(OK_COLOR, action);
		//
		//
		//	REMOVE DELAY (or reduce it)
		//
		//
		delay(5000);
	}
	// If needs password, blinks OK_COLOR and asks for typing
	else if (status == PASSWORD_REQUIRED)
	{
		// Blinks DO_SOMETHING_COLOR
		BlinkRGB(2, 250, BLACK, DO_SOMETHING_COLOR, action);
		Serial.println("-- Waiting for password...");
		// Gets password
		pw = GetPassword();
		Serial.print("-- Password: ");
		Serial.println(pw);
		// Blinks WAITING_COLOR once password is read
		BlinkRGB(2, 250, BLACK, WAITING_COLOR, action);
		// Hashes password
		Serial.println("-- Hashing password...");
		hashed = HashedPassword(pw);
		Serial.print("-- Hashed password (SHA-256): ");
		Serial.println(hashed);
		// Generates POST data for AUTHENTICATE API
		Serial.println("-- Generating POST data...");
		postData = GenerateAuthenticatePostData(tag, hashed);
		Serial.println(postData);
		// Sends POST data to AUTHENTICATE API
		output = SendPostRequest(postData, AUTHENTICATE);
		Serial.print ("-- Server response: ");
		Serial.println(output);
		// Parse response's status
		status = ParseResponse(output);
		Serial.print("-- Status: ");
		Serial.println(status);
		// If authorized
		if (status == AUTHORIZED)
		{
			// Checks if there's any visitor on tagsArray
			if (tagsArray[0] == "")
			{
				UnlockDoor();
				WriteRGB(OK_COLOR, action);
				//
				//
				//	REMOVE DELAY (or reduce it)
				//
				//
				delay(5000);
			}
			// If there are visitor tags
			else
			{
				//
				//	TODO: Generates POST for AUTHORIZE_VISITOR, gets response and unlocks door.
				//
			}
		}
		else
		{
			WriteRGB(ERROR_COLOR, action);
			delay (5000);
		}
	}
	else if (status == VISITOR_RFID_FOUND)
	{
		//
		// TODO: Appends visitors' UID to tagsArray until UID belongs to employee.
		//
	}
	else
	{
		WriteRGB(ERROR_COLOR, action);
		delay (5000);
	}
	//while(true);
}