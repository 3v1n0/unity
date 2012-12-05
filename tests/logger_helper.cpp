// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#include "logger_helper.h"

#include <glib.h>
#include <iostream>

#include "NuxCore/Logger.h"
#include "NuxCore/LoggingWriter.h"

namespace unity
{
namespace helper
{
namespace
{

nux::logging::Level glog_level_to_nux(GLogLevelFlags log_level)
{
  // For some weird reason, ERROR is more critical than CRITICAL in gnome.
  if (log_level & G_LOG_LEVEL_ERROR)
    return nux::logging::Critical;
  if (log_level & G_LOG_LEVEL_CRITICAL)
    return nux::logging::Error;
  if (log_level & G_LOG_LEVEL_WARNING)
    return nux::logging::Warning;
  if (log_level & G_LOG_LEVEL_MESSAGE ||
      log_level & G_LOG_LEVEL_INFO)
    return nux::logging::Info;
  // default to debug.
  return nux::logging::Debug;
}

void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data)
{
  // If nothing else, all log messages from unity should be identified as such
  std::string module("unity");
  if (log_domain)
  {
    module += std::string(".") + log_domain;
  }
  nux::logging::Logger logger(module);
  nux::logging::Level level = glog_level_to_nux(log_level);
  if (level >= logger.GetEffectiveLogLevel())
  {
    nux::logging::LogStream(level, logger.module(), "<unknown>", 0).stream()
      << message;
  }
}

}


CaptureLogOutput::CaptureLogOutput()
{
  nux::logging::Writer::Instance().SetOutputStream(sout_);
}

CaptureLogOutput::~CaptureLogOutput()
{
  nux::logging::Writer::Instance().SetOutputStream(std::cout);
}

std::string CaptureLogOutput::GetOutput()
{
  std::string result = sout_.str();
  sout_.str("");
  return result;
}

void configure_logging(std::string const& env_var)
{
  const char* env = (env_var.empty() ? "UNITY_LOG_SEVERITY" : env_var.c_str());
  nux::logging::configure_logging(::getenv(env));
  g_log_set_default_handler(capture_g_log_calls, NULL);
}


}
}
