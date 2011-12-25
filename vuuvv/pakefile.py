from pake import cc, task, cd
from distutils import sysconfig
import os

libraries = [
	'gdi32', 'user32', 'kernel32', 'shell32', 'imm32', 'ws2_32',
]

include_dirs = ['.', sysconfig.get_python_inc()] 

vuuvv_srcs = [
	['vuuvv.c', ['vuuvv.h']],
	['eventloop.c', ['eventloop.h']],
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
