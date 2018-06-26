import datetime
from django.db.models import Q
from django.http import HttpResponse
from django.db.models.signals import pre_save
from django.dispatch import receiver
from accesscontrol.models import *
from accesscontrol.consts import *

def get_current_tag_owner(uid):
    not_expired_users = User.objects.filter(rfidtaguserlink__expire_date__gte=datetime.date.today())
    never_expire_users = User.objects.filter(rfidtaguserlink__expire_date__isnull=True)
    all_valid_users = not_expired_users | never_expire_users
    return all_valid_users.get(rfid_tag__uid__iexact=uid)

def check_password(user, password):
    if (user.password.lower() == ("%s%s" % ("sha256$$", password)).lower()):
        return True
    return False

def malformed_post():
    return HttpResponse("Malformed POST request. Please check documentation.")