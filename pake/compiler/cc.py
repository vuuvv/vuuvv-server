import os
import sys
from distutils import ccompiler
from distutils import sysconfig
from subprocess import call
from pake import loader, log
from pake import settings

cc = ccompiler.new_compiler()

ccompile_args = ['output_dir', 'macros', 'include_dirs', 'debug']
link_args = ['output_dir', 'libraries', 'library_dirs', 'runtime_library_dirs', 'debug']
shared_object_args = link_args + ['export_symbols']

def out_of_date(target, depends):
	if not os.path.exists(target):
		return True
	target_mtime = os.path.getmtime(target)
	for source in depends:
		if not os.path.exists(source):
			raise PakeErro("source file %s not found" % source)
		source_mtime = os.path.getmtime(source)
		if source_mtime > target_mtime:
			return True
	else:
		return False

def object_filename(source, output_dir=''):
	return cc.object_filenames([source], output_dir=output_dir)[0]

def libs_dir(output_dir=None):
	if output_dir is None:
		output_dir = loader.module.output_dir
	return os.path.join(output_dir, settings.DEFAULT_LIB_DIR)

def get_py_ext_filename(ext_name, debug = False):
	r"""Convert the name of an extension (eg. "foo.bar") into the name
	of the file from which it will be loaded (eg. "foo/bar.so", or
	"foo\bar.pyd").
	"""
	from distutils.sysconfig import get_config_var
	ext_path = ext_name.split('.')
	# OS/2 has an 8 character module (extension) limit :-(
	if os.name == "os2":
		ext_path[len(ext_path) - 1] = ext_path[len(ext_path) - 1][:8]
	# extensions in debug_mode are named 'module_d.pyd' under windows
	so_ext = get_config_var('SO')
	if os.name == 'nt' and debug:
		return os.path.join(*ext_path) + '_d' + so_ext
	return os.path.join(*ext_path) + so_ext

def get_python_lib_dirs():
	lib_dirs = []

	if os.name == 'nt':
		lib_dirs.append(os.path.join(sys.exec_prefix, 'libs'))
	elif os.name == 'os2':
		lib_dirs.append(os.path.join(sys.exec_prefix, 'Config'))
	
	if sys.platform[:6] == 'cygwin' or sys.platform[:6] == 'atheos':
		if sys.excutable.startswith(os.path.join(sys.exec_prefix, "bin")):
			lib_dirs.append(os.path.join(sys.prefix, "lib", 
				"python" + sysconfig.get_python_version(), "config"))
		else:
			lib_dirs.append(".")

	sysconfig.get_config_var('Py_ENABLE_SHARED')
	if ((sys.platform.startswith('linux') or sys.platform.startswith('gnu')
		 or sys.platform.startswith('sunos'))
		and sysconfig.get_config_var('Py_ENABLE_SHARED')):
		if sys.executable.startswith(os.path.join(sys.exec_prefix, "bin")):
			# building third party extensions
			lib_dirs.append(sysconfig.get_config_var('LIBDIR'))
		else:
			# building python standard extensions
			lib_dirs.append('.')
	return lib_dirs

def get_base_name(path):
	directory, filename = os.path.split(path)
	basename, ext = os.path.splitext(filename)
	return basename

def get_python_lib_export_symbols(ext_name):
	return ['PyInit_' + get_base_name(ext_name)]

def get_python_lib():
	"""Return the list of libraries to link against when building a
	shared extension.  On most platforms, this is just 'ext.libraries';
	on Windows and OS/2, we add the Python library (eg. python20.dll).
	"""
	# The python library is always needed on Windows.  For MSVC, this
	# is redundant, since the library is mentioned in a pragma in
	# pyconfig.h that MSVC groks.  The other Windows compilers all seem
	# to need it mentioned explicitly, though, so that's what we do.
	# Append '_d' to the python import library on debug builds.
	if sys.platform == "win32":
		from distutils.msvccompiler import MSVCCompiler
		if not isinstance(cc, MSVCCompiler):
			template = "python%d%d"
			#if self.debug:
			#	template = template + '_d'
			pythonlib = (template %
				   (sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff))
			# don't extend ext.libraries, it may be shared with other
			# extensions, it is a reference to the original list
			return [pythonlib]
		else:
			return []
	elif sys.platform == "os2emx":
		# EMX/GCC requires the python library explicitly, and I
		# believe VACPP does as well (though not confirmed) - AIM Apr01
		template = "python%d%d"
		# debug versions of the main DLL aren't supported, at least
		# not at this time - AIM Apr01
		#if self.debug:
		#    template = template + '_d'
		pythonlib = (template %
			   (sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff))
		return [pythonlib]
	elif sys.platform[:6] == "cygwin":
		template = "python%d.%d"
		pythonlib = (template %
			   (sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff))
		return [pythonlib]
	elif sys.platform[:6] == "atheos":
		from distutils import sysconfig

		template = "python%d.%d"
		pythonlib = (template %
			   (sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff))
		# Get SHLIBS from Makefile
		extra = []
		for lib in sysconfig.get_config_var('SHLIBS').split():
			if lib.startswith('-l'):
				extra.append(lib[2:])
			else:
				extra.append(lib)
		return [pythonlib, "m"] + extra
	elif sys.platform == 'darwin':
		# Don't use the default code below
		return []
	elif sys.platform[:3] == 'aix':
		# Don't use the default code below
		return []
	else:
		from distutils import sysconfig
		if sysconfig.get_config_var('Py_ENABLE_SHARED'):
			pythonlib = 'python{}.{}{}'.format(
				sys.hexversion >> 24, (sys.hexversion >> 16) & 0xff,
				sys.abiflags)
			return [pythonlib]
		else:
			return []

executable_filename = cc.executable_filename

def compile(source, depends=[]):
	module = loader.module
	obj = object_filename(source, output_dir=module.output_dir)
	if out_of_date(obj, [source] + depends):
		cc.compile([source], **loader.generate_args(ccompile_args))
	else:
		log.info("%s is up to date" % obj)

def link(target, objects):
	module = loader.module
	target_file = cc.executable_filename(target, output_dir=module.output_dir)
	if out_of_date(target_file, objects):
		cc.link_executable(objects, target, **loader.generate_args(link_args))
	else:
		log.info("%s is up to date" % target_file)

def link_py_extention(target, objects):
	module = loader.module
	libdir = libs_dir(module.output_dir)
	link_args = loader.generate_args(shared_object_args)
	link_args['output_dir'] = libdir
	link_args['libraries'] = link_args.get('libraries', []) + get_python_lib()
	link_args['library_dirs'] = link_args.get('library_dirs', []) + get_python_lib_dirs()
	link_args['export_symbols'] = link_args.get('export_symbols', []) + get_python_lib_export_symbols(target)
	if out_of_date(target, objects):
		cc.link_shared_object(objects, target, **link_args)
	else:
		log.info("%s is up to date" % target)


def run(target):
	log.info("running program: %s" % target)
	call(target)


#TODO: generate setup file
#TODO: cmd like "pake cc hello.c"
#TODO: package, dist, xxx


