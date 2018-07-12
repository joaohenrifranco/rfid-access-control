# RFID Access Control
Access control system developed to log each entrance and exit of internal rooms at a lab located at Federal University of Rio de Janeiro (UFRJ).

## Server
Django based server that provides a RESTful API for authentication and logging. Also provides a control panel made with Django Admin.

Code and more detailed info inside `server/`
## Client
Developed and tested for Arduino Mega using [PlatformIO](https://platformio.org/).

Works primarily with RFID readers, one for outside and one for inside, checking if the read UID is registered in server database and has sufficient privileges for unlocking the desired room.

Password authentication is optional through a 4x3 matrix keyboard, providing some kind of security.

Code and more detailed info inside `client/`

## Extras

### Front door "smart doorbell"
Developed in parallel with this project. Works with a RaspberryPi 3 located at the front door and a VoIP server (Asterisk) for live video transferring.

When doorbell is rang, the raspberry calls all people inside the lab, with some preconfigured order. They can take the call, check the camera and open the door if desired. All with their smartphone, PC or any other SIP-capable device.

Employees can automatically get in with their RFID tags registered in this system.

Also, everything is logged through this API.

For code and more info on that check the repositories below:

 - [Asterisk server](https://github.com/joaohenriquef/asterisk-smartdoor)
 - [Raspberry client](https://github.com/joaohenriquef/rasp-smartdoor)

## Disclaimer

Note that it is not made with security in mind and its main purpouse is internal logging, therefore it may be vulnerable to external attackers.


