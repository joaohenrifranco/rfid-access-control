from django.db import models

class User(models.Model):
	name = models.CharField(max_length=200)
	access_level = models.PositiveIntegerField
	rfid_tag = models.CharField(max_length=8)
	hashed_password = models.CharField(max_length=40)


class Event(models.Model):
	EVENT_TYPE_CHOICES = (
    	('0', 'authorized'),
    	('1', 'tag not found')
		('2', 'invalid password')
		('3', 'insufficient privileges')
	)

	READER_POSITION_CHOICES = (
    	('0', 'outside'),
    	('1', 'inside')
	)

	user = models.ForeignKey(User, on_delete=models.PROTECT)
	room = models.ForeignKey(Room, on_delete=models.PROTECT)
	date = models.DateTimeField('event occurred')
	event_type = models.CharField(max_length=50, choices=EVENT_TYPE_CHOICES)
	reader_position = models.CharField(max_length=7, choices=READER_POSITION_CHOICES)

class Room(models.Model):
	name = models.CharField(max_length=200)
	access_level = models.PositiveIntegerField
