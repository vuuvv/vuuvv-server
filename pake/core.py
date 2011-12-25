"""
Python Makefile System

A task have a name, func, what module in and depends, store in the tasks.

The resposibility of the Loader is load the pakefile into memory.
If the loader loading the top level pakefile, it should parse the command line.
Since argparse in python can't handle the default sub-command, we use some
trick skill to get it. 
We use the parse_known_args to handle the command line in 3 place:
	1. Begin of the program(in main method), handle the "-f xxx.py" args.
	2. Before load the toplevel pakefile, handle the top level help arg
	3. After the pakefile loaded, handle the sub-command args
"""

import os
import sys
import imp
import types
import argparse
import inspect
from functools import wraps, partial
from gettext import gettext as _

from pake.exceptions import PakeError
from pake.settings import DEFAULT_TASK
from pake import log, settings

def empty(target, func, depends, default):
	return func, depends, default

class _dict(dict):
	pass

class TaskList(object):
	def __init__(self):
		self.lookup = {}

	def add(self, target, func=None, depends=[], default=False, arg_func=empty):
		func, depends, default = arg_func(target, func, depends, default)
		module = loader.module
		lookup = self.lookup
		if module not in lookup:
			lookup[module] = _dict()
		task_list = lookup[module]
		task_list[target] = {'func': func, 'depends': depends}
		if default:
			task_list.default = target

	def remove(self, module):
		module = module if isinstance(module, types.ModuleType) else sys.modules[module]
		try:
			del self.lookup[module]
		except KeyError:
			log.warning("The %s should not be remove" % name) 

	def find(self, module, target=None):
		module = module if isinstance(module, types.ModuleType) else sys.modules[module]
		try:
			task_list = self.lookup[module]
			default = getattr(task_list, 'default', DEFAULT_TASK)
			target = target if target is not None else default
			return task_list[target]
		except (KeyError, AttributeError):
			raise PakeError("Can't find task %s in %s" % (target, module.__file__))

	def run(self, target, module=None, completed=set()):
		if module is None: module = loader.module
		task = tasks.find(module, target)
		func = task['func']
		depends = task['depends']
		for depend in depends:
			if depend not in completed:
				completed |= self.run(depend)
		func()
		completed.add(target)
		return completed

tasks = TaskList()

class Task(object):
	def __init__(self, depends=[], target=None, default=False, help=None):
		self.target = target
		self.depends = depends
		self.default = default
		self.help = help

	def __call__(self, func):
		if self.target is None:
			self.target = func.__name__

		helper = partial(func, self.depends, self.target)
		_task = wraps(func)(helper)
		tasks.add(self.target, _task, self.depends, 
				self.default, arg_func=self.arg_func)
		return self.target

	def arg_func(self, target, func, depends, default):
		help = self.help if self.help is not None else target
		parser = loader.subparser.add_parser(target, help=help)
		return func, depends, default

def task(depends, **kwargs):
	return Task(depends, **kwargs)

class Loader(object):
	def __init__(self):
		self.module = None
		self.toplevel = None
		self._arg_parser = None
		self.subparser = None
		self.argv = sys.argv[1:]
		self.args = None

	@property
	def arg_parser(self):
		if self._arg_parser is None:
			parser = argparse.ArgumentParser(
					description="Python Makefile System", 
					prog="pake", add_help=False)
			parser.add_argument(
					'-f', '--file', 
					type=str, default=settings.PAKEFILE_NAME, 
					help='specified a pakefile')
			parser.add_argument(
					'-v', '--verbose', 
					type=int, choices=[0, 1, 2], default=1)
			self._arg_parser = parser
		return self._arg_parser

	def main(self):
		parser = self.arg_parser

		# use this code to implement default subcommand
		# first use a 'action' to check is there have a target
		# then remove the 'action'
		action = parser.add_argument(
				'target', type=str, nargs="?", default=None)
		self.args, self.argv = parser.parse_known_args(self.argv)
		args = self.args
		target = self.args.target
		del self.args.target
		log.set_verbosity(args.verbose)
		parser._remove_action(action)

		self.load(args.file, target=target, toplevel=True)

	def get_output_dir(self):
		top = os.path.abspath(self.toplevel)
		cur = os.path.abspath(os.getcwd())
		return os.path.join(top, settings.DEFAULT_BUILD_DIR, cur[len(top)+len(os.path.sep):])

	def insert_build_in(self, module):
		from pake import rules
		module.output_dir = self.get_output_dir()
		setattr(module, 'rule', rules)

	def load_pakefile(self, name):
		module = types.ModuleType(name)
		self.insert_build_in(module)
		self.module = module
		sys.modules[name] = module
		self.module = imp.reload(module)
		return self.module

	def init_module(self):
		try:
			type = self.module.type
		except AttributeError:
			self.module.type = settings.DEFAULT_TYPE
			type = self.module.type

		try:
			m = __import__('pake.types.' + type.lower(), fromlist=['init'])
			m.init()
		except ImportError:
			raise PakeError("%s have not implemented in module types" % type)

	def parse_toplevel_help(self):
		pass

	def load(self, pakefile, target=None, toplevel=False):
		path = os.path.abspath(pakefile)
		if not os.path.exists(pakefile):
			raise PakeError('File "%s" is not exists' % path)
		directory, file = os.path.split(path)
		name, ext = os.path.splitext(file)

		old_dir = os.getcwd()
		old_module = self.module
		sys.path.insert(0, directory)
		os.chdir(directory)
		log.info("-"*70)
		log.info(">> %s" % directory)

		if toplevel:
			self.toplevel = directory
			parser = self.arg_parser
			parser.add_argument(
					'-h', '--help', 
					action='store_true', 
					help=_('show this help message and exit'))
			if target is None:
				self.args, self.argv = parser.parse_known_args(self.argv, self.args)
			self.subparser = parser.add_subparsers(help="target help", dest="target")

		module = self.load_pakefile(name)
		self.init_module()

		if toplevel: 
			if target is None:
				if self.args.help:
					parser.print_help()
					parser.exit()
				task_list = tasks.lookup[module]
				target = getattr(task_list, 'default', DEFAULT_TASK)
			self.argv.insert(0, target)
			self.args, self.argv = parser.parse_known_args(self.argv, self.args)
			target = self.args.target
		tasks.run(target)

		log.info("<< %s" % directory)
		log.info("*"*70)
		os.chdir(old_dir)
		sys.path.remove(directory)
		tasks.remove(module)
		self.module = old_module

	def cd(self, directory, target=None, toplevel=False):
		path = os.path.abspath(directory)
		if not os.path.isdir(path):
			raise exceptions.PakeError('Directory "%s" is not exists' % path)
		pakefile = os.path.join(path, settings.PAKEFILE_NAME)
		self.load(pakefile, target, toplevel)

	def generate_args(self, args):
		module = self.module
		return dict([(arg, getattr(module, arg)) for arg in args if hasattr(module, arg)])

loader = Loader()
