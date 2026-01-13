#!/usr/bin/env python3
import hashlib


# data must be in this format so it can account for null bytes

data = b"\x31\x32\x33\x34\x35\x0a" + b"\x00" * 20

digest = hashlib.md5(data).hexdigest()

print(digest)
