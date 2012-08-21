// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011-2012 Canonical Ltd
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
* Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
*              Marco Trevisan <marco.trevisan@canonical.com>
*/

#ifndef UNITY_GLIB_SIGNAL_H
#define UNITY_GLIB_SIGNAL_H

#include <string>
#include <vector>
#include <memory>
#include <boost/noncopyable.hpp>
#include <glib-object.h>

namespace unity
{
namespace glib
{

class SignalBase : boost::noncopyable
{
public:
  typedef std::shared_ptr<SignalBase> Ptr;

  virtual ~SignalBase();

  void Disconnect();

  GObject* object() const;
  std::string const& name() const;

protected:
  SignalBase();

  GObject* object_;
  guint32 connection_id_;
  std::string name_;
};

template <typename R, typename G, typename... Ts>
class Signal : public SignalBase
{
public:
/* Versions of GCC prior to 4.7 doesn't support the Ts... expansion, we can
 * workaround this issue using the trick below.
 * See GCC bug http://gcc.gnu.org/bugzilla/show_bug.cgi?id=35722 */
#if !defined(__GNUC__) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 7)
  typedef std::function<R(G, Ts...)> SignalCallback;
#else
  template<template <typename...> class T, typename... params>
  struct expand_variadic {
      typedef T<params...> type;
  };

  typedef typename expand_variadic<std::function, R(G, Ts...)>::type SignalCallback;
#endif

  inline Signal() {};
  inline Signal(G object, std::string const& signal_name, SignalCallback const& callback);

  inline void Connect(G Object, std::string const& signal_name, SignalCallback const& callback);

private:
  static R Callback(G Object, Ts... vs, Signal* self);

  SignalCallback callback_;
};

class SignalManager : boost::noncopyable
{
public:
  SignalManager();
  ~SignalManager();
  void Add(SignalBase* signal);
  void Add(SignalBase::Ptr const& signal);
  template <typename R, typename G, typename... Ts>
  void Add(G object, std::string const& signal_name, typename Signal<R, G, Ts...>::SignalCallback const& callback)
  {
    Add(std::make_shared<Signal<R, G, Ts...>>(object, signal_name, callback));
  }

  void Disconnect(void* object, std::string const& signal_name = "");

private:
  static void OnObjectDestroyed(SignalManager* self, GObject* old_obj);

protected:
  std::vector<SignalBase::Ptr> connections_;
};

}
}

#include "GLibSignal-inl.h"

#endif
