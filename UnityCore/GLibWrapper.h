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

#ifndef UNITY_GLIB_WRAPPER_H
#define UNITY_GLIB_WRAPPER_H

#include <iosfwd>
#include <string>

#include <boost/utility.hpp>
#include <glib.h>

namespace unity
{
namespace glib
{

template <typename T>
class Object
{
public:
  Object();
  explicit Object(T* val);
  Object(Object const&);
  ~Object();

  Object& operator=(T* val);
  Object& operator=(Object const& other);

  operator T* ();
  operator bool();
  T* operator->();
  T* RawPtr();
  // Release ownership of the object. No unref will occur.
  T* Release();

private:
  T* object_;
};

class Error : boost::noncopyable
{
public:
  Error();
  ~Error();

  GError** AsOutParam();
  GError** operator&();

  operator bool() const;
  std::string Message() const;

private:
  GError* error_;
};

class String : boost::noncopyable
{
public:
  String();
  explicit String(gchar* str);
  ~String();

  gchar** AsOutParam();
  gchar** operator&();

  gchar* Value();
  std::string Str() const;

private:
  gchar* string_;
};

std::ostream& operator<<(std::ostream& o, Error const& e);
std::ostream& operator<<(std::ostream& o, String const& s);

}
}

#include "GLibWrapper-inl.h"

#endif
