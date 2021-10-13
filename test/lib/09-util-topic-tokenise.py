#!/usr/bin/env python3

from dimq_test_helper import *

rc = 1

client_args = sys.argv[1:]
env = dict(os.environ)
env['LD_LIBRARY_PATH'] = '../../lib:../../lib/cpp'
try:
    pp = env['PYTHONPATH']
except KeyError:
    pp = ''
env['PYTHONPATH'] = '../../lib/python:'+pp

client = dimq_test.start_client(filename=sys.argv[1].replace('/', '-'), cmd=client_args, env=env)
client.wait()
exit(client.returncode)
