/*
 *  This code is compatible with Arduino MEGA 2560 R3
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
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>

/*
 *  Macros
 */
#define WHO_AM_I              					"ENSAIOS_REP"
#define MEASURE_NUMBERS       					10
#define MAX_VISITOR_NUM       					20
#define SERIAL_SPEED          					9600
#define MAC_ADDRESS       						{0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02} 	// Change MAC for individual modules
#define SERVER_IP         						{192, 168, 88, 41}
#define REQUEST_UNLOCK    						"/api/request-unlock"
#define AUTHENTICATE      						"/api/authenticate"
#define AUTHORIZE_VISITOR 						"/api/authorize-visitor"
#define REQUEST_PORT      						80                 						//Standard HTTP port
#define KEYPAD_LINES      						4
#define KEYPAD_COLUMNS    						3
#define KEYS              						{{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}}
#define END_OF_PASSWORD   						'#'
#define QUIT_TYPING       						'*'
#define NUM_READERS       						2
byte BLACK [] =									{0, 0, 0};								//Turn LED off
byte BLUE [] =									{0, 0, HIGH};
byte GREEN [] =									{0, HIGH, 0};
byte AQUA [] =									{0, HIGH, HIGH};						//Light blue
byte RED [] =									{HIGH, 0, 0};
byte FUCHSIA [] =								{HIGH, 0, HIGH};						//Kinda purple
byte YELLOW [] =								{HIGH, HIGH, 0};
byte WHITE [] =									{HIGH, HIGH, HIGH};
#define WAITING_COLOR							YELLOW
#define OK_COLOR								GREEN
#define ERROR_COLOR								RED
#define STANDBY_COLOR							WHITE
#define DO_SOMETHING_COLOR						BLUE
#define DOOR_UNLOCK_TIME						2000
#define TIMEOUT_VISITOR							20000
#define TIMEOUT_DOOR							60000
#define TIMEOUT_PASSWORD						15000

/*
 *	Server Error Codes
 */
#define UNKNOWN_ERROR							-1
#define AUTHORIZED								0
#define RFID_NOT_FOUND							1
#define INSUFFICIENT_PRIVILEGES					2
#define WRONG_PASSWORD							3
#define PASSWORD_REQUIRED						4
#define VISITOR_RFID_FOUND						5
#define VISITOR_AUTHORIZED						6
#define VISITOR_RFID_NOT_FOUND					7
#define ROOM_NOT_FOUND							8
#define OPEN_DOOR_TIMEOUT						9

/*
 *  Pins
 */
#define LED_IN_R								36
#define LED_IN_G								38
#define LED_IN_B								40
#define LED_OUT_R								42
#define LED_OUT_G								44
#define LED_OUT_B								46
#define PIN_SENSOR								48
#define PIN_BUZZER								26
#define DOOR_PIN								28
#define RST_PIN									30
#define KEYPAD_LIN_PINS							{37, 39, 41, 43}
#define KEYPAD_COL_PINS							{45, 47, 49}

// Macros for reference only
#define MISO_PIN								50
#define MOSI_PIN								51
#define SCK_PIN									52

//	SS pins
const byte SS_PIN_ETHERNET = 					10;
const byte SS_PIN_OUTSIDE	=					22;
const byte SS_PIN_INSIDE	= 					24;

//	Redefining Ethernet SS pin
#ifdef ETHERNET_SHIELD_SPI_CS
	#undef ETHERNET_SHIELD_SPI_CS
#endif
#define ETHERNET_SHIELD_SPI_CS SS_PIN_ETHERNET

/*
 *  Declaring the RFID modules
 */
const byte ssPins [] = {SS_PIN_OUTSIDE, SS_PIN_INSIDE};
MFRC522 readers [NUM_READERS];
bool readers_locked [2] = {false, false};
char readers_id [2] = {'o', 'i'};

/*
 *  Declaring IP, MAC and the ethernet client itself
 */
// TODO: FIX AUTOMATIC MAC, FOLLOWING COMMENTED LINES CRASH ETHERNET
// #if defined(WIZ550io_WITH_MACADDRESS)
// ;
// #else
byte mac[] = MAC_ADDRESS;
EthernetClient ethClient;
IPAddress serverIP(SERVER_IP);
HttpClient httpClient = HttpClient(ethClient, serverIP, REQUEST_PORT);


