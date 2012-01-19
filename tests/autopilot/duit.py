#!/usr/bin/env python

#
# Script to generate a nice PNG file of the currently running unity introspection tree.
import dbus
from autopilot.emulators.unity import Unity

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
    if type(dbus_type) in (dbus.Int16, dbus.UInt16, dbus.Int32, dbus.UInt32, dbus.Int64, dbus.UInt64):
        return repr(int(dbus_type))
    if type(dbus_type) == dbus.Double:
        return repr(float(dbus_type))
    if type(dbus_type) == dbus.Array:
        return ', '.join([string_rep(i) for i in dbus_type])
    else:
        return repr(dbus_type)
def escape(s):
    return pydot.quote_if_necessary(s)

def traverse_graph(state, parent, graph):
    for key in state.keys():
        #if type(state[key]) == dbus.Array:
        if key == 'Children':
            # Add all array nodes as children of this node.
            for child_name, child_state in state[key]:
                child = pydot.Node(child_name)
                graph.add_node(child)
                graph.add_edge(pydot.Edge(parent, child))
                traverse_graph(child_state, child, graph)
        else:
            pass
            #graph.propertyAppend(parent, escape(key), string_rep(state[key]))

if __name__ == '__main__':
    u = Unity()
    introspection_tree = u.get_state()
    graph = pydot.Graph()
    graph.set_simplify(False)
    graph.set_node_defaults(shape='record')
    gnode_unity = pydot.Node("Unity")
    traverse_graph(introspection_tree[0], gnode_unity, graph)

    #buff = StringIO()
    #graph.dot(buff)
    open('out.dot', 'w').write(graph.to_string())