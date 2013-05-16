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

namespace unity
{
namespace glib
{

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

GError** Error::operator&()
{
  return &error_;
}

Error::operator bool() const
{
  return bool(error_);
}

Error::operator GError* ()
{
    return error_;
}

std::string Error::Message() const
{
  std::string result;
  if (error_)
    result = error_->message;
  return result;
}

std::ostream& operator<<(std::ostream& o, Error const& e)
{
  if (e)
    o << e.Message();
  return o;
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

gchar** String::operator&()
{
  return &string_;
}

gchar* String::Value()
{
  return string_;
}

String::operator char*()
{
  return string_;
}

String::operator std::string()
{
  return Str();
}

String::operator bool() const
{
  return bool(string_);
}

std::string String::Str() const
{
  return glib::gchar_to_string(string_);
}

std::ostream& operator<<(std::ostream& o, String const& s)
{
  if (s)
    o << s.Str();
  else
    o << "<null>";
  return o;
}


Cancellable::Cancellable()
  : cancellable_(g_cancellable_new())
{}

Cancellable::~Cancellable()
{
  Cancel();
}

Cancellable::operator GCancellable*()
{
  return cancellable_;
}

Cancellable::operator Object<GCancellable>()
{
  return cancellable_;
}

Object<GCancellable> Cancellable::Get() const
{
  return cancellable_;
}

bool Cancellable::IsCancelled() const
{
  return g_cancellable_is_cancelled(cancellable_) != FALSE;
}

bool Cancellable::IsCancelled(glib::Error &error) const
{
  return g_cancellable_set_error_if_cancelled(cancellable_, &error) != FALSE;
}

void Cancellable::Cancel()
{
  g_cancellable_cancel(cancellable_);
}

void Cancellable::Reset()
{
  g_cancellable_reset(cancellable_);
}

void Cancellable::Renew()
{
  Cancel();
  cancellable_ = g_cancellable_new();
}

bool operator==(Cancellable const& lhs, Cancellable const& rhs)
{
  return (lhs.Get() == rhs.Get());
}

bool operator!=(Cancellable const& lhs, Cancellable const& rhs)
{
  return !(lhs == rhs);
}

bool operator==(GCancellable* lhs, Cancellable const& rhs)
{
  return lhs == rhs.Get();
}

bool operator!=(GCancellable* lhs, Cancellable const& rhs)
{
  return !(lhs == rhs.Get());
}

bool operator==(Object<GCancellable> const& lhs, Cancellable const& rhs)
{
  return lhs == rhs.Get();
}

bool operator!=(Object<GCancellable> const& lhs, Cancellable const& rhs)
{
  return !(lhs == rhs.Get());
}


}
}

