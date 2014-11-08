#!/usr/bin/python
#
# io_in_test
#
# Test that the shell correctly makes use of the
# "<" command to direct input
#
#       Requires the use of the following commands:
#
#       vim, cat
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

# open a new (empty file named test2)
c.sendline("vi test2")

# inserte the line "hello world" into the file
c.sendline("ihello world")

#return to vi's command prompt
c.sendcontrol('[')

#save and exit the new file
c.sendline(":wq")

# run a command that directs input
c.sendline("cat < test2")

# should output the contents of the text file "test2"
assert c.expect_exact("hello world") == 0, "Shell did not print the expected prompt"

#exit
c.sendline("exit")
assert c.expect_exact("exit\r\n") == 0, "Shell output extraneous characters"


shellio.success()
