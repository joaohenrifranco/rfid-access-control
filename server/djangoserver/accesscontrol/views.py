from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
from django.http import JsonResponse
from .models import *

REQUIRE_LEVEL_THRESHOLD = 3

SERVER_SETUP_ERROR = -1
AUTHORIZED = 0
RFID_NOT_FOUND = 1
INSUFFICIENT_PRIVILEGES = 2
WRONG_PASSWORD = 3
PASSWORD_REQUIRED = 4

def index(request):
    return HttpResponse("Access control api is online!")

@csrf_exempt
def request_unlock(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':
        request_rfid_tag = request.POST['rfidTag']
        request_room_id = request.POST['roomId']
        request_action = request.POST['action']

        response = {}

        # Tries to get user with request's rfid tag
        try:
            user = User.objects.get(rfid_tag=request_rfid_tag)
        except  :
            response['status'] =  RFID_NOT_FOUND
            return JsonResponse(response)
        except:
            response['status'] = SERVER_SETUP_ERROR
            return JsonResponse(response)

        # Tries to get room with request's room id
        try:
            room = Room.objects.get(name=request_room_id)
        except:
            response['status'] = SERVER_SETUP_ERROR
            return JsonResponse(response)

        # Checks if permission should be denied
        if user.accesslevel < room.accesslevel:
            response['status'] = INSUFFICIENT_PRIVILEGES
            return JsonResponse()

        # Checks if room needs password
        if (room.accesslevel >= REQUIRE_LEVEL_THRESHOLD) and (request_action != 1):
            response['status'] = PASSWORD_REQUIRED
            return JsonResponse()
        
        # If reaches this point, authorize unlock
        response['status'] = AUTHORIZED
        return JsonResponse(response)

@csrf_exempt
def authenticate(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':
        request_hashed_password = request.POST['password']
        request_user = request.POST['rfidTag']
        
        try:
            user = User.objects.get(rfid_tag=request_rfid_tag)
        except:
            response['status'] = SERVER_SETUP_ERROR
            return JsonResponse(response)

        if user.hashed_password != request_hashed_password:
            response['status'] = WRONG_PASSWORD
            return JsonResponse(response)

        response['status'] = AUTHORIZED
        return JsonResponse(response)

@csrf_exempt
def authorize_visitor(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':
        request_user = request.POST['rfidTag']
        request_ = request.POST['rfidTagVisitor']
        
        try:
            user = User.objects.get(rfid_tag=request_rfid_tag)
        except:
            response['status'] = SERVER_SETUP_ERROR
            return JsonResponse(response)

        try:
            visitor = User.objects.get(rfid_tag=request_rfid_tag)
        except:
            response['status'] = RFID_NOT_FOUND
            return JsonResponse(response)

        response['status'] = AUTHORIZED
        return JsonResponse(response)