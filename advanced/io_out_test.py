#!/usr/bin/python
#
# io_out_test
#
# Test that the shell correctly makes use of the
# ">" command to direct input
#
#       Requires the use of the following commands:
#
#       vim, echo, cat
#

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
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

# run a command that directs output
c.sendline("echo hey there > test")

# print the content of the file test
c.sendline("cat test")

# should output the contents of the text file "test"
assert c.expect_exact("hey there\r\n") == 0, "Shell did not print the expected prompt"

#exit
c.sendline("exit")
assert c.expect_exact("exit\r\n") == 0, "Shell output extraneous characters"


shellio.success()
