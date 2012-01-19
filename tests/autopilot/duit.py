#!/usr/bin/env python

#
# Script to generate a nice PNG file of the currently running unity introspection tree.
import dbus
from autopilot.emulators.unity import Unity
from StringIO import StringIO

try:
    import gvgen
except ImportError:
    print "Error: the 'gvgen' module is required to run this script."
    print "Try installing the 'python-gvgen' package."
    exit(1)

try:
    import pydot
except ImportError:
    print "Error: the 'pydot' module is required to run this script."
    print "Try installing the 'python-pydot' package."

def string_rep(dbus_type):
    if type(dbus_type) == dbus.Boolean:
        return repr(bool(dbus_type))
    if type(dbus_type) == dbus.String:
        return str(dbus_type)
    if type(dbus_type) in (dbus.Int16, dbus.Int32, dbus.Int64):
        return repr(int(dbus_type))
    else:
        return repr(dbus_type)

def traverse_graph(state, parent, graph):
    for key in state.keys():
        #if type(state[key]) == dbus.Array:
        if key == 'Children':
            # Add all array nodes as children of this node.
            for child_name, child_state in state[key]:
                traverse_graph(child_state, graph.newItem(child_name, parent), graph)
        else:
            graph.propertyAppend(parent, key, string_rep(state[key]))

if __name__ == '__main__':
    u = Unity()
    introspection_tree = u.get_state()
    graph = gvgen.GvGen()
    gnode_unity = graph.newItem("Unity")
    traverse_graph(introspection_tree[0], gnode_unity, graph)

    buff = StringIO()
    graph.dot(buff)
    g = pydot.graph_from_dot_data(buff.getvalue())
    import pdb; pdb.set_trace()