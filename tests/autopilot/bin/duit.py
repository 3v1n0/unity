#!/usr/bin/env python

#
# Script to generate a nice PNG file of the currently running unity introspection tree.
from sys import argv

if len(argv) != 2:
    print """Usage: %s output_file.png.

This script queries the currently running Unity process and dumps the entire
introspection tree into a graph, and renders this to a PNG file.
""" % (argv[0])
    exit(1)

import dbus
from autopilot.emulators.unity import Unity


try:
    import pydot
except ImportError:
    print "Error: the 'pydot' module is required to run this script."
    print "Try installing the 'python-pydot' package."

NEXT_NODE_ID=1

def string_rep(dbus_type):
    if type(dbus_type) == dbus.Boolean:
        return repr(bool(dbus_type))
    if type(dbus_type) == dbus.String:
        return str(dbus_type)
    if type(dbus_type) in (dbus.Int16, dbus.UInt16, dbus.Int32, dbus.UInt32, dbus.Int64, dbus.UInt64):
        return repr(int(dbus_type))
    if type(dbus_type) == dbus.Double:
        return repr(float(dbus_type))
    if type(dbus_type) == dbus.Array:
        return ', '.join([string_rep(i) for i in dbus_type])
    else:
        return repr(dbus_type)
def escape(s):
    return pydot.quote_if_necessary(s).replace('<','\\<').replace('>', '\\>')

def traverse_graph(state, parent, graph):
    global NEXT_NODE_ID
    lbl = parent.get_comment() + "|"
    # first, set labels for this node:
    bits = ["%s=%s" % (k, string_rep(state[k])) for k in state.keys() if k != 'Children']
    lbl += "\l".join(bits)
    parent.set_label(escape('"{' + lbl + '}"'))
    if state.has_key('Children'):
        # Add all array nodes as children of this node.
        for child_name, child_state in state['Children']:
            child = pydot.Node(str(NEXT_NODE_ID))
            NEXT_NODE_ID+=1
            child.set_comment(child_name)
            graph.add_node(child)
            graph.add_edge(pydot.Edge(parent, child))

            traverse_graph(child_state, child, graph)
        

if __name__ == '__main__':
    u = Unity()
    introspection_tree = u.get_state()
    graph = pydot.Dot()
    graph.set_simplify(False)
    graph.set_node_defaults(shape='Mrecord')
    graph.set_fontname('Ubuntu')
    graph.set_fontsize('10')
    
    gnode_unity = pydot.Node("Unity")
    gnode_unity.set_comment("Unity")
    traverse_graph(introspection_tree[0], gnode_unity, graph)

    graph.write(argv[1], format='png')