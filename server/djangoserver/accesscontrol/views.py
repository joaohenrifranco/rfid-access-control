from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
from django.http import JsonResponse
import json
import datetime

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
			return HttpResponse("Malformed POST")

		response = {}
		log = Event()
		log.rfid = request_rfid_tag
		log.reader_position = request_action
		log.date = datetime.datetime.now()
		log.api_module = UNLOCK_API

		# Tries to get user and room from database with request info. Logs errors.
		try:
			user = User.objects.get(rfid_tag=request_rfid_tag)
			room = Room.objects.get(name=request_room_id)
		except Room.DoesNotExist:
			log.event_type = ROOM_NOT_FOUND
			response['status'] =  ROOM_NOT_FOUND
	
			return JsonResponse(response)
		except User.DoesNotExist:
			log.event_type = RFID_NOT_FOUND
			log.rfids = {request_rfid_tag}
			response['status'] = RFID_NOT_FOUND
			return JsonResponse(response)
		except:
			log.event_type = UNKNOWN_ERROR
			response['status'] = UNKNOWN_ERROR
			return JsonResponse(response)
		finally:
			log.save()

		
		# Now a user and a room can be attributted to the log
		log.room = room
		log.user = user
		log.save()

		# Always unlock door on exit
		if (request_action == 1):
			log.event_type = AUTHORIZED
			log.save()

			response['status'] = AUTHORIZED
			return JsonResponse(response)

		# Checks if RFID is from a visitor
		if (user.access_level == 0):
			log.event_type = VISITOR_RFID_FOUND
			log.save()
			
			response['status'] = VISITOR_RFID_FOUND
			return JsonResponse(response)

		# Checks if permission should be denied
		if user.access_level < room.access_level:
			log.event_type = INSUFFICIENT_PRIVILEGES
			log.save()

			response['status'] = INSUFFICIENT_PRIVILEGES
			return JsonResponse(response)

		# Checks if room needs password
		if (room.access_level >= REQUIRE_PASSWORD_LEVEL_THRESHOLD):
			log.event_type = PASSWORD_REQUIRED
			log.save()

			response['status'] = PASSWORD_REQUIRED
			return JsonResponse(response)
		
		# If reaches this point, authorize unlock
		
		log.event_type = AUTHORIZED
		log.save()

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
			request_rfid_tag = data['rfidTag']
		except:
			return HttpResponse("Malformed POST")
				
		response = {}

		log = Event()
		log.rfid = request_rfid_tag
		log.date = datetime.datetime.now()
		log.api_module = AUTH_API
		
		try:
			user = User.objects.get(rfid_tag=request_rfid_tag)
		except User.DoesNotExist:
			log.event_type = RFID_NOT_FOUND

			response['status'] =  RFID_NOT_FOUND
			return JsonResponse(response)
		except:
			log.event_type = UNKNOWN_ERROR

			response['status'] = UNKNOWN_ERROR
			return JsonResponse(response)
		finally:
			log.save()

		if user.hashed_password != request_hashed_password:
			log.event_type = WRONG_PASSWORD
			response['status'] = WRONG_PASSWORD
			return JsonResponse(response)

		log.event_type = AUTHORIZED
		
		response['status'] = AUTHORIZED
		
		return JsonResponse(response)

@csrf_exempt # Disables CSRF verification for this method
def authorize_visitor(request): ##TODO: MAKE AN ARRAY OF VISITOR TAGS
	if request.method == 'GET':
		return index(request)
	
	elif request.method == 'POST':

		try:
			data = json.loads(request.body)
			request_rfid_tag = data['rfidTag']
			request_visitor = data['rfidTagVisitor']
		except:
			return HttpResponse("Malformed POST")
		
		response = {}

		log = Event()
		log.rfid = request_rfid_tag
		log.date = datetime.datetime.now()
		log.api_module = VISITOR_API
		
		try:
			user = User.objects.get(rfid_tag=request_rfid_tag)
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