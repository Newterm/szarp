"""Helper functions

Consists of functions to typically be used within templates, but also
available to Controllers. This module is available to both as 'h'.
"""

#from webhelpers.rails.wrapped import *

from webhelpers.date import *
from webhelpers.text import *
from webhelpers.html.converters import *
from webhelpers.html.tools import *
from webhelpers.util import *
from webhelpers.containers import *
from webhelpers.html.tags import *
from routes import url_for, redirect_to
from pylons import config

import string
from smtplib import SMTP
from email.message import Message
from email.header import Header

def count_different(str):
	"""
	Count number of different characters in str
	"""
	l = list(str)
	l.sort()
	c = 0
	for i in range(1, len(l) - 1):
		if l[i] != l[i - 1]:
			c += 1
	return c

def check_classes(str):
	"""
	Check if str contains at least one letter and at least one digit.
	"""
	l = False
	d = False
	for i in str:
		if i in string.letters:
			l = True
		if i in string.digits:
			d = True
	return (l and d)

def check_password(user, p):
	"""
	Returns None if password is ok, otherwise description of problem
	"""
	if len(p) < 8:
		return "Password is too short!"
	if p == user:
		return "Password is too similar to user name!"
	if not check_classes(p):
		return "Password must contain at least one letter and one digit!"
	if count_different(p) < 5:
		return "Password is too simple!"
	return None

def send_email(to_a, subject, message):
	"""
	Sends e-mail
	"""
	m = Message()
	#m['From'] = config['sss_mail']
	m['To'] = to_a
	m['Subject'] = Header(subject, 'utf-8')
	m.set_charset('utf-8')
	del m['Content-Transfer-Encoding']
	m['Content-Transfer-Encoding'] = '8bit'
	m.set_payload(message.encode('utf-8'))

	smtp = SMTP(config['smtp_server'])
	smtp.sendmail(config['sss_mail'], to_a, m.as_string())
	smtp.quit()

