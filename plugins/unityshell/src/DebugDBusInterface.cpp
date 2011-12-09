// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alex Launi <alex.launi@canonical.com>
 */

#include <queue>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <core/core.h>
#include <NuxCore/Logger.h>

#include "DebugDBusInterface.h"
#include "Introspectable.h"

namespace unity
{
const std::string DBUS_BUS_NAME = "com.canonical.Unity";

namespace debug
{
namespace
{
  nux::logging::Logger logger("unity.debug.DebugDBusInterface");
}

GVariant* GetState(std::string const& query);
Introspectable* FindPieceToIntrospect(std::queue<Introspectable*> queue, 
                                      std::string const& pieceName);


const char* DebugDBusInterface::DBUS_DEBUG_OBJECT_PATH = "/com/canonical/Unity/Debug";

const gchar DebugDBusInterface::introspection_xml[] =
  " <node>"
  "   <interface name='com.canonical.Unity.Debug.Introspection'>"
  ""
  "     <method name='GetState'>"
  "       <arg type='s' name='piece' direction='in' />"
  "       <arg type='a{sv}' name='state' direction='out' />"
  "     </method>"
  ""
  "   </interface>"
  " </node>";

GDBusInterfaceVTable DebugDBusInterface::interface_vtable =
{
  DebugDBusInterface::HandleDBusMethodCall,
  NULL,
  NULL
};

static CompScreen* _screen;
static Introspectable* _parent_introspectable;

DebugDBusInterface::DebugDBusInterface(Introspectable* parent, 
                                       CompScreen* screen)
{
  _screen = screen;
  _parent_introspectable = parent;
  _owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
                             unity::DBUS_BUS_NAME.c_str(),
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             &DebugDBusInterface::OnBusAcquired,
                             &DebugDBusInterface::OnNameAcquired,
                             &DebugDBusInterface::OnNameLost,
                             this,
                             NULL);
}

DebugDBusInterface::~DebugDBusInterface()
{
  g_bus_unown_name(_owner_id);
}

void
DebugDBusInterface::OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer data)
{
  int i = 0;
  GError* error;

  GDBusNodeInfo* introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  if (!introspection_data)
  {
    LOG_WARNING(logger) << "No dbus introspection data could be loaded. State introspection will not work";
    return;
  }

  while (introspection_data->interfaces[i] != NULL) 
  {
    error = NULL;
    g_dbus_connection_register_object(connection,
                                      DebugDBusInterface::DBUS_DEBUG_OBJECT_PATH,
                                      introspection_data->interfaces[i],
                                      &interface_vtable,
                                      data,
                                      NULL,
                                      &error);
    if (error != NULL)
    {
      g_warning("Could not register debug interface onto d-bus");
      g_error_free(error);
    }
    i++;
  }
}

void
DebugDBusInterface::OnNameAcquired(GDBusConnection* connection, const gchar* name, gpointer data)
{
}

void
DebugDBusInterface::OnNameLost(GDBusConnection* connection, const gchar* name, gpointer data)
{
}

void
DebugDBusInterface::HandleDBusMethodCall(GDBusConnection* connection,
                                         const gchar* sender,
                                         const gchar* object_path,
                                         const gchar* interface_name,
                                         const gchar* method_name,
                                         GVariant* parameters,
                                         GDBusMethodInvocation* invocation,
                                         gpointer user_data)
{
  if (g_strcmp0(method_name, "GetState") == 0)
  {
    GVariant* ret;
    const gchar* input;
    g_variant_get(parameters, "(&s)", &input);

    ret = GetState(input);
    g_dbus_method_invocation_return_value(invocation, ret);
    g_variant_unref(ret);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error(invocation, 
                                               unity::DBUS_BUS_NAME.c_str(),
                                               "Failed to find method");
  }
}

GVariant*
GetState(std::string const& query)
{
  std::queue<Introspectable*> queue;
  queue.push(_parent_introspectable);

  // Since the empty string won't really match the name of the parent (Unity),
  // we make sure that we're able to accept a blank string and just define it to
  // mean the top level.
  Introspectable* piece = (query == "")
    ? _parent_introspectable
    : FindPieceToIntrospect(queue, query);

  // FIXME this might not work, make sure it does.
  if (piece == NULL)
    return NULL;

  return piece->Introspect(true);
}

