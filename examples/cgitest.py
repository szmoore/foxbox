#!/usr/bin/python

import cgi
import os

print("Content-type: text/plain\r\n\r\n")

for var in os.environ:
	print(var + " = " + os.environ[var])
