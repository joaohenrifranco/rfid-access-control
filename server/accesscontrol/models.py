from django.utils import timezone
from django.db import models
from django.contrib.auth.models import AbstractBaseUser, BaseUserManager
from django.utils.translation import ugettext_lazy as _
from django.core.exceptions import ValidationError
from accesscontrol.consts import *

ACCESS_LEVEL_CHOICES = (
  (0, _('visitor').title()),
  (1, _('level 1').title()),
  (2, _('level 2').title()),
  (3, _('level 3').title()),
  (4, _('level 4').title()),
  (5, _('level 5').title()),
)

class RfidTag(models.Model):
  uid = models.CharField(
    verbose_name=_('RFID tag UID'), 
    max_length=256, 
    default=None, 
    unique=True
    )
  created = models.DateTimeField(
    verbose_name=_('created'), 
    auto_now_add=True
    )
  class Meta:
    verbose_name = _('RFID tag')
  def __str__(self):
    return self.uid

class UserManager(BaseUserManager):
  def create_user(self, email, date_added=None, password=None):
    if not email:
      raise ValueError_('Users must have an email address')
    user = self.model(email=self.normalize_email(email),)
    user.set_password(password)
    user.save(using=self._db)
    return user

class User(AbstractBaseUser):
  objects = UserManager()
  USERNAME_FIELD = 'email'
  
  email = models.EmailField(
    verbose_name=_('email'),
    max_length=255, 
    unique=True
    )
  first_name = models.CharField(
    verbose_name=_('first name'),
    max_length=200
    )
  last_name = models.CharField(
    verbose_name=_('last name'),
    max_length=200
    )
  cpf = models.CharField(
    verbose_name=_('CPF'),
    max_length=11, 
    unique=True,
    null=True,
    blank=True
    )
  access_level = models.IntegerField(
    verbose_name=_('access level'),
    choices=ACCESS_LEVEL_CHOICES,
    default='0'
    )
  rfid_tag = models.ManyToManyField(
    RfidTag, 
    through='RfidTagUserLink', 
    blank=True, 
    verbose_name=_('RFID Tag UID')
    )
  created = models.DateTimeField(
    verbose_name=_('created'), 
    auto_now_add=True
    )
  sip = models.CharField(
    verbose_name=_('SIP'), 
    max_length=3, 
    unique=True, 
    blank=True, 
    null=True
    )
  
  def get_full_name(self):
    return '%s %s' % (self.first_name, self.last_name)

  def get_short_name(self):
    return self.first_name

  def __str__(self):
    return '%s %s' % (self.first_name, self.last_name)
  
  class Meta:
    verbose_name = _('user')

class RfidTagUserLink(models.Model):
  rfid_tag = models.ForeignKey(
    RfidTag, 
    on_delete=models.PROTECT, 
    default=None, 
    verbose_name=_('RFID Tag')
    )
  user = models.ForeignKey(
    User, 
    on_delete=models.PROTECT, 
    default=None, 
    verbose_name=_('user'))
  created = models.DateTimeField(
    auto_now_add=True, 
    verbose_name=_('created'))
  expire_date = models.DateTimeField(
    null=True, 
    blank=True, 
    verbose_name=_('expire date')
    )
  
  def __str__(self):
    return (self.rfid_tag.uid + ' - ' + self.user.get_full_name())
  
  def clean(self):
    if (self.expire_date is None or self.expire_date > timezone.now()):
      users_with_same_tag = User.objects.filter(rfid_tag=self.rfid_tag)
      print(users_with_same_tag)
      users_with_same_tag = users_with_same_tag.exclude(rfidtaguserlink__expire_date__lte=timezone.now())
      print(users_with_same_tag)
      users_with_same_tag = users_with_same_tag.exclude(pk=self.user.pk)
      print(users_with_same_tag)
      if (users_with_same_tag.count() > 0):
        raise ValidationError(_('This tag is already active with another user'))

class Room(models.Model):
  name = models.CharField(
    max_length=15, 
    verbose_name=_('room ID')
    )
  description = models.TextField(
    null=True, 
    blank=True, 
    verbose_name=_('description')
    )
  access_level = models.IntegerField(
    choices=ACCESS_LEVEL_CHOICES, 
    default='0', 
    verbose_name=_('access level')
    )
  class Meta:
    verbose_name = _('room')  
  def __str__(self):
    return self.name.title()
  
class Event(models.Model):
  EVENT_TYPE_CHOICES = (
    (AUTHORIZED, _('authorized').capitalize()),
    (UNREGISTERED_UID, _('unregistered tag').capitalize()),
    (INSUFFICIENT_PRIVILEGES, _('insufficient privileges').capitalize()),
    (WRONG_PASSWORD, _('wrong password').capitalize()),
    (PASSWORD_REQUIRED, _('password required').capitalize()),
    (VISITOR_UID_FOUND, _('visitor tag indentified').capitalize()),
    (VISITOR_AUTHORIZED, _('visitors authorized').capitalize()),
    (UNREGISTERED_VISITOR_UID, _('unregistered visitor tag').capitalize()),
    (OPEN_DOOR_TIMEOUT, _('open door timeout').capitalize()),
    (UNEXPECTED_ERROR, _('unexpected error').capitalize()),
    (ROOM_NOT_FOUND, _('room not found').capitalize()),
    (FRONT_DOOR_OPENED, _('front door opened').capitalize()),
    (UNREGISTERED_SIP, _('unregistered SIP').capitalize()),
  )

  READER_POSITION_CHOICES = ((0, _('outside').capitalize()),(1, _('inside').capitalize()))

  API_MODULE_CHOICES = (
    (AUTH_API, '/api/authenticate/'),
    (UNLOCK_API, '/api/request-unlock/'),
    (VISITOR_API, '/api/authorize-visitor'),
    (FRONT_DOOR_API, '/api/request-front-door-unlock'),
  )

  user = models.ForeignKey(
    User, 
    on_delete=models.PROTECT, 
    default=None, 
    blank=True, 
    null=True, 
    related_name='events', 
    verbose_name=_('user')
    )  
  event_type = models.IntegerField(
    choices=EVENT_TYPE_CHOICES, 
    null=True, 
    verbose_name=_('event type')
    )
  reader_position = models.IntegerField(
    choices=READER_POSITION_CHOICES, 
    verbose_name=_('reader position')
    )
  api_module = models.IntegerField(
    choices=API_MODULE_CHOICES, 
    default=None, 
    verbose_name=_('api Module')
    )
  uid = models.CharField(
    max_length=256, 
    default=None, 
    blank=True, 
    null=True,
    verbose_name=_('RFID Tag UID')
    )
  sip = models.CharField(
    verbose_name=_('SIP'), 
    max_length=3, 
    blank=True, 
    null=True
    )
  room = models.ForeignKey(
    Room,
    on_delete=models.PROTECT, 
    default=None, 
    blank=True, 
    null=True, 
    verbose_name=_('room')
    )
  date = models.DateTimeField(
    auto_now_add=True,
    verbose_name=_('date ocurred')
    )
  visitors = models.ManyToManyField(
    User, 
    related_name='visitors_authorized', 
    default=None, 
    blank=True, 
    verbose_name=_('visitors')
    )

  def __str__(self):
    return (
      self.get_event_type_display() + ' - ' + self.date.strftime('%Y-%m-%d %H:%M:%S')
    )
  class Meta:
    verbose_name = _('event')