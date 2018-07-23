#!/usr/bin/python
# -*- encoding: utf-8 -*-

#script to gather and show data from Instagram
#change photo_URL and account_URL to run this script

import sys                                  # exit()
import time                                 # sleep()
import logging
from logging.handlers import SysLogHandler

import requests
import re

BASE_PREFIX = ""
photo_URL = "URL_to_instagram_photo"
account_URL = "URL_to_instagram_account"

instagram_suffixes = {
	'k' : 1000,
	'm' : 1000000
}

class InstaReader:
	def get_trailing_number(self, text, key):
		pos = text.find(key)
		text = text[:pos-1].strip()
		multiplier = 1
		if text[-1] in instagram_suffixes.keys() :
			multiplier = instagram_suffixes[text[-1]]
			text = text[:-1]
		number = re.search(r'\d+(((\.|\,)\d+)?)$', text).group()
		number = float(number.replace(',', '.'))
		return int(multiplier * number)

	def get_photo_likes(self, URL):
		r = requests.get(photo_URL)
		likes = self.get_trailing_number(r.text, "Likes")
		comments = self.get_trailing_number(r.text, "Comment")
		return (likes, comments)


	def get_account_info(self, URL):
		acc_r = requests.get(URL)
		followers = self.get_trailing_number(acc_r.text, "Followers")
		following = self.get_trailing_number(acc_r.text, "Following")
		posts = self.get_trailing_number(acc_r.text, "Posts")
		return (followers, following, posts)

	def update_values(self):
		try:
			likes, comments = self.get_photo_likes(photo_URL)
			followers, following, posts = self.get_account_info(account_URL)
			ipc.set_read(0, likes)
			ipc.set_read(1, comments)
			ipc.set_read(2, followers)
			ipc.set_read(3, following)
			ipc.set_read(4, posts)
		except Exception as err:
			logger.error("failed to fetch data, %s", str(err).splitlines()[0])
			ipc.set_no_data(0)
			ipc.set_no_data(1)
			ipc.set_no_data(2)
			ipc.set_no_data(3)
			ipc.set_no_data(4)

		ipc.go_parcook()

def main(argv=None):
	global logger
	logger = logging.getLogger('pdmn_instagram.py')
	logger.setLevel(logging.DEBUG)
	handler = logging.handlers.SysLogHandler(address='/dev/log', facility=SysLogHandler.LOG_DAEMON)
	handler.setLevel(logging.WARNING)
	formatter = logging.Formatter('%(filename)s: [%(levelname)s] %(message)s')
	handler.setFormatter(formatter)
	logger.addHandler(handler)

	if 'ipc' not in globals():
		global ipc
		sys.path.append('/opt/szarp/lib/python')
		from test_ipc import TestIPC
		ipc = TestIPC("%s"%BASE_PREFIX, "/opt/szarp/%s/pdmn_instagram.py"%BASE_PREFIX)

	ex = InstaReader()
	time.sleep(10)

	while True:
		ex.update_values()
		time.sleep(7200)


# end of main()

if __name__ == "__main__":
	sys.exit(main())
