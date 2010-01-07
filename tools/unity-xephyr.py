#!/usr/bin/python

import os
import sys
import subprocess
import time
import signal

start_display = os.environ["DISPLAY"]

DISPLAY=":12"

# Start Xephyr
try:
  xephyr = subprocess.Popen (["Xephyr", DISPLAY, "-screen", "1024x600", "-host-cursor"])
except OSError, e:
  if e.errno == errno.ENOENT:
    print "Could not find Xephyr"
    sys.exit (1)
  else:
    raise

time.sleep (3)

f=os.popen("dbus-launch")
f=os.popen("export %s" % f.read())

os.environ["DISPLAY"] = DISPLAY

time.sleep (2)
subprocess.Popen(["gcalctool"])
subprocess.Popen(["gnome-panel"])
subprocess.Popen(["nautilus"])

time.sleep (1)
# Start Mutter
if os.path.exists(os.path.join(os.getcwd(), "mutter-plugin")):
  print ""
  print "Using locally built libunity-mutter.la"
  print ""
  plugin = os.path.join(os.getcwd(), "mutter-plugin", "libunity-mutter.la")
else:
  print ""
  print "Using system libunity-mutter.so"
  print ""
  plugin = "libunity-mutter"

args = []
args.extend(["mutter", "--mutter-plugins="+plugin])

mutter = subprocess.Popen(args)

try:
  mutter.wait ()
except KeyboardInterrupt, e:
  try:
    os.kill(mutter.pid, signal.SIGKILL)
  except:
    pass
  mutter.wait ()
finally:
  try:
    os.kill(xephyr.pid, signal.SIGKILL)
  except OSError:
    pass
  os.environ['DISPLAY'] = start_display
