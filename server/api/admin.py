from django.contrib import admin
from django import forms
from django.forms import PasswordInput
from django.contrib.auth.models import Group
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin
from django.contrib.auth.forms import ReadOnlyPasswordHashField

from .models import Profile, Room, Event

admin.site.site_title = 'Controle de Acesso LASPI'
admin.site.site_header = 'Controle de Acesso LASPI'

class EventAdmin(admin.ModelAdmin):
    model = Event
    list_display = ('date','user', 'room', 'event_type', 'reader_position')
    list_filter = ['date','user', 'room', 'event_type', 'reader_position']

class ProfileForm(forms.ModelForm):
    class Meta:
        model = Profile
        exclude = ['last_login']
        
class ProfileAdmin(admin.ModelAdmin):
    form = ProfileForm

class ProfileCreationForm(forms.ModelForm):
    """A form for creating new users. Includes all the required
    fields, plus a repeated password."""
    password1 = forms.CharField(label='Password', widget=forms.PasswordInput)
    password2 = forms.CharField(label='Password confirmation', widget=forms.PasswordInput)

    class Meta:
        model = Profile
        fields = ('rfid_tag', 'access_level')

    def clean_password2(self):
        # Check that the two password entries match
        password1 = self.cleaned_data.get("password1")
        password2 = self.cleaned_data.get("password2")
        if password1 and password2 and password1 != password2:
            raise forms.ValidationError("Passwords don't match")
        return password2

    def save(self, commit=True):
        # Save the provided password in hashed format
        user = super(ProfileCreationForm, self).save(commit=False)
        user.set_password(self.cleaned_data["password1"])
        if commit:
            user.save()
        return user


class ProfileChangeForm(forms.ModelForm):
    """A form for updating users. Includes all the fields on
    the user, but replaces the password field with admin's
    password hash display field.
    """
    password = ReadOnlyPasswordHashField()

    class Meta:
        model = Profile
        fields = ('rfid_tag', 'access_level')

    def clean_password(self):
        # Regardless of what the user provides, return the initial value.
        # This is done here, rather than on the field, because the
        # field does not have access to the initial value
        return self.initial["password"]


class UserAdmin(BaseUserAdmin):
    # The forms to add and change user instances
    form = ProfileChangeForm
    add_form = ProfileCreationForm

    # The fields to be used in displaying the User model.
    # These override the definitions on the base UserAdmin
    # that reference specific fields on auth.User.
    fields = ('rfid_tag', 'access_level')
    list_display =  ('rfid_tag', 'access_level')
    list_filter = ('rfid_tag', 'access_level')
    # fieldsets = (
    #     (None, {'fields': ('email', 'password')}),
    #     ('Personal info', {'fields': ('date_of_birth',)}),
    #     ('Permissions', {'fields': ('is_admin',)}),
    # )
    # add_fieldsets is not a standard ModelAdmin attribute. UserAdmin
    # overrides get_fieldsets to use this attribute when creating a user.
    # add_fieldsets = (
    #     (None, {
    #         'classes': ('wide',),
    #         'fields': ('email', 'date_of_birth', 'password1', 'password2')}
    #     ),
    # )
    search_fields = ('name',)
    ordering = ('name',)
    filter_horizontal = ()


admin.site.register(Profile, ProfileAdmin)
admin.site.register(Room)
admin.site.register(Event, EventAdmin)