// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2013 Canonical Ltd
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
 *              Thomi Richards <thomi.richards@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <fstream>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <NuxCore/Logger.h>
#include <NuxCore/LoggingWriter.h>
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/Variant.h>
#include <xpathselect/xpathselect.h>
#include <dlfcn.h>

#include "DebugDBusInterface.h"
#include "Introspectable.h"

namespace unity
{
namespace debug
{
namespace
{

DECLARE_LOGGER(logger, "unity.debug.interface");

namespace local
{
  const std::string PROTOCOL_VERSION = "1.4";
  const std::string XPATH_SELECT_LIB = "libxpathselect.so.1.4";

  void* xpathselect_driver_ = nullptr;

  class IntrospectableAdapter : public std::enable_shared_from_this<IntrospectableAdapter>, public xpathselect::Node
  {
  public:
    typedef std::shared_ptr<IntrospectableAdapter const> Ptr;
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

    bool MatchStringProperty(std::string const& name, std::string const& value) const
    {
      auto const& prop_value = GetPropertyValue(name);

      if (prop_value)
      {
        if (!g_variant_is_of_type(prop_value, G_VARIANT_TYPE_STRING))
        {
          LOG_WARNING(logger) << "Unable to match '"<< name << "', it's not a string property.";
          return false;
        }

        return (prop_value.GetString() == value);
      }

      return false;
    }

    bool MatchBooleanProperty(std::string const& name, bool value) const
    {
      auto const& prop_value = GetPropertyValue(name);

      if (prop_value)
      {
        if (!g_variant_is_of_type(prop_value, G_VARIANT_TYPE_BOOLEAN))
        {
          LOG_WARNING(logger) << "Unable to match '"<< name << "', it's not a boolean property.";
          return false;
        }

        return (prop_value.GetBool() == value);
      }

      return false;
    }

    bool MatchIntegerProperty(std::string const& name, int32_t value) const
    {
      auto const& prop_value = GetPropertyValue(name);

      if (prop_value)
      {
        GVariantClass prop_val_type = g_variant_classify(prop_value);
        // it'd be nice to be able to do all this with one method.
        // I can't figure out how to group all the integer types together
        switch (prop_val_type)
        {
          case G_VARIANT_CLASS_BYTE:
            return static_cast<unsigned char>(value) == prop_value.GetByte();
          case G_VARIANT_CLASS_INT16:
            return value == prop_value.GetInt16();
          case G_VARIANT_CLASS_UINT16:
            return static_cast<uint16_t>(value) == prop_value.GetUInt16();
          case G_VARIANT_CLASS_INT32:
            return value == prop_value.GetInt32();
          case G_VARIANT_CLASS_UINT32:
            return static_cast<uint32_t>(value) == prop_value.GetUInt32();
          case G_VARIANT_CLASS_INT64:
            return value == prop_value.GetInt64();
          case G_VARIANT_CLASS_UINT64:
            return static_cast<uint64_t>(value) == prop_value.GetUInt64();
        default:
          LOG_WARNING(logger) << "Unable to match '"<< name << "' against property of unknown integer type.";
        };
      }

      return false;
    }

    glib::Variant GetPropertyValue(std::string const& name) const
    {
      if (name == "id")
        return glib::Variant(GetId());

      IntrospectionData introspection;
      node_->AddProperties(introspection);
      return g_variant_lookup_value(glib::Variant(introspection.Get()), name.c_str(), nullptr);
    }

    std::vector<xpathselect::Node::Ptr> Children() const
    {
      std::vector<xpathselect::Node::Ptr> children;
      auto const& this_ptr = shared_from_this();

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
      xpathselect_driver_ = dlopen(XPATH_SELECT_LIB.c_str(), RTLD_LAZY);

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

} // local namespace
} // anonymous namespace

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

struct DebugDBusInterface::Impl
{
  Impl(Introspectable*);

  GVariant* HandleDBusMethodCall(std::string const&, GVariant*);
  GVariant* GetState(std::string const&);

  void StartLogToFile(std::string const&);
  void ResetLogging();
  void SetLogSeverity(std::string const& log_component, std::string const& severity);
  void LogMessage(std::string const& severity, std::string const& message);

  Introspectable* introspection_root_;
  glib::DBusServer::Ptr server_;
  std::ofstream output_file_;
};

DebugDBusInterface::DebugDBusInterface(Introspectable* root)
  : impl_(new DebugDBusInterface::Impl(root))
{}

DebugDBusInterface::~DebugDBusInterface()
{}

DebugDBusInterface::Impl::Impl(Introspectable* root)
  : introspection_root_(root)
  , server_(std::make_shared<glib::DBusServer>(dbus::BUS_NAME))
{
  if (server_)
  {
    server_->AddObjects(dbus::INTROSPECTION_XML, dbus::OBJECT_PATH);

    for (auto const& obj : server_->GetObjects())
      obj->SetMethodsCallsHandler(sigc::mem_fun(this, &Impl::HandleDBusMethodCall));
  }
}

GVariant* DebugDBusInterface::Impl::HandleDBusMethodCall(std::string const& method, GVariant* parameters)
{
  if (method == "GetState")
  {
    const gchar* input;
    g_variant_get(parameters, "(&s)", &input);

    return GetState(input);
  }
  else if (method == "GetVersion")
  {
    return g_variant_new("(s)", local::PROTOCOL_VERSION.c_str());
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

GVariant* DebugDBusInterface::Impl::GetState(std::string const& query)
{
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("a(sv)"));

  auto root_node = std::make_shared<local::IntrospectableAdapter>(introspection_root_);
  for (auto const& n : local::select_nodes(root_node, query))
  {
    auto p = std::static_pointer_cast<local::IntrospectableAdapter const>(n);
    if (p)
      g_variant_builder_add(&builder, "(sv)", p->GetPath().c_str(), p->Node()->Introspect());
  }

  return g_variant_new("(a(sv))", &builder);
}

void DebugDBusInterface::Impl::StartLogToFile(std::string const& file_path)
{
  if (output_file_.is_open())
    output_file_.close();

  output_file_.open(file_path);
  nux::logging::Writer::Instance().SetOutputStream(output_file_);
}

void DebugDBusInterface::Impl::ResetLogging()
{
  if (output_file_.is_open())
    output_file_.close();

  nux::logging::Writer::Instance().SetOutputStream(std::cout);
  nux::logging::reset_logging();
}

void DebugDBusInterface::Impl::SetLogSeverity(std::string const& log_component, std::string const& severity)
{
  nux::logging::Logger(log_component).SetLogLevel(nux::logging::get_logging_level(severity));
}

void DebugDBusInterface::Impl::LogMessage(std::string const& severity, std::string const& message)
{
  nux::logging::Level level = nux::logging::get_logging_level(severity);
  nux::logging::Logger const& log_ref = Unwrap(logger);
  if (log_ref.GetEffectiveLogLevel() <= level)
  {
    nux::logging::LogStream(level, log_ref.module(), __FILE__, __LINE__).stream()
      << message;
  }
}

} // debug namepsace
} // unity namespace
