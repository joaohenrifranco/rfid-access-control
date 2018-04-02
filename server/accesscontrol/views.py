from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
from django.http import JsonResponse
import json
import datetime

from .services import *
from .models import *
from .errors import *


def index(request):
	return HttpResponse("Access control api is online! It is accessible through POST requests.")

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
			return malformed_post()

		response = {}
		log = Event()
		log.rfid = request_rfid_tag
		log.reader_position = request_action
		log.date = datetime.datetime.now()
		log.api_module = UNLOCK_API

		# Tries to get user and room from database with request info. Logs errors.
		try:
			user = get_current_tag_owner(request_rfid_tag)
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
			request_password = data['password']
			request_rfid_tag = data['rfidTag']
		except:
			return malformed_post()
				
		response = {}

		log = Event()
		log.reader_position = 0
		log.rfid = request_rfid_tag
		log.date = datetime.datetime.now()
		log.api_module = AUTH_API
		
		try:
			user = get_current_tag_owner(request_rfid_tag)
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

		if (not check_password()):
			log.event_type = WRONG_PASSWORD
			response['status'] = WRONG_PASSWORD
			return JsonResponse(response)

		log.event_type = AUTHORIZED
		
		response['status'] = AUTHORIZED
		
		return JsonResponse(response)

@csrf_exempt # Disables CSRF verification for this method
def authorize_visitor(request):
	if request.method == 'GET':
		return index(request)
	
	elif request.method == 'POST':

		request_visitor_array = []
		try:
			data = json.loads(request.body)
			request_rfid_tag = data['rfidTag']
			request_visitor_array = data['rfidTagsVisitors']
			request_room_id = data['roomId']
		except:
			return malformed_post()

		response = {}

		log = Event()
		log.rfid = request_rfid_tag
		log.date = datetime.datetime.now()
		log.reader_position = 0
		log.api_module = VISITOR_API

		try:
			room = Room.objects.get(name=request_room_id)
			user = get_current_tag_owner(request_rfid_tag)
		except User.DoesNotExist:
			log.event_type = RFID_NOT_FOUND

			response['status'] =  RFID_NOT_FOUND
			return JsonResponse(response)
		except:
			log.event_type = ROOM_NOT_FOUND
			response['status'] =  ROOM_NOT_FOUND
	
			return JsonResponse(response)
		finally:
			log.save()
		
		log.user = user
		log.room = room
		
		if (user.access_level == 0): 
			response['status'] = INSUFFICIENT_PRIVILEGES
			return JsonResponse(response)

		visitor_list = []

		for visitor_rfid in request_visitor_array:
			try:
				visitor_list.append(get_current_tag_owner(visitor_rfid))
			except:
				log.event_type = VISITOR_RFID_NOT_FOUND
				log.save()

				response['status'] = VISITOR_RFID_NOT_FOUND
				return JsonResponse(response)

		log.save()
		log.event_type = VISITOR_AUTHORIZED
		log.visitors.add(*visitor_list)
		log.save()
		
		response['status'] = VISITOR_AUTHORIZED
		return JsonResponse(response)