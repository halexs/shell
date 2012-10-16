#!/usr/bin/python
#
# Block header comment
#
#
import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the shell process is terminated
def force_shell_termination(shell_process):
	c.close(force=True)

#pulling in the regular expression and other definitions
definitions_scriptname = sys.argv[1]
plugin_dir = sys.argv[2]
def_module = imp.load_source('', definitions_scriptname)
logfile = None
if hasattr(def_module, 'logfile'):
    logfile = def_module.logfile

#spawn an instance of the shell
c = pexpect.spawn(def_module.shell + plugin_dir, drainpty=True, logfile=logfile, )

atexit.register(force_shell_termination, shell_process=c)

c.timeout = 20

#
# a regexp matching a fortune when printed using the 'fortune' command
# must capture (fortune_string)
#
fortune_regex =  ".+?\.\r\n"

#
# Send the 'fortune' command
#
c.sendline("fortune")

#
# Run the 'fortune' command 5 times and check output against the regex
#
assert c.expect(fortune_regex) == 0

c.sendline("fortune")

assert c.expect(fortune_regex) == 0

c.sendline("fortune")

assert c.expect(fortune_regex) == 0

c.sendline("fortune")

assert c.expect(fortune_regex) == 0

c.sendline("fortune")

assert c.expect(fortune_regex) == 0

shellio.success()
