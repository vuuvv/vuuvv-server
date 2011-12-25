from distutils.dist import Distribution as _Dist
from distutils.fancy_getopt import FancyGetopt
from distutils.dist import command_re
from distutils.errors import *

from pake import log

class Distribution(_Dist):
	"""Almost moved from distutils module and remove the help options"""

	def parse_command_line(self):
		#
		# We now have enough information to show the Macintosh dialog
		# that allows the user to interactively specify the "command line".
		#
		toplevel_options = self._get_toplevel_options()

		# We have to parse the command line a bit at a time -- global
		# options, then the first command, then its options, and so on --
		# because each command will be handled by a different class, and
		# the options that are valid for a particular class aren't known
		# until we have loaded the command class, which doesn't happen
		# until we know what the command is.

		self.commands = []
		parser = FancyGetopt(toplevel_options + self.display_options)
		parser.set_negative_aliases(self.negative_opt)
		parser.set_aliases({'licence': 'license'})
		args = parser.getopt(args=self.script_args, object=self)
		option_order = parser.get_option_order()
		log.set_verbosity(self.verbose)

		# for display options we return immediately
		if self.handle_display_options(option_order):
			return
		while args:
			args = self._parse_command_opts(parser, args)
			if args is None:            # user asked for help (and got it)
				return

		# All is well: return true
		return True

	def _parse_command_opts(self, parser, args):
		"""Almost moved from distutils module and remove the help options"""
		# late import because of mutual dependence between these modules
		from distutils.cmd import Command

		# Pull the current command from the head of the command line
		command = args[0]
		if not command_re.match(command):
			raise SystemExit("invalid command name '%s'" % command)
		self.commands.append(command)

		# Dig up the command class that implements this command, so we
		# 1) know that it's a valid command, and 2) know which options
		# it takes.
		try:
			cmd_class = self.get_command_class(command)
		except DistutilsModuleError as msg:
			raise DistutilsArgError(msg)

		# Require that the command class be derived from Command -- want
		# to be sure that the basic "command" interface is implemented.
		if not issubclass(cmd_class, Command):
			raise DistutilsClassError(
				  "command class %s must subclass Command" % cmd_class)

		# Also make sure that the command object provides a list of its
		# known options.
		if not (hasattr(cmd_class, 'user_options') and
				isinstance(cmd_class.user_options, list)):
			raise DistutilsClassError(("command class %s must provide " +
				   "'user_options' attribute (a list of tuples)") % \
				  cmd_class)

		# If the command class has a list of negative alias options,
		# merge it in with the global negative aliases.
		negative_opt = self.negative_opt
		if hasattr(cmd_class, 'negative_opt'):
			negative_opt = negative_opt.copy()
			negative_opt.update(cmd_class.negative_opt)

		help_options = []


		# All commands support the global options too, just by adding
		# in 'global_options'.
		parser.set_option_table(self.global_options +
								cmd_class.user_options +
								help_options)
		parser.set_negative_aliases(negative_opt)
		(args, opts) = parser.getopt(args[1:])

		# Put the options from the command-line into their official
		# holding pen, the 'command_options' dictionary.
		opt_dict = self.get_option_dict(command)
		for (name, value) in vars(opts).items():
			opt_dict[name] = ("command line", value)

		return args

