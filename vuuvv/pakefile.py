from pake import cc, task, cd
from distutils import sysconfig
import os

libraries = [
	'gdi32', 'user32', 'kernel32', 'shell32', 'imm32', 'ws2_32',
]

macros = [
	('_CRT_SECURE_NO_WARNINGS', True), ('DEBUG', True)
]

include_dirs = ['.', sysconfig.get_python_inc()] 

headers = [
	'vuuvv.h', 
	'defines.h',
	'log.h',
	'common.h',
	'stream.h',
	'config.h',
	'eventloop.h', 
	'event_iocp.h',
	'os/win32/defines.h',
]

vuuvv_srcs = [
	['vuuvv.c', headers],
	['eventloop.c', headers],
	['log.c', headers],
	['stream.c', headers],
	['config.c', headers],
	['event_iocp.c', headers],
	['os/win32/errno.c', headers],
]
vuuvv_objs = [rule.c(*file) for file in vuuvv_srcs]
vuuvv_pyd = rule.pyd(vuuvv_objs, 'vuuvv')

@task([vuuvv_pyd], default=True)
def build(depends, target):
	pass

@task([vuuvv_pyd])
def run(depends, target):
	import sys
	sys.path.insert(0, cc.libs_dir())
	import vuuvv
	vuuvv.test("abc", 123)

@task([vuuvv_pyd])
def test(depends, target):
	import sys
	sys.path.insert(0, cc.libs_dir())
	_test("tests")

def _test(directory):
	import os, sys, unittest, logging
	logging.basicConfig(level=logging.DEBUG)
	loader = unittest.TestLoader()
	suite = loader.discover(directory)
	runner = unittest.TextTestRunner(verbosity=2)
	runner.run(suite)
