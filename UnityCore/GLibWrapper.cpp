// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011 Canonical Ltd
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

#include "GLibWrapper.h"

namespace unity {
namespace glib {

Error::Error()
  : error_(0)
{}

Error::~Error()
{
  if (error_)
    g_error_free(error_);
}

GError** Error::AsOutParam()
{
  return &error_;
}

Error::operator bool() const
{
  return bool(error_);
}

std::string Error::Message() const
{
  std::string result;
  if (error_)
    result = error_->message;
  return result;
}


String::String()
  : string_(0)
{}

String::String(gchar* str)
  : string_(str)
{}

String::~String()
{
  if (string_)
    g_free(string_);
}

gchar** String::AsOutParam()
{
  return &string_;
}

gchar* String::Value()
{
  return string_;
}

std::string String::Str() const
{
  if (string_)
    return std::string(string_);
  else
    return std::string("");
}


}
}

