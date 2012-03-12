#!/usr/bin/env python

#
# Script to generate a nice PNG file of the currently running unity introspection tree.
from argparse import ArgumentParser
from os import remove
from os.path import splitext
import dbus

try:
    from autopilot.emulators.unity import get_state_by_path
except ImportError, e:
    print "Error: could not import the autopilot python module."
    print "Make sure the autopilot module is in your $PYTHONPATH."
    exit(1)

try:
    import pydot
except ImportError:
    print "Error: the 'pydot' module is required to run this script."
    print "Try installing the 'python-pydot' package."
    exit(1)

NEXT_NODE_ID=1
NODE_BLACKLIST=["Result"]

def string_rep(dbus_type):
    """Get a string representation of various dbus types."""
    if type(dbus_type) == dbus.Boolean:
        return repr(bool(dbus_type))
    if type(dbus_type) == dbus.String:
        return dbus_type.encode('ascii', errors='ignore')
    if type(dbus_type) in (dbus.Int16, dbus.UInt16, dbus.Int32, dbus.UInt32, dbus.Int64, dbus.UInt64):
        return repr(int(dbus_type))
    if type(dbus_type) == dbus.Double:
        return repr(float(dbus_type))
    if type(dbus_type) == dbus.Array:
        return ', '.join([string_rep(i) for i in dbus_type])
    else:
        return repr(dbus_type)


def escape(s):
    """Escape a string so it can be use in a dot label."""
    return pydot.quote_if_necessary(s).replace('<','\\<').replace('>', '\\>').replace("'", "\\'")


def traverse_tree(state, parent, graph):
    """Recursively traverse state tree, building dot graph as we go."""
    global NEXT_NODE_ID
    lbl = parent.get_comment() + "|"
    # first, set labels for this node:
    bits = ["%s=%s" % (k, string_rep(state[k])) for k in sorted(state.keys()) if k != 'Children']
    lbl += "\l".join(bits)
    parent.set_label(escape('"{' + lbl + '}"'))
    if state.has_key('Children'):
        # Add all array nodes as children of this node.
        for child_name, child_state in state['Children']:
            if child_name in NODE_BLACKLIST:
                continue
            child = pydot.Node(str(NEXT_NODE_ID))
            NEXT_NODE_ID+=1
            child.set_comment(child_name)
            graph.add_node(child)
            graph.add_edge(pydot.Edge(parent, child))

            traverse_tree(child_state, child, graph)


if __name__ == '__main__':
    parser = ArgumentParser()
    mg = parser.add_mutually_exclusive_group(required=True)
    mg.add_argument('-o', '--output', nargs=1,
        help='Store output in specified file on disk.')
    mg.add_argument('-d','--display', action='store_true',
        help='Display output in image viewer.')

    args = parser.parse_args()

    introspection_tree = get_state_by_path('/')
    graph = pydot.Dot()
    graph.set_simplify(False)
    graph.set_node_defaults(shape='Mrecord')
    graph.set_fontname('Ubuntu')
    graph.set_fontsize('10')

    gnode_unity = pydot.Node("Unity")
    gnode_unity.set_comment("Unity")
    traverse_tree(introspection_tree[0], gnode_unity, graph)

    if args.output:
        base, extension = splitext(args.output[0])
        write_method_name = 'write_' + extension[1:]
        if hasattr(graph, write_method_name):
            getattr(graph, write_method_name)(args.output[0])
        else:
            print "Error: unsupported format: '%s'" % (extension)
    elif args.display:
        from tempfile import NamedTemporaryFile
        from subprocess import call
        tf = NamedTemporaryFile(suffix='.png', delete=False)
        tf.write(graph.create_png())
        tf.close()
        call(["eog", tf.name])
        remove(tf.name)
    else:
        print 'unknown output mode!'


