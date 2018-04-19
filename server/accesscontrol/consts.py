## Constants used throughout all the django server code

# Error codes used in the 'status' flag sent in POST response and database logging
UNEXPECTED_ERROR = -1
AUTHORIZED = 0
UNREGISTERED_RFID = 1
INSUFFICIENT_PRIVILEGES = 2
WRONG_PASSWORD = 3
PASSWORD_REQUIRED = 4
VISITOR_RFID_FOUND = 5
VISITOR_AUTHORIZED = 6
UNREGISTERED_VISITOR_RFID = 7
ROOM_NOT_FOUND = 8
OPEN_DOOR_TIMEOUT = 9
FRONT_DOOR_OPENED = 10
UNREGISTERED_SIP = 11
LOGGING_ERROR = 99

# Used to store in the database which API triggered the log; for debugging purposes.
UNLOCK_API = 0
AUTH_API = 1
VISITOR_API = 2
FRONT_DOOR_API = 3

# Rooms with this level or greater will also need password authentication
REQUIRE_PASSWORD_LEVEL_THRESHOLD = 3