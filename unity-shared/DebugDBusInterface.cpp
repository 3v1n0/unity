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

#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <NuxCore/Logger.h>
#include <NuxCore/LoggingWriter.h>
#include <xpathselect/xpathselect.h>
#include <dlfcn.h>

#include "DebugDBusInterface.h"
#include "Introspectable.h"

namespace unity
{

namespace debug
{
DECLARE_LOGGER(logger, "unity.debug.interface");
namespace
{
namespace local
{
  std::ofstream output_file;
  void* xpathselect_driver_ = nullptr;

  class IntrospectableAdapter : public std::enable_shared_from_this<IntrospectableAdapter>, public xpathselect::Node
  {
  public:
    typedef std::shared_ptr<IntrospectableAdapter> Ptr;
    IntrospectableAdapter(Introspectable* node, IntrospectableAdapter::Ptr const& parent = nullptr)
      : node_(node)
      , parent_(parent)
      , full_path_((parent_ ? parent_->GetPath() : "") + "/" + GetName())
    {}

    int32_t GetId() const
    {
      return node_->GetIntrospectionId();
    }

    std::string GetName() const
    {
      return node_->GetName();
    }

    std::string GetPath() const
    {
      return full_path_;
    }

    Node::Ptr GetParent() const
    {
      return parent_;
    }

    bool MatchBooleanProperty(std::string const& name, bool value) const
    {
      return MatchProperty(name, value ? "TRUE" : "FALSE");
    }

    bool MatchIntegerProperty(std::string const& name, int32_t value) const
    {
      return MatchProperty(name, std::to_string(value));
    }

    bool MatchStringProperty(std::string const& name, std::string const& value) const
    {
      return MatchStringProperty(name, value);
    }

    bool MatchProperty(std::string const& name, std::string const& value) const
    {
      bool matches = false;
      GVariantBuilder child_builder;
      g_variant_builder_init(&child_builder, G_VARIANT_TYPE("a{sv}"));
      g_variant_builder_add(&child_builder, "{sv}", "id", g_variant_new_uint64(node_->GetIntrospectionId()));
      node_->AddProperties(&child_builder);
      GVariant* prop_dict = g_variant_builder_end(&child_builder);
      GVariant* prop_value = g_variant_lookup_value(prop_dict, name.c_str(), nullptr);

      if (prop_value)
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
            bool p = value == "TRUE" || value == "ON" || value == "YES" || value == "1";
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
      auto const& this_ptr = std::const_pointer_cast<IntrospectableAdapter>(shared_from_this());

      for(auto const& child: node_->GetIntrospectableChildren())
        children.push_back(std::make_shared<IntrospectableAdapter>(child, this_ptr));

      return children;
    }

    Introspectable* Node() const
    {
      return node_;
    }

  private:
    Introspectable* node_;
    IntrospectableAdapter::Ptr parent_;
    std::string full_path_;
  };

  xpathselect::NodeVector select_nodes(IntrospectableAdapter::Ptr const& root, std::string const& query)
  {
    if (!xpathselect_driver_)
      xpathselect_driver_ = dlopen("libxpathselect.so.1.4", RTLD_LAZY);

    if (xpathselect_driver_)
    {
      typedef decltype(&xpathselect::SelectNodes) select_nodes_t;
      dlerror();
      auto SelectNodes = reinterpret_cast<select_nodes_t>(dlsym(xpathselect_driver_, "SelectNodes"));
      const char* err = dlerror();
      if (err)
      {
        LOG_ERROR(logger) << "Unable to load entry point in libxpathselect: " << err;
      }
      else
      {
        return SelectNodes(root, query);
      }
    }
    else
    {
      LOG_WARNING(logger) << "Cannot complete introspection request because libxpathselect is not installed.";
    }

    // Fallen through here as we've hit an error
    return xpathselect::NodeVector();
  }

  // This needs to be called at destruction to cleanup the dlopen
  void cleanup_xpathselect()
  {
    if (xpathselect_driver_)
      dlclose(xpathselect_driver_);
  }

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

namespace dbus
{
const std::string BUS_NAME = "com.canonical.Unity";
const std::string OBJECT_PATH = "/com/canonical/Unity/Debug";

const std::string INTROSPECTION_XML =
  " <node>"
  "   <interface name='com.canonical.Autopilot.Introspection'>"
  ""
  "     <method name='GetState'>"
  "       <arg type='s' name='piece' direction='in' />"
  "       <arg type='a(sv)' name='state' direction='out' />"
  "     </method>"
  ""
  "     <method name='GetVersion'>"
  "       <arg type='s' name='version' direction='out' />"
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
}

static Introspectable* _parent_introspectable;

DebugDBusInterface::DebugDBusInterface(Introspectable* parent)
  : server_(dbus::BUS_NAME)
{
  _parent_introspectable = parent;

  server_.AddObjects(dbus::INTROSPECTION_XML, dbus::OBJECT_PATH);

  for (auto const& obj : server_.GetObjects())
    obj->SetMethodsCallsHandler(&DebugDBusInterface::HandleDBusMethodCall);
}

DebugDBusInterface::~DebugDBusInterface()
{
  local::cleanup_xpathselect();
}

GVariant* DebugDBusInterface::HandleDBusMethodCall(std::string const& method, GVariant* parameters)
{
  if (method == "GetState")
  {
    const gchar* input;
    g_variant_get(parameters, "(&s)", &input);

    return GetState(input);
  }
  else if (method == "GetVersion")
  {
    return g_variant_new("(s)", "1.3");
  }
  else if (method == "StartLogToFile")
  {
    const gchar* log_path;
    g_variant_get(parameters, "(&s)", &log_path);

    StartLogToFile(log_path);
  }
  else if (method == "ResetLogging")
  {
    ResetLogging();
  }
  else if (method == "SetLogSeverity")
  {
    const gchar* component;
    const gchar* severity;
    g_variant_get(parameters, "(&s&s)", &component, &severity);

    SetLogSeverity(component, severity);
  }
  else if (method == "LogMessage")
  {
    const gchar* severity;
    const gchar* message;
    g_variant_get(parameters, "(&s&s)", &severity, &message);

    LogMessage(severity, message);
  }

  return nullptr;
}

GVariant* GetState(std::string const& query)
{
  GVariantBuilder  builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sv)"));

  auto root_node = std::make_shared<local::IntrospectableAdapter>(_parent_introspectable);
  for (auto const& n : local::select_nodes(root_node, query))
  {
    auto p = std::static_pointer_cast<local::IntrospectableAdapter const>(n);
    if (p)
      g_variant_builder_add(&builder, "(sv)", p->GetPath().c_str(), p->Node()->Introspect());
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
