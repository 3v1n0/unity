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

MAX_WAIT=10

binary=$(which $1)

if [ -z "$binary" ]; then
  if [ -z "$1" ]; then
    echo "Empty command submitted"
  elif [ ! -x "$1" ]; then
    echo "The provided $1 is not executable"
  fi

  exit 1
fi

logfile=$(mktemp -t dummy.Xorg.log.XXXXXXXX)
conffile=$(mktemp -t dummy.Xorg.conf.XXXXXXXX)

cat << 'END_DUMMY_XORG_CONF' > $conffile
Section "Monitor"
  Identifier "Dummy Monitor"
EndSection

Section "Device"
  Identifier "Dummy Card"
  Driver "dummy"
EndSection

Section "Screen"
  DefaultDepth 24
  Identifier "Dummy Screen"
  Device "Dummy Card"
  Monitor "Dummy Monitor"
EndSection
END_DUMMY_XORG_CONF

function do_cleanup()
{
  if [ -n "$x_pid" ] && (kill -0 $x_pid &> /dev/null); then kill $x_pid; fi
  rm $conffile
  rm $logfile*
}

trap "do_cleanup; exit 1" SIGHUP SIGINT SIGSEGV SIGTERM

dpy=$((RANDOM+1))
export DISPLAY=:`for id in $(seq $dpy $((dpy+50))); do test -e /tmp/.X$id-lock || { echo $id; exit 0; }; done; exit 1`
Xorg $DISPLAY -config $conffile -logfile $logfile &> /dev/null &
x_pid=$!

start_time=$(date +%s)
while [ ! -e /tmp/.X${DISPLAY:1}-lock ] && [ $(($(date +%s) - start_time)) -le $MAX_WAIT ]; do
  sleep 0.1
done

if [ $(($(date +%s) - start_time)) -gt $MAX_WAIT ]; then
  echo "The X server was not able to run in time"
  if [ -s $logfile ]; then
    echo "Xorg Log:"
    cat $logfile
  fi
  do_cleanup
  exit 1
fi

shift
$binary $@
ret_val=$?

do_cleanup
exit $ret_val
