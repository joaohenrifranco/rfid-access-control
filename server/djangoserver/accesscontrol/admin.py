from django.contrib import admin

from .models import *

admin.site.register(User)
admin.site.register(Room)
admin.site.register(Event)
