from django.contrib import admin
from .models import User as UserProfile, Room, Event
from django.contrib.auth.models import User
from django.contrib.auth.models import Group

admin.site.site_title = 'Controle de Acesso LASPI'
admin.site.site_header = 'Controle de Acesso LASPI'

class EventAdmin(admin.ModelAdmin):
    model = Event
    list_display = ('date','user', 'room', 'event_type', 'reader_position')
    list_filter = ['date','user', 'room', 'event_type', 'reader_position']

admin.site.register(UserProfile)
admin.site.register(Room)
admin.site.register(Event, EventAdmin)

admin.site.unregister(User)
admin.site.unregister(Group)
