import base64
import binascii
import functools
import hashlib
import importlib
import warnings
from collections import OrderedDict

from django.conf import settings
from django.core.exceptions import ImproperlyConfigured
from django.core.signals import setting_changed
from django.dispatch import receiver
from django.utils.crypto import (
    constant_time_compare, get_random_string, pbkdf2,
)
from django.utils.module_loading import import_string
from django.utils.translation import gettext_noop as _

from django.contrib.auth.hashers import BasePasswordHasher, mask_hash

class SHA256PasswordHasher(BasePasswordHasher):

    algorithm = "sha256"

    # Overrides default salting (random string generation)
    # Though insecure, this is done because the arduino sends password hashed without salt
    # Arduino does it just to add a layer of security while sending unencrypted POST request
    # due to hardware limitations

    def salt(self):
        return ""

    def encode(self, password, salt):
        assert password is not None
        # Line below is commented because salt is empty
        #assert salt and '$' not in salt
        hash = hashlib.sha256(password.encode()).hexdigest()
        return "%s$%s$%s" % (self.algorithm, salt, hash)

    def verify(self, password, encoded):
        algorithm, salt, hash = encoded.split('$', 2)
        assert algorithm == self.algorithm
        encoded_2 = self.encode(password, salt)
        return constant_time_compare(encoded, encoded_2)

    def safe_summary(self, encoded):
        algorithm, salt, hash = encoded.split('$', 2)
        assert algorithm == self.algorithm
        return OrderedDict([
            (_('algorithm'), algorithm),
            (_('salt'), mask_hash(salt, show=2)),
            (_('hash'), mask_hash(hash)),
        ])

    def harden_runtime(self, password, encoded):
        pass
