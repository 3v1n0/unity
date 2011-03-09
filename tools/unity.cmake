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
import glob
from optparse import OptionParser
import os
import shutil
import signal
import subprocess
import sys
import time

home_dir = os.path.expanduser("~%s" % os.getenv("SUDO_USER"))
supported_prefix = "/usr/local"

well_known_local_path = ("%s/share/unity" % supported_prefix,
                         "%s/share/ccsm/icons/*/*/*/*unity*" % supported_prefix,
                         "%s/share/locale/*/LC_MESSAGES/*unity*" % supported_prefix,
                         "%s/.compiz-1/*/*unity*" % home_dir,
                         "%s/.gconf/schemas/*unity*" % home_dir,
                         "%s/lib/*unity*"  % supported_prefix,
                         "%s/share/dbus-1/services/*Unity*"  % supported_prefix,
                         "%s/bin/*unity*"  % supported_prefix,
                         "%s/share/man/man1/*unity*"  % supported_prefix,
                         "%s/lib/*nux*"  % supported_prefix,
                         "%s/pkgconfig/nux*"  % supported_prefix,
                         "%s/include/Nux*"  % supported_prefix
                         )


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
    
    if not client:
		print "WARNING: no gconf client found. No reset will be done"
		return
    
    # get current compiz profile to know if we need to switch or not
    # as compiz is setting that as a default key schema each time you
    # change the profile, the key isn't straightforward to get and set
    # as compiz set a new schema instead of a value..
    current_profile_schema = client.get_schema("/apps/compizconfig-1/current_profile")
    
    # default value to not force reset if current_profile is unset
    current_profile_gconfvalue = ""
    if current_profile_schema:
		current_profile_gconfvalue = current_profile_schema.get_default_value()

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
        unity_instance = process_and_start_unity (verbose, debug, compiz_args, log_file)
        subprocess.Popen(["killall", "unity-panel-service"])
        unity_instance.wait()
    except KeyboardInterrupt, e:
        try:
            os.kill(unity_instance.pid, signal.SIGKILL)
        except:
            pass
        unity_instance.wait()
    sys.exit(unity_instance.returncode)

def reset_to_distro():
	''' remove all known default local installation path '''
	
	# check if we are root, we need to be root
	if os.getuid() != 0:
		print "Error: You need to be root to remove your local unity installation"
		return 1
	error = False
	
	for filedir in well_known_local_path:
		for elem in glob.glob(filedir):
			try:
				shutil.rmtree(elem)
			except OSError, e:
				if os.path.isfile(elem):
					os.remove(elem)
				else:
					print "ERROR: Cannot remove %s", e
					error = True
	
	if error:
		print "See above: some error happened and you should clean them before trying to restart unity"
		return 1
	else:
		print "Unity local install cleaned, you can now restart unity"
		return 0

if __name__ == '__main__':
    usage = "usage: %prog [options]"
    parser = OptionParser(version= "%prog @UNITY_VERSION@", usage=usage)

    parser.add_option("--advanced-debug", action="store_true",
                      help="Run unity under debugging to help debugging an issue. /!\ Only if devs ask for it.")    
    parser.add_option("--distro", action="store_true",
                      help="Remove local build if present with default values to return to the package value (this doesn't run unity and need root access)")    
    parser.add_option("--log", action="store",
                      help="Store log under filename.")
    parser.add_option("--replace", action="store_true",
                      help="Run unity /!\ This is for compatibility with other desktop interfaces and acts the same as running unity without --replace")
    parser.add_option("--reset", action="store_true",
                      help="Reset the unity profile in compiz and restart it.")    
    parser.add_option("-v", "--verbose", action="store_true",
                      help="Get additional debug output from unity.")
    (options, args) = parser.parse_args()

    set_unity_env()

    if options.distro:
		sys.exit(reset_to_distro())
    
    if options.reset:
        reset_unity_compiz_profile ()
	
	if options.replace:
		print ("WARNING: This is for compatibility with other desktop interfaces please use unity without --replace")
			
    run_unity (options.verbose, options.advanced_debug, args, options.log)
