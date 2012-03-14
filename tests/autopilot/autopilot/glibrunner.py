import sys
from dbus.mainloop.glib import DBusGMainLoop
import glib
import testtools

DBusGMainLoop(set_as_default=True)


# Turning run_in_glib_loop into a decorator is left as an exercise for the
# reader.
def run_in_glib_loop(function, *args, **kwargs):
    loop = glib.MainLoop()
    # XXX: I think this has re-entrancy problems.  There is parallel code in
    # testtools somewhere (spinner.py or deferredruntest.py)
    result = []
    errors = []

    def callback():
        try:
            result.append(function(*args, **kwargs))
        except:
            errors.append(sys.exc_info())
            # XXX: Not sure if this is needed / desired
            raise
        finally:
            loop.quit()
    glib.idle_add(callback)
    loop.run()
    if errors:
        raise errors[0]
    return result[0]


class GlibRunner(testtools.RunTest):

    # This implementation runs setUp, the test and tearDown in one event
    # loop. Maybe not what's needed.

    def _run_core(self):
        run_in_glib_loop(super(GlibRunner, self)._run_core)
