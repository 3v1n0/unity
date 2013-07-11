#!/bin/bash
# Copyright (C) 2013 Canonical Ltd
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Marco Trevisan <marco.trevisan@canonical.com>

function check_command()
{
  local binary=$(which $1)

  if [ -z "$binary" ]; then
    if [ -z "$1" ]; then
      echo "Invalid $2 submitted"
    elif [ ! -x "$1" ]; then
      echo "The provided $1 is not executable"
    fi

    exit 1
  fi
}

check_command "$1" "service"
service=$1
for p in $(pidof $(basename $service)); do kill $p; done

shift
check_command "$1" "test"
binary=$1

dbusfile=$(mktemp -t dummy.dbus.sh.XXXXXXXX)
dbus-launch --sh-syntax > $dbusfile
source $dbusfile

function do_cleanup()
{
  if [ -n "$service_pid" ] && (kill -0 $service_pid &> /dev/null); then kill $service_pid; fi
  kill $DBUS_SESSION_BUS_PID
  rm $dbusfile
}

trap "do_cleanup; exit 1" SIGHUP SIGINT SIGSEGV SIGTERM

$service &> /dev/null &
service_pid=$!

shift
$binary $@
ret_val=$?

do_cleanup
exit $ret_val
