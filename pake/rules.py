import os
from functools import partial

from pake.compiler import cc
from pake.core import tasks, loader

def c(source, depends=[], default=False):
	func = partial(cc.compile, source, depends)
	target = cc.object_filename(source, output_dir=loader.module.output_dir)
	tasks.add(target, func, [], default)
	return target

def exe(objects, prog=None, default=False):
	if prog is None:
		mainobj = objects[0]
		dirname, filename = os.path.split(mainobj)
		prog, ext = os.path.splitext(filename)
	func = partial(cc.link, prog, objects)
	target = cc.executable_filename(prog, output_dir=loader.module.output_dir)
	tasks.add(target, func, objects, default)
	return target

def pyd(objects, prog, default=False):
	fullname = cc.get_py_ext_filename(prog)
	target = os.path.join(cc.libs_dir(loader.module.output_dir), fullname);
	func = partial(cc.link_py_extention, target, objects)
	tasks.add(target, func, objects, default)
	return target

