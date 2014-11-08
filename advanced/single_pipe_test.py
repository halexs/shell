#!/usr/bin/python
#
# Block header comment
#
#
import sys, imp, atexit
sys.path.append("/home/courses/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the shell process is terminated
def force_shell_termination(shell_process):
	c.close(force=True)

#pulling in the regular expression and other definitions
definitions_scriptname = sys.argv[1]
def_module = imp.load_source('', definitions_scriptname)
logfile = None
if hasattr(def_module, 'logfile'):
    logfile = def_module.logfile

#spawn an instance of the shell
c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile)
atexit.register(force_shell_termination, shell_process=c)

# set timeout for all following 'expect*' calls to 2 seconds
c.timeout = 2

# ensure that shell prints expected prompt
assert c.expect(def_module.prompt) == 0, "Shell did not print expected prompt"

# run a command that uses a single pipe
c.sendline("echo hello world! | wc")

# output wc of "hello world!"
assert c.expect_exact("      1       2      13") == 0, " Shell did not print the expected prompt"

# run a command that uses a singl epipe
c.sendline("grep mail /etc/passwd | cut -d: -f 6")

# finds the line that contains mail in /etc/passwd, delimits it starting from the : and gets field 6 of the line
# which is the home dir of that user
assert c.expect_exact("/var/spool/mail") == 0, "Shell did not print the expected prompt"

# run another pipeline
c.sendline("echo /this/is/my/complicated/path/here | cut -d/ -f-4,6-")

# and check for output again
assert c.expect_exact("/this/is/my/path/here") == 0, "Shell did not print the expected prompt"

#exit
c.sendline("exit")
assert c.expect_exact("exit\r\n") == 0, "Shell output extraneous characters"


shellio.success()
