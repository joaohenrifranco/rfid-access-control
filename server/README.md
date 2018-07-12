# Server
Django based server that provides a RESTful API for authentication and logging.

Also provides a control panel made with Django Admin.

Works with [Django 2.0+](https://docs.djangoproject.com/en/2.0/).

## Running

 - Get Django through pip
 - Clone this repo and navigate to `server/`

Now creating  our database, run the following commands:

 - `python3 manage.py makemigrations` 
 - `python3 manage.py migrate`

Creating a admin login for the dashboard:

 - `python3 manage.py createsuperuser` and fill the asked fields
 
 Now let's test the server:
 - `python3 manage.py runserver`

**Don't run Django's test server in production**. I recommend using Apache with mod_wsig.

Please check [Django documentation](https://docs.djangoproject.com/en/2.0/) for more info.



## Database overview
These can be checked and modified at    `/accesscontrol/views.py`

- **Rooms**: have an id and a level required to get in.
-  **Users**: have multiple identifications fields, a access level, a numeric password and one or multiple RFID tag associated.
- **RFID Tags**: contain a unique uid and a expiration date.

## The API
They are pretty self-explanatory and their complete behaviour can be understood by a quick look at `/accesscontrol/views.py`. However, for a quick overview:

  -   `/api/request-unlock`
Checks if the user has privileges to enter the desired room, if he is entering or leaving and if the room requires password.

  -   `/api/authenticate`

Verifies user password

  -   `/api/authorize-visitor`

Authorizes users with "Visitor" level. Only for logging purposes.

- `/api/request-front-door-unlock`

Used by the Asterisk "smart doorbell". Described in the [main readme](https://github.com/joaohenriquef/rfid-access-control/blob/master/README.md).

## Translations

Although all code is in English, the interface has been translated to Portuguese as it was intended for use in Brazil. The default language is Portuguese however it can be changed to English in `/djangoserver/settings.py`. You can also add new languages and translations.
