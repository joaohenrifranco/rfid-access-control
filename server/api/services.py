import datetime
from django.db.models import Q

from .models import *
from .errors import *

def get_active_tag_list():
    return RfidTagUserLink.objects.filter(models.Q(expire_date__time__gte=datetime.date.today()) | models.Q(models.Q(expire_date__isnull=True)))

def get_current_tag_owner(rfid_tag):
    return get_active_tag_list().get(user=self.rfid_tag)