/*
 *  Declaring and initializing the keypad (4x3)
 */
char keys[KEYPAD_LINES][KEYPAD_COLUMNS] = KEYS;
byte keyPadLinPins[KEYPAD_LINES] = KEYPAD_LIN_PINS;
byte keyPadColPins[KEYPAD_COLUMNS] = KEYPAD_COL_PINS;
Keypad keyPad = Keypad(makeKeymap(keys), keyPadLinPins, keyPadColPins, KEYPAD_LINES, KEYPAD_COLUMNS);

/*
 *	Global vars for visitors
 */
byte visitor_counter = 0;
String tagsArray [MAX_VISITOR_NUM];
unsigned long visitorInitTime = 0;

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
 *	Write function header
 */
void WriteReaderLED (byte color[])
{
	for (byte i = 0; i < 2; i++)
	{
		if (readers_locked[i] == true)
		{
			WriteRGB(ERROR_COLOR, readers_id[i]);
		}
		else
		{
			WriteRGB(color, readers_id[i]);
		}
	}
}

/*
 *	void BlinkRGB (byte n_times, byte delay_time, byte blink_color [], byte end_color [], char readerPosition);
 *
 *  Description:
 *  - Blinks RGB LED in "readerPosition" from "blink_color" to "end_color" "n_times" times within a "delay_time" time
 *
 *  Inputs/Outputs:
 *  [INPUT] byte n_times: number of times the LED will blink
 * 	[INPUT] byte delay_time: time between blinks
 *  [INPUT] byte blink_color []: the LED color when blinking
 *  [INPUT] byte end_color []: the LED color when it ends
 *  [INPUT] char readerPosition: which LED should blink
 *
 *  Returns:
 *  -
 */
