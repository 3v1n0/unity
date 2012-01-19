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
    return pydot.quote_if_necessary(s)

def traverse_graph(state, parent, graph):
    global NEXT_NODE_ID
    lbl = parent.get_comment() + "|"
    # first, set labels for this node:
    for key in state.keys():
        #if type(state[key]) == dbus.Array:
        if key == 'Children':
            continue
        lbl += "\l " + key + "=" + string_rep(state[key])
    parent.set_label('"{' + lbl + '}"')
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
    graph = pydot.Graph()
    graph.set_simplify(False)
    graph.set_node_defaults(shape='record')
    gnode_unity = pydot.Node("Unity")
    gnode_unity.set_comment("Unity")
    traverse_graph(introspection_tree[0], gnode_unity, graph)

    #buff = StringIO()
    #graph.dot(buff)
    open('out.dot', 'w').write(graph.to_string())