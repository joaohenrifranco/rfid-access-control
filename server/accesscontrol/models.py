from django.db import models
from .errors import *
from django.contrib.auth.models import AbstractBaseUser, BaseUserManager
from django.db.models.signals import pre_save
from .services import *

ACCESS_LEVEL_CHOICES = (
  (0, 'Visitor'),
  (1, 'Level 1'),
  (2, 'Level 2'),
  (3, 'Level 3'),
  (4, 'Level 4'),
  (5, 'Level 5'),
)

class RfidTag(models.Model):
  rfid_tag_id = models.CharField(max_length=8, default=None, unique=True)
  created = models.DateTimeField(auto_now_add=True)
  def __str__(self):
    return self.rfid_tag_id.upper()

class UserManager(BaseUserManager):
  def create_user(self, email, date_added=None, password=None):
    if not email:
      raise ValueError('Users must have an email address')
    user = self.model(email=self.normalize_email(email),)
    user.set_password(password)
    user.save(using=self._db)
    return user

class User(AbstractBaseUser):
  email = models.EmailField(max_length=255, unique=True)
  first_name = models.CharField(max_length=200)
  last_name = models.CharField(max_length=200)
  cpf = models.CharField(max_length=11, unique=True)
  access_level = models.IntegerField(choices=ACCESS_LEVEL_CHOICES, default='0')
  rfid_tag = models.ManyToManyField(RfidTag, through='RfidTagUserLink', blank=True)
  created = models.DateTimeField(auto_now_add=True)
  
  objects = UserManager()

  USERNAME_FIELD = 'email'

  def get_full_name(self):
    return "%s %s" % (self.first_name, self.last_name)

  def get_short_name(self):
    return self.first_name

  def __str__(self):
    return "%s %s" % (self.first_name, self.last_name)

class RfidTagUserLink(models.Model):
  rfid_tag = models.ForeignKey(RfidTag, on_delete=models.PROTECT, default=None)
  user = models.ForeignKey(User, on_delete=models.PROTECT, default=None)
  created = models.DateTimeField(auto_now_add=True)
  expire_date = models.DateTimeField(null=True, blank=True)
  
  def __str__(self):
    return (self.rfid_tag.rfid_tag_id + " - " + self.user.get_full_name())

  def rfid_tag_user_link_pre_save(sender, instance, created, *args, **kwargs):
    if created:
      print("ddd")

pre_save.connect(RfidTagUserLink.rfid_tag_user_link_pre_save, RfidTagUserLink, dispatch_uid=".models.RfidTagUserLink")

class Room(models.Model):
  name = models.CharField(max_length=15)
  description = models.TextField(null=True, blank=True)
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
    (VISITOR_AUTHORIZED, 'Visitor authorized'),
    (VISITOR_RFID_NOT_FOUND, 'Visitor RFID not found'),
    (OPEN_DOOR_TIMEOUT, 'Open door timeout'),
    (UNKNOWN_ERROR, 'Unknown error'),
    (ROOM_NOT_FOUND, 'Room not found'),
  )

  READER_POSITION_CHOICES = (
    (0, 'Outside'),
    (1, 'Inside'),
  )

  API_MODULE_CHOICES = (
    (AUTH_API, '/api/authenticate/'),
    (UNLOCK_API, '/api/request-unlock/'),
    (VISITOR_API, '/api/authorize-visitor')
  )

  user = models.ForeignKey(User, on_delete=models.PROTECT, default=None, blank=True, null=True, related_name='events')
  
  event_type = models.IntegerField(choices=EVENT_TYPE_CHOICES, default=LOGGING_ERROR)
  reader_position = models.IntegerField(choices=READER_POSITION_CHOICES)
  api_module = models.IntegerField(choices=API_MODULE_CHOICES, default=None)
  rfid_tag_id = models.CharField(max_length=8, default=None, blank=True, null=True)
  room = models.ForeignKey(Room, on_delete=models.PROTECT, default=None, blank=True, null=True)
  date = models.DateTimeField(auto_now_add=True)
  visitors = models.ManyToManyField(User, related_name='visitors_authorized', default=None, blank=True)