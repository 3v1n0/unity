#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright (C) 2010, 2015 Canonical
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

import glob
from optparse import OptionParser
import os
import re
import shutil
import signal
import subprocess
import sys
import time

DEFAULT_COMMAND = "compiz --replace"
home_dir = os.path.expanduser("~%s" % os.getenv("SUDO_USER"))
supported_prefix = "/usr/local"

well_known_local_path = ("%s/share/locale/*/LC_MESSAGES/*unity*" % supported_prefix,
                         "%s/share/man/man1/*unity*" % supported_prefix,
                         "%s/lib/*unity*" % supported_prefix,
                         "%s/share/dbus-1/services/*Unity*" % supported_prefix,
                         "%s/bin/*unity*" % supported_prefix,
                         "%s/include/Unity*"  % supported_prefix,
                         "%s/lib/pkgconfig/unity*"  % supported_prefix,
                         "%s/.compiz-1/*/*networkarearegion*" % home_dir,
                         "%s/.config/compiz-1/gsettings/schemas/*networkarearegion*" % home_dir,
                         "%s/.gconf/schemas/*networkarearegion*" % home_dir,
                         "%s/.compiz-1/*/*unity*" % home_dir,
                         "%s/.config/compiz-1/gsettings/schemas/*unity*" % home_dir,
                         "%s/.gconf/schemas/*unity*" % home_dir,
                         "%s/share/ccsm/icons/*/*/*/*unity*" % supported_prefix,
                         "%s/share/unity" % supported_prefix,
                         "%s/.compiz-1/unity*" % home_dir,
                         "%s/lib/*nux*"  % supported_prefix,
                         "%s/lib/pkgconfig/nux*"  % supported_prefix,
                         "%s/include/Nux*"  % supported_prefix
                         )


def set_unity_env ():
    '''set variable environnement for unity to run'''

    os.environ['COMPIZ_CONFIG_PROFILE'] = 'ubuntu'

    if not 'DISPLAY' in os.environ:
        # take an optimistic chance and warn about it :)
        print("WARNING: no DISPLAY variable set, setting it to :0")
        os.environ['DISPLAY'] = ':0'

def reset_launcher_icons ():
    '''Reset the default launcher icon and restart it.'''
    subprocess.Popen(["gsettings", "reset" ,"com.canonical.Unity.Launcher" , "favorites"])

def is_upstart_session():
  return 'UPSTART_SESSION' in os.environ.keys() and len(os.environ['UPSTART_SESSION'])

def process_and_start_unity (compiz_args):
    '''launch unity under compiz (replace the current shell in any case)'''

    cli = []

    if options.debug_mode > 0:
        # we can do more check later as if it's in PATH...
        if not os.path.isfile('/usr/bin/gdb'):
            print("ERROR: you don't have gdb in your system. Please install it to run in advanced debug mode.")
            sys.exit(1)
        elif options.debug_mode == 1:
            cli.extend(['gdb', '-ex', 'run', '-ex', '"bt full"', '--batch', '--args'])
        elif 'DESKTOP_SESSION' in os.environ:
            print("ERROR: it seems you are under a graphical environment. That's incompatible with executing advanced-debug option. You should be in a tty.")
            sys.exit(1)
        else:
            cli.extend(['gdb', '--args'])

    if options.compiz_path:
        cli.extend([options.compiz_path, '--replace'])
    else:
        cli.extend(DEFAULT_COMMAND.split())

    if options.verbose:
        cli.append("--debug")
    if compiz_args:
        cli.extend(compiz_args)

    if options.log:
        cli.extend(['2>&1', '|', 'tee', options.log])

    if is_upstart_session():
      if b'start/running' in subprocess.check_output("status unity7".split()):
        subprocess.call("stop unity7".split())

    # kill a previous compiz if was there (this is a hack as compiz can
    # sometimes get stuck and not exit on --replace)
    display = "DISPLAY=" + os.environ["DISPLAY"]
    pids = [pid for pid in os.listdir("/proc") if pid.isdigit()]

    for pid in pids:
        try:
            pid_path = os.path.join("/proc", pid)
            cmdline = open(os.path.join(pid_path, "cmdline"), "rb").read()
            if re.match(rb"^compiz\b", cmdline):
                compiz_env = open(os.path.join(pid_path, "environ"), "rb").read()
                if display in compiz_env.decode(sys.getdefaultencoding()):
                    subprocess.call(["kill", "-9", pid])
        except IOError:
            continue

    run_command = " ".join(cli)

    if is_upstart_session() and run_command == DEFAULT_COMMAND and not options.ignore_upstart:
      return subprocess.Popen("start unity7".split())
    else:
      # shell = True as it's the simpest way to | tee.
      # In this case, we need a string and not a list
      # FIXME: still some bug with 2>&1 not showing everything before wait()
      return subprocess.Popen(" ".join(cli), env=dict(os.environ), shell=True)


def run_unity (compiz_args):
    '''run the unity shell and handle Ctrl + C'''

    try:
        options.debug_mode = 2 if options.advanced_debug else 1 if options.debug else 0
        if is_upstart_session(): subprocess.call(["stop", "unity-panel-service"])
        unity_instance = process_and_start_unity (compiz_args)
        if is_upstart_session(): subprocess.call(["start", "unity-panel-service"])
        unity_instance.wait()
    except KeyboardInterrupt as e:
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
        print("Error: You need to be root to remove your local unity installation")
        return 1
    error = False

    for filedir in well_known_local_path:
        for elem in glob.glob(filedir):
            try:
                shutil.rmtree(elem)
            except OSError as e:
                if os.path.isfile(elem) or os.path.islink(elem):
                    os.remove(elem)
                else:
                    print("ERROR: Cannot remove %s: %s" % (elem, e))
                    error = True

    if error:
        print("See above: some error happened and you should clean them before trying to restart unity")
        return 1
    else:
        print("Unity local install cleaned, you can now restart unity")
        return 0

if __name__ == '__main__':
    usage = "usage: %prog [options]"
    parser = OptionParser(version= "%prog @UNITY_VERSION@", usage=usage)

    parser.add_option("--advanced-debug", action="store_true",
                      help="Run unity under debugging to help debugging an issue. /!\ Only if devs ask for it.")
    parser.add_option("--compiz-path", action="store", dest="compiz_path",
                      help="Path to compiz. /!\ Only if devs ask for it.")
    parser.add_option("--debug", action="store_true",
                      help="Run unity under gdb and print a backtrace on crash. /!\ Only if devs ask for it.")
    parser.add_option("--distro", action="store_true",
                      help="Remove local build if present with default values to return to the package value (this doesn't run unity and need root access)")
    parser.add_option("--log", action="store",
                      help="Store log under filename.")
    parser.add_option("--replace", action="store_true",
                      help="Run unity /!\ This is for compatibility with other desktop interfaces and acts the same as running unity without --replace")
    parser.add_option("--ignore-upstart", action="store_true",
                      help="Run unity without upstart support")
    parser.add_option("--reset", action="store_true",
                      help="(deprecated: provided for backwards compatibility)")
    parser.add_option("--reset-icons", action="store_true",
                      help="Reset the default launcher icon.")
    parser.add_option("-v", "--verbose", action="store_true",
                      help="Get additional debug output from unity.")
    (options, args) = parser.parse_args()

    set_unity_env()

    if options.distro:
        sys.exit(reset_to_distro())

    if options.reset:
        print ("The --reset option is deprecated, You should run with no options instead.")

    if options.reset_icons:
        reset_launcher_icons ()

    if options.replace:
        print ("WARNING: This is for compatibility with other desktop interfaces please use unity without --replace")

    run_unity(args)
