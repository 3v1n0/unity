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
#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bind.hpp>
#include <NuxCore/Logger.h>
#include <NuxCore/LoggingWriter.h>

#include "DebugDBusInterface.h"
#include "Introspectable.h"
#include "XPathQueryPart.h"

namespace unity
{
const std::string DBUS_BUS_NAME = "com.canonical.Unity";

namespace debug
{
namespace
{
nux::logging::Logger logger("unity.debug.DebugDBusInterface");

namespace local
{
  std::ofstream output_file;
}
}

GVariant* GetState(std::string const& query);
void StartLogToFile(std::string const& file_path);
void ResetLogging();
void SetLogSeverity(std::string const& log_component,
  std::string const& severity);
void LogMessage(std::string const& severity,
  std::string const& message);

const char* DebugDBusInterface::DBUS_DEBUG_OBJECT_PATH = "/com/canonical/Unity/Debug";

const gchar DebugDBusInterface::introspection_xml[] =
  " <node>"
  "   <interface name='com.canonical.Autopilot.Introspection'>"
  ""
  "     <method name='GetState'>"
  "       <arg type='s' name='piece' direction='in' />"
  "       <arg type='a(sv)' name='state' direction='out' />"
  "     </method>"
  ""
  "   </interface>"
  ""
  "   <interface name='com.canonical.Unity.Debug.Logging'>"
  ""
  "     <method name='StartLogToFile'>"
  "       <arg type='s' name='file_path' direction='in' />"
  "     </method>"
  ""
  "     <method name='ResetLogging'>"
  "     </method>"
  ""
  "     <method name='SetLogSeverity'>"
  "       <arg type='s' name='log_component' direction='in' />"
  "       <arg type='s' name='severity' direction='in' />"
  "     </method>"
  ""
  "     <method name='LogMessage'>"
  "       <arg type='s' name='severity' direction='in' />"
  "       <arg type='s' name='message' direction='in' />"
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

static Introspectable* _parent_introspectable;

DebugDBusInterface::DebugDBusInterface(Introspectable* parent)
{
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
  g_dbus_node_info_unref(introspection_data);
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
    // GetState returns a floating variant and
    // g_dbus_method_invocation_return_value ref sinks it
    g_dbus_method_invocation_return_value(invocation, ret);
  }
  else if (g_strcmp0(method_name, "StartLogToFile") == 0)
  {
    const gchar* log_path;
    g_variant_get(parameters, "(&s)", &log_path);

    StartLogToFile(log_path);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else if (g_strcmp0(method_name, "ResetLogging") == 0)
  {
    ResetLogging();
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else if (g_strcmp0(method_name, "SetLogSeverity") == 0)
  {
    const gchar* component;
    const gchar* severity;
    g_variant_get(parameters, "(&s&s)", &component, &severity);

    SetLogSeverity(component, severity);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else if (g_strcmp0(method_name, "LogMessage") == 0)
  {
    const gchar* severity;
    const gchar* message;
    g_variant_get(parameters, "(&s&s)", &severity, &message);

    LogMessage(severity, message);
    g_dbus_method_invocation_return_value(invocation, NULL);
  }
  else
  {
    g_dbus_method_invocation_return_dbus_error(invocation,
                                               unity::DBUS_BUS_NAME.c_str(),
                                               "Failed to find method");
  }
}


GVariant* GetState(std::string const& query)
{
  // process the XPath query:
  std::list<Introspectable*> parts = GetIntrospectableNodesFromQuery(query, _parent_introspectable);
  GVariantBuilder  builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sv)"));
  if (parts.empty())
  {
    LOG_WARNING(logger) << "Query '" << query << "' Did not match anything.";
  }
  for (Introspectable *node : parts)
  {
    g_variant_builder_add(&builder, "(sv)", node->GetName().c_str(), node->Introspect());
  }

  return g_variant_new("(a(sv))", &builder);
}

void StartLogToFile(std::string const& file_path)
{
  if (local::output_file.is_open())
    local::output_file.close();
  local::output_file.open(file_path);
  nux::logging::Writer::Instance().SetOutputStream(local::output_file);
}

void ResetLogging()
{
  if (local::output_file.is_open())
    local::output_file.close();
  nux::logging::Writer::Instance().SetOutputStream(std::cout);
  nux::logging::reset_logging();
}

void SetLogSeverity(std::string const& log_component,
                    std::string const& severity)
{
  nux::logging::Logger(log_component).SetLogLevel(nux::logging::get_logging_level(severity));
}

void LogMessage(std::string const& severity,
                std::string const& message)
{
  nux::logging::Level level = nux::logging::get_logging_level(severity);
  if (logger.GetEffectiveLogLevel() <= level)
  {
   nux::logging::LogStream(level, logger.module(), __FILE__, __LINE__).stream()
      << message;
  }
}

/*
 * Do a breadth-first search of the introspection tree and find all nodes that match the
 * query.
 */
std::list<Introspectable*> GetIntrospectableNodesFromQuery(std::string const& query, Introspectable* tree_root)
{
  std::list<Introspectable*> start_points;
  std::string sanitised_query;
  // Allow user to be lazy when specifying root node.
  if (query == "" || query == "/")
  {
    sanitised_query = "/" + tree_root->GetName();
  }
  else
  {
    sanitised_query = query;
  }
  // split query into parts
  std::list<XPathQueryPart> query_parts;

  {
    std::list<std::string> query_strings;
    boost::algorithm::split(query_strings, sanitised_query, boost::algorithm::is_any_of("/"));
    // Boost's split() implementation does not match it's documentation! According to the
    // docs, it's not supposed to add empty strings, but it does, which is a PITA. This
    // next line removes them:
    query_strings.erase( std::remove_if( query_strings.begin(),
                                        query_strings.end(),
                                        boost::bind( &std::string::empty, _1 ) ),
                      query_strings.end());
    for (auto part : query_strings)
    {
      query_parts.push_back(XPathQueryPart(part));
    }
  }

  // absolute or relative query string?
  if (sanitised_query.at(0) == '/' && sanitised_query.at(1) != '/')
  {
    // absolute query - start point is tree root node.
    if (query_parts.front().Matches(tree_root))
    {
      start_points.push_back(tree_root);
    }
  }
  else
  {
    // relative - need to do a depth first tree search for all nodes that match the
    // first node in the query.

    // warn about malformed queries (all queries must start with '/')
    if (sanitised_query.at(0) != '/')
    {
      LOG_WARNING(logger) << "Malformed relative introspection query: '" << query << "'.";
    }

    // non-recursive BFS traversal to find starting points:
    std::queue<Introspectable*> queue;
    queue.push(tree_root);
    while (!queue.empty())
    {
      Introspectable *node = queue.front();
      queue.pop();
      if (query_parts.front().Matches(node))
      {
        // found one. We keep going deeper, as there may be another node beneath this one
        // with the same node name.
        start_points.push_back(node);
      }
      // Add all children of current node to queue.
      for (Introspectable* child : node->GetIntrospectableChildren())
      {
        queue.push(child);
      }
    }
  }

  // now we have the tree start points, process them:
  query_parts.pop_front();
  typedef std::pair<Introspectable*, std::list<XPathQueryPart>::iterator> node_match_pair;

  std::queue<node_match_pair> traverse_queue;
  for (Introspectable *node : start_points)
  {
    traverse_queue.push(node_match_pair(node, query_parts.begin()));
  }
  start_points.clear();

  while (!traverse_queue.empty())
  {
    node_match_pair p = traverse_queue.front();
    traverse_queue.pop();

    Introspectable *node = p.first;
    auto query_it = p.second;

    if (query_it == query_parts.end())
    {
      // found a match:
      start_points.push_back(node);
    }
    else
    {
      // push all children of current node to start of queue, advance search iterator, and loop again.
      for (Introspectable* child : node->GetIntrospectableChildren())
      {
        if (query_it->Matches(child))
        {
          auto it_copy(query_it);
          ++it_copy;
          traverse_queue.push(node_match_pair(child, it_copy));
        }
      }
    }
  }
  return start_points;
}
}
}
