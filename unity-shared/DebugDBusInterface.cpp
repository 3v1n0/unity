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
#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/bind.hpp>
#include <NuxCore/Logger.h>
#include <NuxCore/LoggingWriter.h>
#include <xpathselect/node.h>
#include <xpathselect/xpathselect.h>
#include <dlfcn.h>

#include "DebugDBusInterface.h"
#include "Introspectable.h"

namespace unity
{
const std::string DBUS_BUS_NAME = "com.canonical.Unity";

namespace debug
{
DECLARE_LOGGER(logger, "unity.debug.interface");
namespace
{
namespace local
{
  std::ofstream output_file;

  class IntrospectableAdapter: public xpathselect::Node
  {
  public:
    typedef std::shared_ptr<IntrospectableAdapter> Ptr;
    IntrospectableAdapter(Introspectable* node)
    : node_(node)
    {}

    std::string GetName() const
    {
      return node_->GetName();
    }

    bool MatchProperty(const std::string& name, const std::string& value) const
    {
      bool matches = false;

      GVariantBuilder  child_builder;
      g_variant_builder_init(&child_builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&child_builder, "{sv}", "id", g_variant_new_uint64(node_->GetIntrospectionId()));
      node_->AddProperties(&child_builder);
      GVariant* prop_dict = g_variant_builder_end(&child_builder);
      GVariant *prop_value = g_variant_lookup_value(prop_dict, name.c_str(), NULL);

      if (prop_value != NULL)
      {
        GVariantClass prop_val_type = g_variant_classify(prop_value);
        // it'd be nice to be able to do all this with one method. However, the booleans need
        // special treatment, and I can't figure out how to group all the integer types together
        // without resorting to template functions.... and we all know what happens when you
        // start doing that...
        switch (prop_val_type)
        {
          case G_VARIANT_CLASS_STRING:
          {
            const gchar* prop_val = g_variant_get_string(prop_value, NULL);
            if (g_strcmp0(prop_val, value.c_str()) == 0)
            {
              matches = true;
            }
          }
          break;
          case G_VARIANT_CLASS_BOOLEAN:
          {
            std::string value = boost::to_upper_copy(value);
            bool p = value == "TRUE" ||
                      value == "ON" ||
                      value == "YES" ||
                      value == "1";
            matches = (g_variant_get_boolean(prop_value) == p);
          }
          break;
          case G_VARIANT_CLASS_BYTE:
          {
            // It would be nice if I could do all the integer types together, but I couldn't see how...
            std::stringstream stream(value);
            int val; // changing this to guchar causes problems.
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_byte(prop_value);
          }
          break;
          case G_VARIANT_CLASS_INT16:
          {
            std::stringstream stream(value);
            gint16 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_int16(prop_value);
          }
          break;
          case G_VARIANT_CLASS_UINT16:
          {
            std::stringstream stream(value);
            guint16 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_uint16(prop_value);
          }
          break;
          case G_VARIANT_CLASS_INT32:
          {
            std::stringstream stream(value);
            gint32 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_int32(prop_value);
          }
          break;
          case G_VARIANT_CLASS_UINT32:
          {
            std::stringstream stream(value);
            guint32 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_uint32(prop_value);
          }
          break;
          case G_VARIANT_CLASS_INT64:
          {
            std::stringstream stream(value);
            gint64 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_int64(prop_value);
          }
          break;
          case G_VARIANT_CLASS_UINT64:
          {
            std::stringstream stream(value);
            guint64 val;
            stream >> val;
            matches = (stream.rdstate() & (stream.badbit|stream.failbit)) == 0 &&
                      val == g_variant_get_uint64(prop_value);
          }
          break;
        default:
          LOG_WARNING(logger) << "Unable to match against property of unknown type.";
        };
      }
      g_variant_unref(prop_value);
      g_variant_unref(prop_dict);
      return matches;
    }

    std::vector<xpathselect::Node::Ptr> Children() const
    {
      std::vector<xpathselect::Node::Ptr> children;
      for(auto child: node_->GetIntrospectableChildren())
      {
        children.push_back(std::make_shared<IntrospectableAdapter>(child));
      }
      return children;

    }

    Introspectable* node_;
  };
}
}

bool TryLoadXPathImplementation();
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
  GVariantBuilder  builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sv)"));

  // try load the xpathselect library:
  void* driver = dlopen("libxpathselect.so", RTLD_LAZY);
  if (driver)
  {
    typedef decltype(&xpathselect::SelectNodes) entry_t;
    // clear errors:
    dlerror();
    entry_t entry_point = (entry_t) dlsym(driver, "SelectNodes");
    const char* err = dlerror();
    if (err)
    {
      LOG_ERROR(logger) << "Unable to load entry point in libxpathselect: " << err;
    }
    else
    {
      // process the XPath query:
      local::IntrospectableAdapter::Ptr root_node = std::make_shared<local::IntrospectableAdapter>(_parent_introspectable);
      auto nodes = entry_point(root_node, query);
      for (auto n : nodes)
      {
        auto p = std::static_pointer_cast<local::IntrospectableAdapter>(n);
        if (p)
          g_variant_builder_add(&builder, "(sv)", p->node_->GetName().c_str(), p->node_->Introspect());
      }
    }
  }
  else
  {
    LOG_WARNING(logger) << "Cannot complete introspection request because libxpathselect is not installed.";
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
  nux::logging::Logger const& log_ref = Unwrap(logger);
  if (log_ref.GetEffectiveLogLevel() <= level)
  {
    nux::logging::LogStream(level, log_ref.module(), __FILE__, __LINE__).stream()
      << message;
  }
}

}
}
