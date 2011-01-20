#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright (C) 2010 Canonical
#
# Authors:
#  Didier Roche <didrocks@ubuntu.com>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; version 3.
#
# This program is distributed in the hope that it will be useful, but WITHOUTa
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

import gconf
from optparse import OptionParser
import os
import signal
import subprocess
import sys
import time

from autopilot import UnityTestRunner

def set_unity_env ():
    '''set variable environnement for unity to run'''

    os.environ['COMPIZ_CONFIG_PROFILE'] = 'ubuntu'
    
    if not 'DISPLAY' in os.environ:
        # take an optimistic chance and warn about it :)
        print "WARNING: no DISPLAY variable set, setting it to :0"
        os.environ['DISPLAY'] = ':0'

def reset_unity_compiz_profile ():
    '''reset the compiz/unity profile to a vanilla one'''
    
    client = gconf.client_get_default()
    
    # get current compiz profile to know if we need to switch or not
    # as compiz is setting that as a default key schema each time you
    # change the profile, the key isn't straightforward to get and set
    # as compiz set a new schema instead of a value..
    current_profile_schema = client.get_schema("/apps/compizconfig-1/current_profile")
    current_profile_gconfvalue = client.get_schema("/apps/compizconfig-1/current_profile").get_default_value()
    if current_profile_gconfvalue.get_string() == 'unity':
        print "WARNING: Unity currently default profile, so switching to metacity while resetting the values"
        subprocess.Popen(["metacity", "--replace"]) #TODO: check if compiz is indeed running
        # wait for compiz to stop
        time.sleep(2)
        current_profile_gconfvalue.set_string('fooo')
        current_profile_schema.set_default_value(current_profile_gconfvalue)
        client.set_schema("/apps/compizconfig-1/current_profile", current_profile_schema)
        # the python binding doesn't recursive-unset right
        subprocess.Popen(["gconftool-2", "--recursive-unset", "/apps/compiz-1"]).communicate()
    subprocess.Popen(["gconftool-2", "--recursive-unset", "/apps/compizconfig-1/profiles/unity"]).communicate()
    

def process_and_start_unity (verbose, debug, compiz_args, log_file):
    '''launch unity under compiz (replace the current shell in any case)'''
    
    cli = []
    
    if debug:
        # we can do more check later as if it's in PATH...
        if not os.path.isfile('/usr/bin/gdb'):
            print("ERROR: you don't have gdb in your system. Please install it to run in advanced debug mode.")
            sys.exit(1)
        elif 'DESKTOP_SESSION' in os.environ:
            print("ERROR: it seems you are under a graphical environment. That's incompatible with executing advanced-debug option. You should be in a tty.")
            sys.exit(1)
        else:
            cli.extend(['gdb', '--args'])
    
    cli.extend(['compiz', '--replace'])
    if options.verbose:
        cli.append("--debug")    
    if args:
        cli.extend(compiz_args)
    
    if log_file:
        cli.extend(['2>&1', '|', 'tee', log_file])

    # shell = True as it's the simpest way to | tee.
    # In this case, we need a string and not a list
    # FIXME: still some bug with 2>&1 not showing everything before wait()
    return subprocess.Popen(" ".join(cli), env=dict(os.environ), shell=True)
    

def run_unity (verbose, debug, compiz_args, log_file):
    '''run the unity shell and handle Ctrl + C'''

    try:
        if options.autopilot:
            os.putenv('UNITY_AUTOPILOT', 1)
            unity_instance = process_and_start_unity (verbose, debug, compiz_args, log_file)
            runner = UnityTestRunner(unity_instance)
            runner.start()
        else:
            unity_instance = process_and_start_unity (verbose, debug, compiz_args, log_file)
        unity_instance.wait()
    except KeyboardInterrupt, e:
        try:
            os.kill(unity_instance.pid, signal.SIGKILL)
        except:
            pass
        unity_instance.wait()
    sys.exit(unity_instance.returncode)

if __name__ == '__main__':
    usage = "usage: %prog [options]"
    parser = OptionParser(version= "%prog @UNITY_VERSION@", usage=usage)

    parser.add_option("--advanced-debug", action="store_true",
                      help="Run unity under debugging to help debugging an issue. /!\ Only if devs ask for it.")    
    parser.add_option("--log", action="store",
                      help="Store log under filename.")
    parser.add_option("--replace", action="store_true",
                      help="Run unity /!\ This is for compatibility with other desktop interfaces and acts the same as running unity without --replace")
    parser.add_option("--reset", action="store_true",
                      help="Reset the unity profile in compiz and restart it.")  
    parser.add_option("--autopilot", action="store_true",
                     help="Run unity in an automated test mode. Implies --reset.")
    parser.add_option("-v", "--verbose", action="store_true",
                      help="Get additional debug output from unity.")
    (options, args) = parser.parse_args()
    
    # sets --reset to be True if it's not already,
    #if --autopilot is set.
    if options.autopilot and not options.reset:
        options.reset = True

    set_unity_env()
    if options.reset:
        reset_unity_compiz_profile ()
	
	if options.replace:
		print ("WARNING: This is for compatibility with other desktop interfaces please use unity without --replace")
			
    run_unity (options.verbose, options.advanced_debug, args, options.log)
