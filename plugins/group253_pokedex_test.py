#!/usr/bin/python
#
# pokedex_test: tests the pokedex command
# 
# Test the pokedex command which, when run with an int 1-151
# will print the corresponding pokemon name from the 1st
# generation of pokemon.
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

# spawn an instance of the shell
c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile, args=['-p','plugins/'])
atexit.register(force_shell_termination, shell_process=c)

# set timeout for all following 'expect*' calls to 2 seconds
c.timeout = 2

#test pokedex with a valid number
c.sendline("pokedex 1")
assert c.expect("bulbasaur") == 0, "pokedex did not print correct pokemon"
c.sendline("pokedex 150")
assert c.expect("mewtwo") == 0, "pokedex did not print correct pokemon"

#test pokedex with invalid numbers
c.sendline("pokedex 155")
assert c.expect("missingno") == 0, "pokedex did not handle error case"
c.sendline("pokedex")
assert c.expect("missingno") == 0, "pokedex did not handle error case"

# end the shell program by sending it an end-of-file character
c.sendline("exit");

# ensure that no extra characters are output after exiting
assert c.expect_exact("exit\r\n") == 0, "Shell output extraneous characters"


# the test was successful
shellio.success()
