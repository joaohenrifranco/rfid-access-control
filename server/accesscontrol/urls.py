from django.urls import path
from . import admin
from . import views

urlpatterns = [
    path('request-unlock', views.request_unlock),
    path('authenticate', views.authenticate),
    path('authorize-visitor', views.authorize_visitor),
    path('request-front-door-unlock', views.request_front_door_unlock),
]
