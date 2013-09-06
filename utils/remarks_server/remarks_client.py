#!/usr/bin/python
# -*- encoding: utf-8 -*-

import xmlrpclib
import argparse

cmd_parser = argparse.ArgumentParser()
cmd_parser.add_argument("LOGIN", help="Remarks database login", type=str)
cmd_parser.add_argument("PASSWD_MD5", help="Remarks database password md5", type=str)
cmd_parser.add_argument("SERVER", help="Remarks server address", type=str)
args = cmd_parser.parse_args()

if (args.LOGIN and args.PASSWD_MD5):
        if args.SERVER:
                server = args.SERVER
        else:
                server = 'https://localhost:7998/'

        s = xmlrpclib.Server(server)
        s.login(args.LOGIN, args.PASSWD_MD5)
        s.prefix_sync()
