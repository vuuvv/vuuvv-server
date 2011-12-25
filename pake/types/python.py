import os
import sys
from functools import partial
from distutils.errors import *
from distutils.util import grok_environment_error

from pake.core import tasks, loader
from pake.misc.distutils import Distribution

dist_args = [
	'name', 'version', 'description', 'long_description', 'author',
	'author_email', 'maintainer', 'maintainer_email', 'url', 'download_url',
	'packages', 'py_modules', 'scripts', 'ext_modules', 'classifiers',
	'distclass', 'script_name', 'script_args', 'options', 'license',
	'keywords', 'platforms', 'cmdclass', 'data_files', 'package_dir',
]

commands = [
	'bdist', 'bdist_dumb', 'bdist_rpm', 'bdist_wininst', 'build', 'build_clib',
	'build_ext', 'build_py', 'build_scripts', 'check', 'clean', 
	'install', 'install_data', 'install_headers', 'install_lib', 
	'install_scripts', 'register', 'sdist', 'upload',
]

default_command = 'build'

_dist = None

def fullsplit(path, result=None):
	if result is None:
		result = []
	head, tail = os.path.split(path)
	if head == '':
		return [tail] + result
	if head == path:
		return result
	return fullsplit(head, [tail] + result)

def resolve_packages():
	module = loader.module

	packages = getattr(module, 'packages', [])
	data_files = getattr(module, 'data_files', [])
	if hasattr(module, 'src'):
		for dirpath, dirnames, filenames in os.walk(module.src):
			for i, dirname in enumerate(dirnames):
				if dirname.startswith('.'): del dirnames[i]
			if '__init__.py' in filenames:
				name = '.'.join(fullsplit(dirpath))
				if name not in packages:
					packages.append('.'.join(fullsplit(dirpath)))
			elif filenames:
				data_files.append([dirpath, [os.path.join(dirpath, f) for f in filenames]])

	module.packages = packages
	module.data_files = data_files

def init():
	resolve_packages()
	for cmd in commands:
		if cmd == default_command:
			tasks.add(cmd, default=True, arg_func=dist_arg)
		else:
			tasks.add(cmd, arg_func=dist_arg)

def dist_arg(target, func, depends=[], default=False):
	command = target
	dist = get_distribution()
	cmd = dist.get_command_obj(command)

	parser = loader.subparser.add_parser(command, 
			conflict_handler='resolve',
			help=cmd.description)
	user_options = cmd.user_options
	for opt in user_options:
		opt0, opt1, opt2 = opt
		nargs = '?'
		if opt0.endswith("="):
			opt0 = opt0[:-1]
			nargs = 1
		if opt1 is None:
			parser.add_argument('--'+opt0, nargs=nargs, help=opt2)
		else:
			parser.add_argument('-'+opt1, '--'+opt0, nargs=nargs, help=opt2)
	func = partial(run_command, dist, command)
	return func, depends, default

def get_distribution():
	global _dist

	if _dist is None:
		_dist = create_distribution()
	return _dist

def create_distribution():
	args = loader.generator_args(dist_args)
	klass = args.get('distclass')
	if klass:
		del agrs['distclass']
	else:
		klass = Distribution

	if 'script_name' not in args:
		args['script_name'] = os.path.basename(sys.argv[0])
	if 'script_args' not in args:
		args['script_args'] = sys.argv[1:]

	try:
		dist = klass(args)
	except DistutilsSetupError as msg:
		if 'name' not in args:
			raise SystemExit("error in pakefile.py: %s" % msg)
		else:
			raise SystemExit("error in %s setup command: %s" %
				(args['name'], msg))
	try:
		dist.parse_command_line()
	except DistutilsArgError as msg:
		raise SystemExit("error: %s" % msg)

	return dist

def run_command(dist, command):
	try:
		dist.run_command(command)
	except KeyboardInterrupt:
		raise SystemExit("interrupted")
	except (IOError, os.error) as exc:
		error = grok_environment_error(exc)
		raise SystemExit(error)
	except (DistutilsError, CCompilerError) as msg:
		raise SystemExit("error: " + str(msg))

