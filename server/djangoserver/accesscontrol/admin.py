from django.contrib import admin
from django import forms
from django.forms import PasswordInput

from django.contrib.auth.models import User
from django.contrib.auth.models import Group

from .models import User as UserProfile, Room, Event

admin.site.site_title = 'Controle de Acesso LASPI'
admin.site.site_header = 'Controle de Acesso LASPI'

class EventAdmin(admin.ModelAdmin):
    model = Event
    list_display = ('date','user', 'room', 'event_type', 'reader_position')
    list_filter = ['date','user', 'room', 'event_type', 'reader_position']

class UserForm(forms.ModelForm):
    password = forms.CharField(widget=PasswordInput())
    class Meta:
        model = User
        exclude = ['hashed_password']
    def clean(self):
        if not password.isdigit():
            raise ValidationError('Sorry, password must contain only digits.')

class UserAdmin(admin.ModelAdmin):
    form = UserForm


admin.site.register(Room)
admin.site.register(Event, EventAdmin)

admin.site.register(UserProfile, UserAdmin)

#admin.site.unregister(User)
#admin.site.unregister(Group)
