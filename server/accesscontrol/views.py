import json
import datetime
from django.http import HttpResponse
from django.views.decorators.csrf import csrf_exempt
from django.http import JsonResponse
from accesscontrol.services import *
from accesscontrol.models import *
from accesscontrol.consts import *

def index(request):
	return HttpResponse_('Access control api is online! It is accessible through POST requests.')

@csrf_exempt # Disables CSRF verification for this method
def request_unlock(request):
	if request.method == 'GET':
		return index(request)
	
	elif request.method == 'POST':
		try:
			data = json.loads(request.body)
			request_uid = data['uid']
			request_room_id = data['roomID']
			request_reader_position = data['readerPosition']
		except:
			return malformed_post()

		response = {}
		log = Event()
		log.uid = request_uid
		log.reader_position = request_reader_position
		log.date = datetime.datetime.now()
		log.api_module = UNLOCK_API

		try:
			user = get_current_tag_owner(request_uid)
			room = Room.objects.get(name=request_room_id)
		except Room.DoesNotExist:
			log.event_type = ROOM_NOT_FOUND
			response['status'] =  ROOM_NOT_FOUND
			return JsonResponse(response)
		except User.DoesNotExist:
			log.event_type = UNREGISTERED_UID
			log.uids = {request_uid}
			response['status'] = UNREGISTERED_UID
			return JsonResponse(response)
		except:
			log.event_type = UNEXPECTED_ERROR
			response['status'] = UNEXPECTED_ERROR
			return JsonResponse(response)
		finally:
			log.save()
		
		log.room = room
		log.user = user
		log.save()

		# Always authorize from inside
		if (request_reader_position == 1):
			log.event_type = AUTHORIZED
			log.save()
			response['status'] = AUTHORIZED
			return JsonResponse(response)

		# Checks if UID is from a visitor
		if (user.access_level == 0):
			log.event_type = VISITOR_UID_FOUND
			log.save()
			response['status'] = VISITOR_UID_FOUND
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
			request_uid = data['uid']
			request_room_id = data['roomID']
		except:
			return malformed_post()
				
		response = {}
		log = Event()
		log.reader_position = 0
		log.uid = request_uid
		log.date = datetime.datetime.now()
		log.api_module = AUTH_API
		
		try:
			user = get_current_tag_owner(request_uid)
			room = Room.objects.get(name=request_room_id)
		except Room.DoesNotExist:
			log.event_type = ROOM_NOT_FOUND
			response['status'] =  ROOM_NOT_FOUND
			return JsonResponse(response)
		except User.DoesNotExist:
			log.event_type = UNREGISTERED_UID
			response['status'] =  UNREGISTERED_UID
			return JsonResponse(response)
		except:
			log.event_type = UNEXPECTED_ERROR
			response['status'] = UNEXPECTED_ERROR
			return JsonResponse(response)
		finally:
			log.save()

		log.user = user
		log.room = room
		log.save()

		if (not check_password(user, request_password)):
			log.event_type = WRONG_PASSWORD
			response['status'] = WRONG_PASSWORD
			log.save()
			return JsonResponse(response)

		log.event_type = AUTHORIZED
		log.save()
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
			request_uid = data['uid']
			request_visitor_array = data['visitorsUids']
			request_room_id = data['roomID']
		except:
			return malformed_post()

		response = {}
		log = Event()
		log.uid = request_uid
		log.date = datetime.datetime.now()
		log.reader_position = 0
		log.api_module = VISITOR_API

		try:
			room = Room.objects.get(name=request_room_id)
			user = get_current_tag_owner(request_uid)
		except User.DoesNotExist:
			log.event_type = UNREGISTERED_UID
			response['status'] =  UNREGISTERED_UID
			return JsonResponse(response)
		except:
			log.event_type = ROOM_NOT_FOUND
			response['status'] =  ROOM_NOT_FOUND
			return JsonResponse(response)
		finally:
			log.save()
		
		log.user = user
		log.room = room
		log.save()
		
		if (user.access_level == 0): 
			response['status'] = INSUFFICIENT_PRIVILEGES
			return JsonResponse(response)

		visitor_list = []

		for visitor_uid in request_visitor_array:
			try:
				visitor_list.append(get_current_tag_owner(visitor_uid))
			except:
				log.event_type = UNREGISTERED_VISITOR_UID
				response['status'] = UNREGISTERED_VISITOR_UID
				return JsonResponse(response)
			finally:
				log.save()

		log.event_type = VISITOR_AUTHORIZED
		log.visitors.add(*visitor_list)
		log.save()
		
		response['status'] = VISITOR_AUTHORIZED
		return JsonResponse(response)

@csrf_exempt
def request_front_door_unlock(request):
	if request.method == 'GET':
		return index(request)
	
	elif request.method == 'POST':
		try:
			data = json.loads(request.body)
			request_sip_id = data['sip']
		except:
			return malformed_post()

		response = {}
		log = Event()
		log.sip = request_sip_id
		log.date = datetime.datetime.now()
		log.reader_position = 0
		log.api_module = FRONT_DOOR_API

		try:
			user = User.objects.get(sip=request_sip_id)
		except User.DoesNotExist:
			log.event_type = UNREGISTERED_SIP
			response['status'] =  UNREGISTERED_SIP
			return JsonResponse(response)
		except:
			log.event_type = UNEXPECTED_ERROR
			response['status'] = UNEXPECTED_ERROR
			return JsonResponse(response)
		finally:
			log.save()
		
		log.user = user
		log.save()
		
		if (user.access_level == 0): 
			response['status'] = INSUFFICIENT_PRIVILEGES
			log.event_type = INSUFFICIENT_PRIVILEGES
			log.save()
			return JsonResponse(response)

		log.event_type = FRONT_DOOR_OPENED
		log.save()
		response['status'] = FRONT_DOOR_OPENED
		return JsonResponse(response)