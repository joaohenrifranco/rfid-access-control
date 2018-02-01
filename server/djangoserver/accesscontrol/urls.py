from django.urls import path

from . import views

urlpatterns = [
    path('', views.index, name='index'),
    path('request-unlock', views.request_unlock),
    path('authenticate', views.authenticate),
    path('authorize-visitor', views.authorize_visitor),
]