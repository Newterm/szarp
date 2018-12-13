#!/usr/bin/python
import socket
import psutil
import argparse
import sys
import struct
import binascii

TCP_IP = "localhost"
TCP_PORT = 9002
BUFFER_SIZE = 1024

ERROR_EXIT_CODE = 2
CORRECT_EXIT_CODE = 0

def connect2Base(sock, base):
	sock.send("connect 0 " + base + "\n")
	receive = sock.recv(BUFFER_SIZE)
	if receive[0] == 'e':
		return ERROR_EXIT_CODE
	return CORRECT_EXIT_CODE

def checkRemotes(sock, remotes):
	exit_status = 1
	for remote in remotes:
		exit_status = connect2Base(sock, remote)
		if exit_status == ERROR_EXIT_CODE:
			print "ERROR:",remote, "base not found"
		else:
			print "OK"
	return exit_status

def split2BaseAndParam(param):
	middle = param.find(":")
	base = param[0 : middle]
	param = param[middle + 1:]
	return base, param

def getValueFromIKS(sock, base, param):
	# connect to base
	if connect2Base(sock, base) == ERROR_EXIT_CODE:
		print base, "base not found"
		sys.exit(ERROR_EXIT_CODE)

	# get time last param
	sock.send("get_latest 1 " + param + " 10s\n")
	receive = sock.recv(BUFFER_SIZE)
	if receive[0] == 'e':
		print "ERROR: error from iks:", receive[:-1]
		sys.exit(ERROR_EXIT_CODE)
	start_timestamp = receive.split(" ")[2].strip()
	end_timestamp = str(int(start_timestamp) + 1)
	start_timestamp = str(int(start_timestamp) - 9)

	# get value of last param
	sock.send("get_history 2 "+ param + " 10s " + start_timestamp + " " + end_timestamp + "\n")
	data = sock.recv(BUFFER_SIZE).split('"data":"')[1].split('"')[0]

	# check if there is any data
	if data == "":
		print "no data"
		sys.exit(ERROR_EXIT_CODE)

	# add missing padding ( required to decode base64 )
	missing_padding = len(data) % 4
	if missing_padding != 0:
		data += b'=' * (4 - missing_padding)

	# decode base64 to binary
	data = data.decode('base64')

	# read data as float and cast to int32
	data = int((struct.unpack('f', data)[0]))
	return data

def splitThresholds(thresholds):
	if thresholds is None:
		return thresholds, thresholds, thresholds
	else:
		thresholds = thresholds.split(",")
		return int(thresholds[0]), int(thresholds[1]), int(thresholds[2])

def checkParam(sock, param, thresholds):
	base, param = split2BaseAndParam(param)
	value = getValueFromIKS(sock, base, param)
	print value

	tmin, tmax, ttimeout = splitThresholds(thresholds)

	# check if value is required
	if value == "unknown":
		return ERROR_EXIT_CODE
	elif thresholds is not None:
		if int(value) < tmin or int(value) < tmax:
			return ERROR_EXIT_CODE
	return CORRECT_EXIT_CODE

if __name__ == "__main__":
	parser = argparse.ArgumentParser(
	description="The script check if IKS Server is running and if current params are not None")

	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument('-p', '--param', help="Full name param")
	group.add_argument('-r', '--remotes', nargs="*", help="List of SZARP bases")

	parser.add_argument('-t', '--thresholds', help='Ex. min,max,timeout', required=False)
	args = parser.parse_args()

	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	if sock.connect_ex((TCP_IP, TCP_PORT)) != 0:
		print "IKS Server is not running"
		sys.exit(ERROR_EXIT_CODE)

	if args.remotes is not None:
		exit_code = checkRemotes(sock, args.remotes)
	else:
		exit_code = checkParam(sock, args.param, args.thresholds)
	sys.exit(exit_code)
