from django.db import models
from .errors import *

ACCESS_LEVEL_CHOICES = (
	(0, 'Visitor'),
	(1, 'Level 1'),
	(2, 'Level 2'),
	(3, 'Level 3'),
	(4, 'Level 4'),
	(5, 'Level 5'),
)

class User(models.Model):
	name = models.CharField(max_length=200)
	access_level = models.IntegerField(choices=ACCESS_LEVEL_CHOICES, default='0')
	rfid_tag = models.CharField(max_length=8)
	hashed_password = models.CharField(max_length=64)

	# Necessary to show name correctly at DjangoAdmin
	def __str__(self):
		return self.name.title()

class Room(models.Model):
	name = models.CharField(max_length=15)
	description = models.TextField
	access_level = models.IntegerField(choices=ACCESS_LEVEL_CHOICES, default='5')
	
	# Necessary to show name correctly at DjangoAdmin
	def __str__(self):
		return self.name.title()

class Event(models.Model):
	EVENT_TYPE_CHOICES = (
    	(AUTHORIZED, 'Authorized'),
    	(RFID_NOT_FOUND, 'RFID Tag not found'),
		(INSUFFICIENT_PRIVILEGES, 'Insufficient privileges'),
		(WRONG_PASSWORD, 'Invalid password'),
		(PASSWORD_REQUIRED, 'Password required'),
		(VISITOR_RFID_FOUND, 'Visitor card indentified'),
		(VISITOR_RFID_NOT_FOUND, 'Visitor authorized'),
		(OPEN_DOOR_TIMEOUT, 'Open door timeout'),
		(UNKNOWN_ERROR, 'Unknown error'),
		(ROOM_NOT_FOUND, 'Room not found'),
		(LOGGING_ERROR, 'Logging error')
	)

	READER_POSITION_CHOICES = (
    	(0, 'outside'),
    	(1, 'inside'),
	)

	rfid = models.CharField(max_length=8, default=None, blank=True, null=True)
	user = models.ForeignKey(User, on_delete=models.PROTECT, default=None, blank=True, null=True)
	room = models.ForeignKey(Room, on_delete=models.PROTECT, default=None, blank=True, null=True)
	date = models.DateTimeField()
	event_type = models.IntegerField(choices=EVENT_TYPE_CHOICES, default=LOGGING_ERROR)
	reader_position = models.IntegerField(choices=READER_POSITION_CHOICES)

	def __str__(self):
		return (self.date.strftime("%Y-%m-%d %H:%M:%S") + ": " + self.get_event_type_display())

