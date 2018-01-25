from django.shortcuts import render
from django.http import JsonResponse

def index(request):
    if request.method == 'POST':
        rfidTag = request.POST['rfidTag']
        clientId = request.POST['roomId']
        action = request.POST['action']

        response = {}

        response['status'] = 0
        response['rfidTag'] = rfidTag
        response['roomId'] = clientId
        response['action'] = action

    return JsonResponse(response)