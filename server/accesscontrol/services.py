import datetime
from django.db.models import Q
from django.http import HttpResponse

from .models import *
from .consts import *

def get_current_tag_owner(rfid_tag):
    from accesscontrol.models import User
    not_expired_users = User.objects.filter(rfidtaguserlink__expire_date__gte=datetime.date.today())
    never_expire_users = User.objects.filter(rfidtaguserlink__expire_date__isnull=True)
    all_valid_users = not_expired_users | never_expire_users

    print(all_valid_users)
    return all_valid_users.get(rfid_tag__rfid_tag_id=rfid_tag.lower())

def check_password(user, password):
    if (user.password.lower() == ("%s%s" % ("sha256$$", password)).lower()):
        return True
    return False

def correct_double_association(rfid_tag):
    try:
        user = get_current_tag_owner(rfid_tag)
    except:
        return
    
    user.rfid_tag.expire_date = datetime.datetime.now()


def malformed_post():
    return HttpResponse("Malformed POST request. Please check documentation.")