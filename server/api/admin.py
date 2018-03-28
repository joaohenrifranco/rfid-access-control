from django.contrib import admin
from django import forms
from django.forms import PasswordInput
from django.contrib.auth.models import User as DjangoAdminUser, Group
from django.contrib.auth.admin import UserAdmin as BaseUserAdmin
from django.contrib.auth.forms import ReadOnlyPasswordHashField
from django.contrib.admin.utils import flatten_fieldsets
from django.utils import timezone
from api.models import User, Room, Event, RfidTagUserLink

admin.site.site_title = 'Controle de Acesso LASPI'
admin.site.site_header = 'Controle de Acesso LASPI'

class UserCreationForm(forms.ModelForm):

    password1 = forms.CharField(label='Password', widget=forms.PasswordInput)
    password2 = forms.CharField(label='Password confirmation', widget=forms.PasswordInput)

    class Meta:
        model = User
        fields = ('first_name','last_name','email', 'cpf','access_level')

    def clean_password2(self):
        # Check that the two password entries match
        password1 = self.cleaned_data.get("password1")
        password2 = self.cleaned_data.get("password2")
        if password1 and password2 and password1 != password2:
            raise forms.ValidationError("Passwords don't match")
        return password2

    def save(self, commit=True):
        # Save the provided password in hashed format
        user = super(UserCreationForm, self).save(commit=False)
        user.set_password(self.cleaned_data["password1"])
        user.date_added = timezone.now()
        if commit:
            user.save()
        return user


class UserChangeForm(forms.ModelForm):
    password1 = forms.CharField(
        label='Password', 
        widget=forms.PasswordInput, 
        required = False, 
        help_text="Leave blank to keep same password")
    password2 = forms.CharField(
        label='Password confirmation', 
        widget=forms.PasswordInput, 
        required = False,
        help_text="Leave blank to keep same password")

    class Meta:
        model = User
        fields = ('first_name','last_name','email', 'cpf','access_level')

    def clean_password2(self):
        # Check that the two password entries match
        password1 = self.cleaned_data.get("password1")
        password2 = self.cleaned_data.get("password2")
        if password1 != password2:
            raise forms.ValidationError("Passwords don't match")
        return password2

    def save(self, commit=True):
        # Save the provided password in hashed format
        user = super(UserChangeForm, self).save(commit=False)
        if (self.cleaned_data["password1"] != ""):
            user.set_password(self.cleaned_data["password1"])
        if commit:
            user.save()
        return user
    
    def clean_date_added(self):
        return self.initial["date_added"]

class UserAdmin(BaseUserAdmin):
    # The forms to add and change user instances
    form = UserChangeForm
    add_form = UserCreationForm

    list_display = ('first_name','last_name','email', 'cpf','access_level')
    list_filter = ('access_level', )
    fieldsets = (
        (None, {'fields': ('first_name','last_name','email', 'cpf','access_level', 'password1', 'password2')}),
    )

    add_fieldsets = (
        (None, {
            'classes': ('wide',),
            'fields': ('first_name','last_name','email', 'cpf','access_level', 'password1', 'password2')}
        ),
    )
    search_fields =  ('first_name','last_name','email', 'cpf','access_level',)
    ordering = ()
    filter_horizontal = ()

class EventAdmin(admin.ModelAdmin):
    model = Event
    list_display = ('date','user', 'room', 'event_type', 'reader_position')
    list_filter = ['date','user', 'room', 'event_type', 'reader_position']

    def get_readonly_fields(self, request, obj=None):
        if self.declared_fieldsets:
            fields = flatten_fieldsets(self.declared_fieldsets)
        else:
            form = self.get_formset(request, obj).form
            fields = form.base_fields.keys()
        return fields
    
    def has_add_permission(self, request):
        # Nobody is allowed to add
        return False
    def has_delete_permission(self, request, obj=None):
        # Nobody is allowed to delete
        return False

admin.site.register(User, UserAdmin)        
admin.site.register(Room)
admin.site.register(RfidTagUserLink)
admin.site.register(Event, EventAdmin)