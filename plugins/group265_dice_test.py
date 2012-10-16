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
c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile, args=['-p', 'plugins/'])
atexit.register(force_shell_termination, shell_process=c)

c.timeout = 3

c.sendline("roll 6")

assert c.expect("Rolling") == 0, "Did not print"

dice_regex = ".*(\d+).*(\d+)"

(d1, d2) = shellio.parse_regular_expression(c, dice_regex)

print d1
print d2

assert (d2 <= d1)
assert (d2 >= 1)


c.sendline("roll 0")

assert c.expect("Please enter a valid number.") == 0, "Did not print"

c.sendline("exit")
assert c.expect_exact("exit\r\n") == 0, "Shell output extraneous characters"

shellio.success()
