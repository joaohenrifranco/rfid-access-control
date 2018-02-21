from django.db import models

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
	hashed_password = models.CharField(max_length=40)

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
    	('0', 'authorized'),
    	('1', 'tag not found'),
		('2', 'invalid password'),
		('3', 'insufficient privileges'),
		('4', 'timeout')
	)

	READER_POSITION_CHOICES = (
    	('0', 'outside'),
    	('1', 'inside'),
	)

	user = models.ForeignKey(User, on_delete=models.PROTECT)
	room = models.ForeignKey(Room, on_delete=models.PROTECT)
	date = models.DateTimeField('event occurred')
	event_type = models.CharField(max_length=50, choices=EVENT_TYPE_CHOICES)
	reader_position = models.CharField(max_length=7, choices=READER_POSITION_CHOICES)
