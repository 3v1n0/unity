from gi.repository import GLib, Gio
from compizconfig import Context

# Importing gi.repository in the test was causing testtools to fail to run the tests.
# This is a work
gio = Gio
glib = GLib

global_context = Context()

# this can be set to True, in which case tests will be recorded.
video_recording_enabled = False

# this is where videos will be put after being encoded.
video_record_directory = "/tmp/autopilot"
