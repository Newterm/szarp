#!/usr/bin/python
# -*- encoding: utf-8 -*-

#  SZARP: SCADA software 

#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

from collections import deque
import random
import time

class Sessions:
	"""Container for all sessions. Stolen from kaboo."""

	def __init__(self):
		"""Init empty container
		"""
		self.data		= {}		 	# dictionary of tokens
		self.otokens		= deque()	 	# deque of id of tokens

		self.expir		= 30 * 60	# session expiration time in seconds
		self.min_token_id	= 1			# Min of token Id
		self.max_token_id	= 100000	# Max of token Id (max - min > 10)

	def new(self, user_id, username):
		"""Create new session, return token for new session.
		"""
		token = self.new_token()
		self.data[token] = (time.time(), "", user_id, username) # (access time, last error string)
		self.otokens.append(token)
		return token

	def check(self, token):
		"""Check if session is active.
		"""
		# check if token is valid
		if not self.data.has_key(token):
			return (False, None, None)
		# check if session not expire
		(atime, errstr, user_id, username) = self.data[token]
		if time.time() - atime > self.expir:
			del self.data[token]
			return (False, None, None)
		# update last access token
		self.data[token] = (time.time(), errstr, user_id, username)
		return (True, user_id, username)

	def clean_expired_tokens(self):
		"""Purge all expired tokens.
		"""
		tokens = deque()
		for token in self.otokens:
			if self.data.has_key(token):
				(atime, errstr, user_id, username) = self.data[token]
				if time.time() - atime > self.expir:
					del self.data[token]
				else:
					tokens.append(token)
		self.otokens = tokens


	def new_token(self):
		"""Create new token, private.
		"""
		if len(self.data) > ((self.max_token_id - self.min_token_id) / 4):
			# Cache overflow detected
			# note: div 4 because we draw lots and we don't want do this again and again
			self.max_token_id = 2 * (self.max_token_id + 10)

		token = random.randint(self.min_token_id, self.max_token_id)
		while True:
			# Delete expired tokens
			self.clean_expired_tokens()
			# Check if token is uniq
			if not self.data.has_key(token):
				return token
			# try one more time
			token = random.randint(self.min_token_id, self.max_token_id)

