#!/usr/bin/env python
#
# Copyright (C) 2009 Canonical Ltd
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
# Authored by Gordon Allott <gord.allott@canonical.com>
#
#
import os, sys
import getopt
import cairo
import csv
import math
import random
from string import Template
from socket import gethostname
from datetime import datetime
import re
import subprocess

header = Template("""Unity bootchart for $hostname ($date)
uname: $uname
CPU: $cpu
GPU: $gpu
time: $total_time""")

def sort_by_domain (x, y):
  if x["start"] - y["start"] < 0:
    return -1
  else:
    return +1

def gatherinfo (filename):
  date =  datetime.fromtimestamp(os.path.getmtime(filename))

  cpufile = open ("/proc/cpuinfo")
  cpuinfo = cpufile.read (10024)
  cpure = re.search (r"^model name\s*: (.*$)", cpuinfo, re.MULTILINE)
  cpu = cpure.group(1)

  gpu_prog = subprocess.Popen("glxinfo", stdout=subprocess.PIPE)
  gpu_prog.wait ()
  gpuinfo = gpu_prog.stdout.read (10024)
  gpure = re.search (r"^OpenGL renderer string: (.*$)", gpuinfo, re.MULTILINE)
  gpu = gpure.group (1)

  return {"hostname":gethostname(),
          "date": date.strftime("%A, %d. %B %Y %I:%M%p"),
          "uname": " ".join (os.uname ()),
          "cpu": cpu,
          "gpu": gpu,
          "total_time": "undefined"
          }

width_multiplier = 1000
bar_height = 16

def draw_bg_graph (ctx, seconds, height):

  total_width = seconds * width_multiplier
  ctx.set_source_rgba (0.0, 0.0, 0.0, 0.25)

  ctx.move_to (0, 0)
  ctx.line_to (total_width, 0)
  ctx.stroke ()

  per_ten = 0
  for pos in xrange (0, int(total_width), int (0.01 * width_multiplier)):
    ctx.set_line_width (1)
    ctx.set_source_rgba (0.0, 0.0, 0.0, 0.10)

    if (not per_ten):
      ctx.set_line_width (2)
      ctx.set_source_rgba (0.0, 0.0, 0.0, 0.25)
      ctx.move_to (pos-6, -2)
      ctx.show_text (str (pos / float(width_multiplier)))
      ctx.stroke ()

    ctx.move_to (pos, 0)
    ctx.line_to (pos, height)
    ctx.stroke ()

    per_ten += 1
    per_ten %= 10

def build_graph (data, filename, info):

  padding_left = 6
  padding_right = 100
  padding_top = 6
  padding_bottom = 6

  total_size = 0.0
  for item in data:
    if item['end'] > total_size:
      total_size = item['end']

  width = total_size * width_multiplier + padding_left + padding_right
  height = (len(data) * (bar_height)) + 80 + padding_bottom + padding_top
  surface = cairo.SVGSurface(filename, max (width, 800), max (height, 600))

  ctx = cairo.Context (surface)

  #fill background
  ctx.set_source_rgb (1, 1, 1)
  ctx.rectangle (0, 0,  max (width, 800), max (height, 600))
  ctx.fill ()

  #print header
  info['total_time'] = "%s secs" % total_size
  sheader = header.substitute(info)

  ctx.translate (padding_left, padding_top)
  ctx.set_source_rgb (0, 0, 0)
  for line in sheader.split("\n"):
    ctx.translate (0, 12)
    ctx.show_text (line)
    ctx.fill ()

  ctx.translate (6, 12)

  draw_bg_graph (ctx, total_size + 0.5, max (len (data) * bar_height + 64, 500))

  ctx.set_line_width (1)
  for item in data:
    x = item['start'] * width_multiplier
    x1 = (item['end'] - item['start']) * width_multiplier
    ctx.translate (x, 0)

    ctx.set_source_rgba (0.35, 0.65, 0.8, 0.5)
    ctx.rectangle (0, 0, x1, 16)
    ctx.fill ()

    ctx.set_source_rgba (0.35, 0.65, 0.8, 1.0)
    ctx.rectangle (0, 0, x1, 16)
    ctx.stroke ()

    ctx.translate (8, 10)
    ctx.set_source_rgb (0.0, 0.0, 0.0)
    ctx.show_text ("%s %.4f seconds" % (item['name'], item["end"] - item["start"]))
    ctx.fill()

    ctx.translate (-x-8, 6)

def build_data_structure (input):
  reader = csv.reader(open(input))
  structure = []
  print "reading", input
  for row in reader:
    name = row[0]
    start = float(row[1])
    end = float(row[2])
    structure.append ({"name": name, "start": start, "end": end})

  structure.sort (sort_by_domain)
  return structure


def usage():
  print "use --input=filename.log and --output=filename.svg :)"

def main():

  try:
      opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "output=", "input="])
  except getopt.GetoptError, err:
    # print help information and exit:
    print str(err) # will print something like "option -a not recognized"
    usage()
    sys.exit(2)

  output = None
  input = None
  for o, a in opts:
    if o in ("-h", "--help"):
      usage()
      sys.exit()
    elif o in ("--output"):
      output = a
    elif o in ("--input"):
      input = a
    else:
      assert False, "unhandled option"

  if (not output or not input):
    usage()
    sys.exit()

  data = build_data_structure (input)
  info = gatherinfo (input)
  build_graph (data, output, info)


  return 0

if __name__ == '__main__': main()