/*
 * Do a breadth-first search of the introspection tree and find all nodes that match the 
 * query. Also modify the query string such that the start points are removed from it.
 */
std::list<Introspectable*> FindQueryStartPoints(std::string& query, Introspectable* tree_root)
{
  std::deque<Introspectable*> start_points;
  
  // Allow user to be lazy when specifying root node.
  if (query == "" || query == "/")
  {
    query = "/" + tree_root->GetName();
  }
  // split query into parts
  std::list<std::string> query_parts;
  boost::algorithm::split(query_parts, query, boost::algorithm::is_any_of("/"));
  // Boost's split() implementation does not match it's documentation! According to the 
  // docs, it's not supposed to add empty string, but it does, which is a PITA. This 
  // next line removes them:
  query_parts.erase( std::remove_if( query_parts.begin(), 
                                      query_parts.end(), 
                                      boost::bind( &std::string::empty, _1 ) ), 
                    query_parts.end());

  // absolute or relative query string?
  if (query.at(0) == '/' && query.at(1) != '/')
  {
    LOG_DEBUG(logger) << "Query '" << query << "' is absolute.";
    // absolute query - start point is tree root node.
    start_points.push_back(tree_root);
    query.erase(0,1);
  }
  else
  {
    LOG_DEBUG(logger) << "Query '" << query << "' is relative.";
    // relative - need to do a depth first tree search for all nodes that match the
    // first node in the query.

    // warn about malformed queries (all queries must start with '/')
    if (query.at(0) != '/')
    {
      LOG_WARNING(logger) << "Malformed introspection query: '" << query << "'.";
    }
    
    // non-recursive BFS traversal to find starting points:
    std::queue<Introspectable*> queue;
    queue.push(tree_root);
    while (!queue.empty())
    {
      Introspectable *node = queue.front();
      queue.pop();
      if (node->GetName() == query_parts.front())
      {
        // found one.
        LOG_DEBUG(logger) << "Node '" << node->GetName() << "' Matches query start point.";
        start_points.push_back(node);
      }
      // Add all children of current node to queue.
      foreach(Introspectable* child, node->GetIntrospectableChildren())
      {
        queue.push(child);
      }
    }
  }

  // now we have the tree start points, process them:
  std::list<Introspectable*> results;
  
  LOG_DEBUG(logger) << "Query parts is:" << query_parts.size();
  std::string dbg;
  foreach (std::string p, query_parts)
  {
    dbg += p + ":";
  }
  LOG_DEBUG(logger) << dbg;

  query_parts.pop_front();

  auto query_it = query_parts.begin();
  while (!start_points.empty())
  {
    Introspectable *node = start_points.front();
    start_points.pop_front();

    if (query_it == query_parts.end())
    {
      LOG_DEBUG(logger) << "Reached end of query string - we must have found a match!";
      // found a match:
      results.push_back(node);
      query_it = query_parts.begin();
    }
    else
    {
      // push all children of current node to start of queue, advance search iterator, and loop again.
      foreach (Introspectable *child, node->GetIntrospectableChildren())
      {
        if (child->GetName() == *query_it)
        {
          start_points.push_front(child);
        }
      }
      ++query_it;
    }
  }
  return results;
}
/*
 * Do a breadth-first search of the introspectable tree.
 */
Introspectable*
FindPieceToIntrospect(std::queue<Introspectable*> queue, std::string const& piece_name)
{
  Introspectable* piece;

  while (!queue.empty())
  {
    piece = queue.front();
    queue.pop();

    if (piece->GetName() == piece_name)
    {
      return piece;
    }

    for (auto it = piece->GetIntrospectableChildren().begin(), last = piece->GetIntrospectableChildren().end(); it != last; it++)
    {
      queue.push(*it);
    }

    FindPieceToIntrospect(queue, piece_name);
  }

  return NULL;
}
}
}
