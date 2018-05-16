from django.db import models
from .consts import *
from django.contrib.auth.models import AbstractBaseUser, BaseUserManager
from django.utils.translation import ugettext_lazy as _
#from django.db.models.signals import pre_save
from .services import *

ACCESS_LEVEL_CHOICES = (
  (0, _('Visitor')),
  (1, _('Level 1')),
  (2, _('Level 2')),
  (3, _('Level 3')),
  (4, _('Level 4')),
  (5, _('Level 5')),
)

class RfidTag(models.Model):
  uid = models.CharField(verbose_name=_("RFID Tag UID"), max_length=8, default=None, unique=True)
  created = models.DateTimeField(verbose_name=_("Created"), auto_now_add=True)
  def __str__(self):
    return self.uid

class UserManager(BaseUserManager):
  def create_user(self, email, date_added=None, password=None):
    if not email:
      raise ValueError('Users must have an email address')
    user = self.model(email=self.normalize_email(email),)
    user.set_password(password)
    user.save(using=self._db)
    return user

class User(AbstractBaseUser):
  email = models.EmailField(verbose_name=_("Email"), max_length=255, unique=True)
  first_name = models.CharField(verbose_name=_("First name"), max_length=200)
  last_name = models.CharField(verbose_name=_("Last name"), max_length=200)
  cpf = models.CharField(verbose_name=_("CPF"), max_length=11, unique=True)
  access_level = models.IntegerField(verbose_name=_("Access level"), choices=ACCESS_LEVEL_CHOICES, default='0')
  uid = models.ManyToManyField(RfidTag, through='RfidTagUserLink', blank=True, verbose_name=_("RFID Tag UID"))
  created = models.DateTimeField(verbose_name=_("Created"), auto_now_add=True)
  sip = models.CharField(verbose_name=_("SIP"), max_length=3, unique=True, blank=True, null=True)
  
  objects = UserManager()

  USERNAME_FIELD = 'email'

  def get_full_name(self):
    return "%s %s" % (self.first_name, self.last_name)

  def get_short_name(self):
    return self.first_name

  def __str__(self):
    return "%s %s" % (self.first_name, self.last_name)

class RfidTagUserLink(models.Model):
  rfid_tag = models.ForeignKey(RfidTag, on_delete=models.PROTECT, default=None, verbose_name=_("RFID Tag"))
  user = models.ForeignKey(User, on_delete=models.PROTECT, default=None, verbose_name=_("User"))
  created = models.DateTimeField(auto_now_add=True, verbose_name=_("Created"))
  expire_date = models.DateTimeField(null=True, blank=True, verbose_name=_("Expire date"))
  
  def __str__(self):
    return (self.rfid_tag.uid + " - " + self.user.get_full_name())

  # def rfid_tag_user_link_pre_save(sender, instance, *args, **kwargs):
  #   # TODO: DEACTIVATE OTHER ENTRIS WITH SAME RFID TAG LINK
  #   # if created:
  #   #   print("Created")

#pre_save.connect(RfidTagUserLink.rfid_tag_user_link_pre_save, RfidTagUserLink, dispatch_uid=".models.RfidTagUserLink")

class Room(models.Model):
  name = models.CharField(max_length=15, verbose_name=_("Room ID"))
  description = models.TextField(null=True, blank=True, verbose_name=_("Description"))
  access_level = models.IntegerField(choices=ACCESS_LEVEL_CHOICES, default='5', verbose_name=_("Access level"))
  
  # Necessary to show name correctly at DjangoAdmin
  def __str__(self):
    return self.name.title()
  
class Event(models.Model):
  EVENT_TYPE_CHOICES = (
    (AUTHORIZED, _('Authorized')),
    (UNREGISTERED_UID, _('Unregistered Tag')),
    (INSUFFICIENT_PRIVILEGES, _('Insufficient Privileges')),
    (WRONG_PASSWORD, _('Wrong Password')),
    (PASSWORD_REQUIRED, _('Password Required')),
    (VISITOR_UID_FOUND, _('Visitor Tag Indentified')),
    (VISITOR_AUTHORIZED, _('Visitor Authorized')),
    (UNREGISTERED_VISITOR_UID, _('Unregistered Visitor Tag')),
    (OPEN_DOOR_TIMEOUT, _('Open Door Timeout')),
    (UNEXPECTED_ERROR, _('Unexpected Error')),
    (ROOM_NOT_FOUND, _('Room Not Found')),
    (FRONT_DOOR_OPENED, _('Front Door Opened')),
    (UNREGISTERED_SIP, _('Unregistered SIP')),
  )

  READER_POSITION_CHOICES = ((0, _('Outside')),(1, _('Inside')))

  API_MODULE_CHOICES = (
    (AUTH_API, '/api/authenticate/'),
    (UNLOCK_API, '/api/request-unlock/'),
    (VISITOR_API, '/api/authorize-visitor'),
    (FRONT_DOOR_API, '/api/request-front-door-unlock'),
  )

  user = models.ForeignKey(User, on_delete=models.PROTECT, default=None, blank=True, null=True, related_name='events', verbose_name=_("User"))
  
  event_type = models.IntegerField(choices=EVENT_TYPE_CHOICES, verbose_name=_("Event type"))
  reader_position = models.IntegerField(choices=READER_POSITION_CHOICES, verbose_name=_("Reader position"))
  api_module = models.IntegerField(choices=API_MODULE_CHOICES, default=None, verbose_name=_("Api Module"))
  uid = models.CharField(max_length=8, default=None, blank=True, null=True, verbose_name=_("RFID Tag UID"))
  sip = models.CharField(verbose_name=_("SIP"), max_length=3, unique=True, blank=True, null=True)
  room = models.ForeignKey(Room, on_delete=models.PROTECT, default=None, blank=True, null=True, verbose_name=_("Room"))
  date = models.DateTimeField(auto_now_add=True, verbose_name=_("Date ocurred"))
  visitors = models.ManyToManyField(User, related_name='visitors_authorized', default=None, blank=True, verbose_name=_("Visitors"))