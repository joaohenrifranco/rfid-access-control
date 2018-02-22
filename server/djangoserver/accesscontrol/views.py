from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
from django.http import JsonResponse
import json

from .models import *
from .errors import *

REQUIRE_PASSWORD_LEVEL_THRESHOLD = 3

def index(request):
    return HttpResponse("Access control api is online!")

@csrf_exempt # Disables CSRF verification for this method
def request_unlock(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':

        # Tries to parse json post body
        try:
            data = json.loads(request.body)
            request_rfid_tag = data['rfidTag']
            request_room_id = data['roomId']
            request_action = data['action']
        except:
            return HttpResponse("Malformed POST: " + request.body)
        
        response = {}
        log = Event()

        # Tries to get user with request's rfid tag
        try:
            user = User.objects.get(rfid_tag=request_rfid_tag)
        except User.DoesNotExist:
            response['status'] =  RFID_NOT_FOUND
            return JsonResponse(response)
        except:
            response['status'] = UNKNOWN_ERROR
            return JsonResponse(response)
        
        # Always unlock door on exit
        if (request_action == 1):
            response['status'] = AUTHORIZED
            return JsonResponse(response)

        # Checks if RFID is from a visitor
        if (user.access_level == 0):
            response['status'] = VISITOR_RFID_FOUND
            return JsonResponse(response) 

        # Tries to get room with request's room id
        try:
            room = Room.objects.get(name=request_room_id)
        except Room.DoesNotExist:
            response['status'] =  ROOM_NOT_FOUND
            return JsonResponse(response)
        except:
            response['status'] = UNKNOWN_ERROR
            return JsonResponse(response)

        # Checks if permission should be denied
        if user.access_level < room.access_level:
            response['status'] = INSUFFICIENT_PRIVILEGES
            return JsonResponse(response)

        # Checks if room needs password
        if (room.access_level >= REQUIRE_PASSWORD_LEVEL_THRESHOLD):
            response['status'] = PASSWORD_REQUIRED
            return JsonResponse(response)
        
        # If reaches this point, authorize unlock
        response['status'] = AUTHORIZED
        return JsonResponse(response)

@csrf_exempt # Disables CSRF verification for this method
def authenticate(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':

        try:
            data = json.loads(request.body)
            request_hashed_password = data['password']
            request_user = data['rfidTag']
        except:
            return HttpResponse("Malformed POST: " + request.body)
                
        response = {}
        
        try:
            user = User.objects.get(rfid_tag=request_user)
        except User.DoesNotExist:
            response['status'] =  RFID_NOT_FOUND
            return JsonResponse(response)
        except:
            response['status'] = UNKNOWN_ERROR
            return JsonResponse(response)

        if user.hashed_password != request_hashed_password:
            response['status'] = WRONG_PASSWORD
            return JsonResponse(response)

        response['status'] = AUTHORIZED
        return JsonResponse(response)

@csrf_exempt # Disables CSRF verification for this method
def authorize_visitor(request):
    if request.method == 'GET':
        return index(request)
    
    elif request.method == 'POST':

        try:
            data = json.loads(request.body)
            request_user = data['rfidTag']
            request_visitor = data['rfidTagVisitor']
        except:
            return HttpResponse("Malformed POST: " + request.body)
        
        response = {}
        
        try:
            user = User.objects.get(rfid_tag=request_user)
        except:
            response['status'] = RFID_NOT_FOUND
            return JsonResponse(response)

        if (user.access_level == 0): 
            response['status'] = INSUFFICIENT_PRIVILEGES
            return JsonResponse(response)

        try:
            visitor = User.objects.get(rfid_tag=request_visitor)
        except:
            response['status'] = RFID_NOT_FOUND
            return JsonResponse(response)

        response['status'] = VISITOR_AUTHORIZED
        return JsonResponse(response)