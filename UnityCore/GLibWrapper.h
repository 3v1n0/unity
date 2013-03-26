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
*              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
*/

#ifndef UNITY_GLIB_WRAPPER_H
#define UNITY_GLIB_WRAPPER_H

#include <iosfwd>
#include <string>
#include <memory>

#include <boost/utility.hpp>
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

namespace unity
{
namespace glib
{

struct AddRef{};

template <typename T>
class Object
{
public:
  Object();
  explicit Object(T* val);
  Object(T* val, AddRef const& ref);

  Object(Object const&);
  ~Object();

  void swap(Object<T>& other);

  Object& operator=(T* val);
  Object& operator=(Object other);

  operator T* () const;
  operator bool() const;
  T* operator->() const;
  T* RawPtr() const;
  // Release ownership of the object. No unref will occur.
  T* Release();
  bool IsType(GType type) const;

private:
  T* object_;
};

template <typename T>
bool operator==(Object<T> const& lhs, Object<T> const& rhs)
{
  return (lhs.RawPtr() == rhs.RawPtr());
}

template <typename T>
bool operator!=(Object<T> const& lhs, Object<T> const& rhs)
{
  return !(lhs == rhs);
}

template <typename T>
bool operator!=(T* lhs, Object<T> const& rhs)
{
  return !(lhs == rhs.RawPtr());
}

template <typename G, typename T>
Object<G> object_cast(Object<T> const& obj)
{
  return Object<G>(reinterpret_cast<G*>(obj.RawPtr()), AddRef());
}

class Error : boost::noncopyable
{
public:
  Error();
  ~Error();

  GError** AsOutParam();
  GError** operator&();

  operator GError*();

  operator bool() const;
  std::string Message() const;

private:
  GError* error_;
};

// wrapper for raw gcha*. auto-deleted.
class String : boost::noncopyable
{
public:
  String();
  explicit String(gchar* str);
  ~String();

  gchar** AsOutParam();
  gchar** operator&();

  operator bool() const;
  operator char*();
  operator std::string();
  gchar* Value();
  std::string Str() const;

private:
  gchar* string_;
};

inline std::string gchar_to_string(const gchar* str)
{
  if (!str)
    return std::string("");
  else
    return std::string(str);
}

class Cancellable : boost::noncopyable
{
public:
  typedef std::shared_ptr<Cancellable> Ptr;

  Cancellable();
  ~Cancellable();

  operator GCancellable*();
  operator Object<GCancellable>();

  Object<GCancellable> Get() const;
  bool IsCancelled() const;
  bool IsCancelled(glib::Error &error) const;
  void Cancel();
  void Reset();
  void Renew();

private:
  Object<GCancellable> cancellable_;
};

bool operator==(Cancellable const& lhs, Cancellable const& rhs);
bool operator!=(Cancellable const& lhs, Cancellable const& rhs);
bool operator==(GCancellable* lhs, Cancellable const& rhs);
bool operator!=(GCancellable* lhs, Cancellable const& rhs);
bool operator==(Object<GCancellable> const& lhs, Cancellable const& rhs);
bool operator!=(Object<GCancellable> const& lhs, Cancellable const& rhs);

std::ostream& operator<<(std::ostream& o, Error const& e);
std::ostream& operator<<(std::ostream& o, String const& s);

}
}

namespace std
{
  template <typename T> 
  void swap (unity::glib::Object<T>& lhs, unity::glib::Object<T>& rhs)
  {
    lhs.swap(rhs);
  }
}

#include "GLibWrapper-inl.h"

#endif