void BlinkRGB (byte n_times, byte delay_time, byte blink_color [], byte end_color [], char readerPosition)
{
	for (byte i = 0; i < n_times; i ++)
	{
		WriteReaderLED(blink_color);
		delay(delay_time);
		WriteReaderLED(end_color);
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
 *	Build description (TODO)
 */
void BlinkBuzzer (byte n_times, byte delay_time)
{
	for (byte i = 0; i < n_times; i ++)
	{
		Buzz(true);
		delay(delay_time);
		Buzz(false);
		delay(delay_time);
	}
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
	digitalWrite(SS_PIN_ETHERNET, HIGH);
	*entering_or_leaving = 255;
  	String aux = "";
  	for (byte i = 0; i < NUM_READERS; i++)
	{
		if (readers_locked[i] == false)
		{
			digitalWrite(SS_PIN_INSIDE, HIGH);
			digitalWrite(SS_PIN_OUTSIDE, HIGH);
			digitalWrite(ssPins[i], LOW);
			aux = "";
			if (readers[i].PICC_IsNewCardPresent() && readers[i].PICC_ReadCardSerial())
			{
				aux = UID_toStr(readers[i].uid.uidByte, readers[i].uid.size);
				BlinkBuzzer(1, 10);
				*entering_or_leaving = i;
			}
			if (aux != "")
				return aux;
		}
  	}
	return aux;
}

/*
 *	Build description (TODO)
 */
void BuzzTimer (byte delayTime)
{
	Buzz(true);
	delay(delayTime);
	Buzz(false);
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
	unsigned long initial_timer = millis();
	while (c != END_OF_PASSWORD)
	{
		if (millis() >= initial_timer + TIMEOUT_PASSWORD)
		{
			return "";
		}
		if (c == QUIT_TYPING)
		{
			return "";
		}
		if (c)
		{
			BlinkRGB(1, 75, BLACK, DO_SOMETHING_COLOR, 'o');
			BlinkBuzzer(1, 50);
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
 *  String GenerateUnlockPostData (String uid, byte roomID, byte readerPosition);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP POST to REQUEST_UNLOCK
 *
 *  Inputs/Outputs:
 *  [INPUT] String uid: the RFID Tag read by RFID module
 *  [INPUT] byte roomID: the ID of the room where this client is
 *  [INPUT] byte readerPosition: indicates if the person is entering or leaving the room
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateUnlockPostData (String uid, String roomID, byte readerPosition)
{
	String aux = "{\n\t\"uid\":\"";
	aux.concat(uid);
	aux.concat("\",\n\t\"roomID\":\"");
	aux.concat(roomID);
	aux.concat("\",\n\t\"readerPosition\":");
	aux.concat(String(readerPosition));
	aux.concat("\n}");
	return aux;
}

/*
 *  String GenerateAuthenticatePostData (String uid, String password, String roomID);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP POST to REQUEST_UNLOCK
 *
 *  Inputs/Outputs:
 *  [INPUT] String uid: the RFID Tag read by RFID module
 *	[INPUT] String password: the user's read password
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateAuthenticatePostData (String uid, String password, String roomID)
{
	String aux = "{\n\t\"uid\":\"";
	aux.concat(uid);
	aux.concat("\",\n\t\"password\":\"");
	aux.concat(password);
	aux.concat("\",\n\t\"roomID\":\"");
	aux.concat(roomID);
	aux.concat("\"\n}");
	return aux;
}

/*
 *  String GenerateVisitorPostData (String uid, String visitorsUids []);
 *
 *  Description:
 *  - Generates a JSON format text to send through HTTP POST to AUTHORIZE_VISITOR
 *
 *  Inputs/Outputs:
 *  [INPUT] String uid: an employee RFID
 *	[INPUT] String visitorsUids []: an array with all the visitors' RFIDs
 *
 *  Returns:
 *  [String] A JSON format text contatining the whole input data
 */
String GenerateVisitorPostData (String uid, String visitorsUids [], String roomID)
{
	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["uid"] = uid;
	root["roomID"] = roomID;

	JsonArray& VisUID = root.createNestedArray("visitorsUids");
	for (byte i = 0; i < visitor_counter; i++)
	{
		VisUID.add(visitorsUids[i]);
	}

	String output;
	root.printTo(output);
	return output;
}

/*
 *  byte SendPostRequest (String postData, String requestFrom);
 *
 *  Description:
 *  - Does a POST Request
 *
 *  Inputs/Outputs:
 *  [INPUT] String postData: the JSON format POST data
 *  [INPUT] String requestFrom: the API URL
 *
 *  Returns:
 *  [byte] The server's response status
 */
byte SendPostRequest(String postData, String requestFrom)
{
	digitalWrite(SS_PIN_ETHERNET, LOW);
	digitalWrite(SS_PIN_OUTSIDE, HIGH);
	digitalWrite(SS_PIN_INSIDE, HIGH);
	String response = "";
	byte output = 255;
	String contentType = "application/json";
	Serial.println("Sending post...");
	httpClient.post(requestFrom, contentType, postData);
	Serial.println("POST done!");
	response = httpClient.responseBody();
	Serial.print("Response: ");
	Serial.println(response);
	const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
	DynamicJsonBuffer jsonBuffer(capacity);
	JsonObject& root = jsonBuffer.parseObject(response);
	if (!root.success()) {
		Serial.println(F("Parsing failed!"));
		return 255;
	}
	output = root["status"];
	httpClient.endRequest();
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
	StaticJsonBuffer<500> jsonBuffer;
	JsonObject& root = jsonBuffer.parseObject(response);
	if (!root.success())
	{
		Serial.println("Parsing response failed!");
		return 255;
	}
	else
	{
		status = root["status"];
		return status;
	}
	// byte status = 255;
	// String status_str = "";
	// char cResponse [200];
	// response.toCharArray(cResponse, 200);
	// char *teste = strstr(cResponse, "status");
	// for (byte i = 0; i < strlen(teste); i++)
	// {
	// 	if (teste[i] == '}')
	// 		break;
	// 	if ((teste[i] >= '0') && (teste[i] <= '9'))
	// 	{
	// 		status_str.concat(teste[i]);
	// 	}
	// }
	// //Serial.print("DEBUG: ");
	// //Serial.println(status_str);
	// if (status_str != "")
	// 	status = status_str.toInt();
	// return status;
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
  *	Write header
  */
void ResetStatus (void)
{
	readers_locked[0] = false;
	readers_locked[1] = false;
	visitor_counter = 0;
	loop();
}

/*
 *	Write header
 */
void CheckVisitorTimeout (void)
{
	Serial.print ("Time: ");
	Serial.println(millis() - visitorInitTime);
	if ((millis() - visitorInitTime) >= TIMEOUT_VISITOR)
	{
		ResetStatus();
		Serial.println("Checou timeout");
	}
}

/*
 *	Write header
 */
void CheckDoorTimeout (void)
{
	bool opened = DoorOpened();
	unsigned long timer = millis();
	bool previous_lock [2] = {readers_locked[0], readers_locked[1]};
	while (opened)
	{
		Serial.println ("=== PORTA ABERTA! ===");
		readers_locked[0] = true;
		readers_locked[1] = true;
		WriteReaderLED(ERROR_COLOR);
		if (millis() >= timer + TIMEOUT_DOOR)
		{
			Buzz(true);
		}
		opened = DoorOpened();
	}
	digitalWrite(DOOR_PIN, HIGH);
	WriteReaderLED(STANDBY_COLOR);
	Serial.println("- Porta fechada...");
	Buzz(false);
	readers_locked [0] = previous_lock [0];
	readers_locked [1] = previous_lock [1];
}

/*
 *  void UnlockDoor (void);
 *
 *  Description:
 *  - Procedure to unlock the door
 */
void UnlockDoor (void)
{
	digitalWrite(DOOR_PIN, LOW);
	delay(DOOR_UNLOCK_TIME);
	digitalWrite(DOOR_PIN, HIGH);
	ResetStatus();
}

/*
 *	Write header
 */
void ErrorExit (void)
{
	WriteReaderLED(ERROR_COLOR);
	BlinkBuzzer(3, 50);
	BuzzTimer(200);
	ResetStatus();
	delay(1000);
}

/*
 *  Setup
 */
void setup() 
{

	// Short delay in order to fix reset issues
	delay(1000);


	// Starts serial communication for debugging purposes
	
	Serial.begin(SERIAL_SPEED);
	
	Serial.println("=== Beginning Setup...");
	Serial.println("-- Setting SPI SS pins...");
	
	pinMode(SS_PIN_INSIDE, OUTPUT);
	pinMode(SS_PIN_OUTSIDE, OUTPUT);
	
	Serial.println("-- Setting LEDs pins as output...");

	pinMode(LED_IN_R, OUTPUT);
	pinMode(LED_IN_G, OUTPUT);
	pinMode(LED_IN_B, OUTPUT);
	pinMode(LED_OUT_R, OUTPUT);
	pinMode(LED_OUT_G, OUTPUT);
	pinMode(LED_OUT_B, OUTPUT);
	pinMode(DOOR_PIN, OUTPUT);
	digitalWrite(DOOR_PIN, HIGH);

	Serial.println("-- Initializing RFID modules...");
	
	SPI.begin();
	for (byte i = 0; i < NUM_READERS; i++)
	{
		digitalWrite(SS_PIN_INSIDE, HIGH);
		digitalWrite(SS_PIN_OUTSIDE, HIGH);
		digitalWrite(ssPins[i], LOW);
		readers[i].PCD_Init(ssPins[i], RST_PIN);
		Serial.print("-- Reader ");
		Serial.print(i + 1);
		
		Serial.print(" initialized!\n- Version: ");
		readers[i].PCD_DumpVersionToSerial();
	}	

	Serial.println("-- Initializing Ethernet module...");

	for (byte i = 0; i < NUM_READERS; i++) {
		digitalWrite(ssPins[i], HIGH);
	}
  
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		while(true); // Freezes here if ethernet setup fails
  	}

	Serial.print("- My MAC: ");
	Serial.print(mac[0], HEX); Serial.print(":"); Serial.print(mac[1], HEX); Serial.print(":");
	Serial.print(mac[2], HEX); Serial.print(":"); Serial.print(mac[3], HEX); Serial.print(":");
	Serial.print(mac[4], HEX); Serial.print(":"); Serial.print(mac[5], HEX); Serial.println();
	Serial.print("- My IP: ");
	Serial.println(Ethernet.localIP());

	// Initializes the sensor
	Serial.println("-- Setting sensor pin as input...");
	pinMode(PIN_SENSOR, INPUT);
	// Initializes the buzzer
	Serial.println("-- Setting buzzer pin as output...");
	pinMode(PIN_BUZZER, OUTPUT);
	WriteReaderLED(STANDBY_COLOR);
}

/*
 *  Loop
 */
void loop() 
{
	/*  Full code for Arduino Client (still in development) */
	String pw = "";
	String tag = "";
	byte status = 255;
	String hashed = "";
	String output = "";
	byte readerPosition = 255;
	String postData = "";
	String employeeTag = "";
	char entering_or_leaving = 255; //0 (ZERO) indicates entering and 1 (ONE) indicates leaving

	CheckDoorTimeout();

	// Standby mode
	if (visitor_counter == 0)
		WriteReaderLED(STANDBY_COLOR);
	else
	{
		WriteReaderLED(DO_SOMETHING_COLOR);
	}

	// Starts to read
	Serial.println("=== Starting to read...");
	tag = ReadRFIDTags(&entering_or_leaving);
	while (tag == "")
	{
		Serial.println("-- Reading...");
		if (visitor_counter > 0)
			CheckVisitorTimeout();
		delay (50);
		tag = ReadRFIDTags(&entering_or_leaving);
	}
	Serial.print("UID Tag: ");
	Serial.println(tag);
	// Found an UID. Turns one side to WAITING_MODE and the other to BLOCKED_MODE
	if (entering_or_leaving == 0) readers_locked[1] = true;
	else readers_locked[0] = true;
	WriteReaderLED(WAITING_COLOR);
	// Generates POST data
	Serial.println("-- Generating POST data...");
	postData = GenerateUnlockPostData (tag, WHO_AM_I, entering_or_leaving);
	Serial.println(postData);
	// Sends request to REQUEST_UNLOCK and gets response
	status = SendPostRequest(postData, REQUEST_UNLOCK);
	Serial.print("-- Status: ");
	Serial.println(status);
	// If already authorized, unlocks door
	if (status == AUTHORIZED)
	{
		WriteReaderLED(OK_COLOR);
		UnlockDoor();
	}
	// If needs password, blinks OK_COLOR and asks for typing
	else if (status == PASSWORD_REQUIRED)
	{
		// Copies read tag to employee tag
		employeeTag = tag;
		// Blinks DO_SOMETHING_COLOR
		BlinkRGB(2, 250, BLACK, DO_SOMETHING_COLOR, readerPosition);
		Serial.println("-- Waiting for password...");
		// Gets password
		pw = GetPassword();
		Serial.print("-- Password: ");
		Serial.println(pw);
		if (pw == "")
		{
			ErrorExit();
			return;
		}
		// Blinks WAITING_COLOR once password is read
		BlinkRGB(2, 250, BLACK, WAITING_COLOR, readerPosition);
		// Hashes password
		Serial.println("-- Hashing password...");
		hashed = HashedPassword(pw);
		Serial.print("-- Hashed password (SHA-256): ");
		Serial.println(hashed);
		// Generates POST data for AUTHENTICATE API
		Serial.println("-- Generating POST data...");
		postData = GenerateAuthenticatePostData(tag, hashed, WHO_AM_I);
		Serial.println(postData);
		// Sends POST data to AUTHENTICATE API
		status = SendPostRequest(postData, AUTHENTICATE);
		Serial.print("-- Status: ");
		Serial.println(status);
		// If authorized
		if (status == AUTHORIZED)
		{
			Serial.print("Numero do contador: ");
			Serial.println(visitor_counter);
			Serial.print("Array: ");
			for (byte j = 0; j < visitor_counter; j++)
			{
				Serial.println(j);
				Serial.println(tagsArray[j]);
			}
			// Checks if there's any visitor on tagsArray
			if (visitor_counter == 0)
			{
				WriteReaderLED(OK_COLOR);
				UnlockDoor();
			}
			// If there are visitor tags
			else
			{
				//	Generating POST data for visitors
				Serial.println("-- Generating Visitor POST data...");
				postData = GenerateVisitorPostData (employeeTag, tagsArray, WHO_AM_I);
				Serial.println(postData);
				//	Sending POST to visitors API
				status = SendPostRequest(postData, AUTHORIZE_VISITOR);
				Serial.print("-- Status: ");
				Serial.println(status);
				if (status == VISITOR_AUTHORIZED)
				{
					WriteReaderLED(OK_COLOR);
					UnlockDoor();
				}
				else
				{
					ErrorExit();
				}
			}
		}
		else
		{
			ErrorExit();
		}
	}
	else if (status == VISITOR_RFID_FOUND)
	{
		if (visitor_counter < MAX_VISITOR_NUM)
		{
			visitorInitTime = millis();
			Serial.println("Registrando visitante...");
			tagsArray[visitor_counter] = tag;
			Serial.println(tagsArray[visitor_counter]);
			visitor_counter++;
		}
	}
	else
	{
		ErrorExit();
	}
}