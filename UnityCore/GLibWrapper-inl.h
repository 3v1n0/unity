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

#ifndef UNITY_GLIB_WRAPPER_INL_H
#define UNITY_GLIB_WRAPPER_INL_H

namespace unity {
namespace glib {

template <typename T>
Object<T>::Object()
  : object_(0)
{}

template <typename T>
Object<T>::Object(T* val)
  : object_(val)
{}

template <typename T>
Object<T>::Object(Object const& other)
  : object_(other.object_)
{
  if (object_)
    g_object_ref(object_);
}

template <typename T>
Object<T>::~Object()
{
  if (object_)
    g_object_unref(object_);
}

template <typename T>
Object<T>& Object<T>::operator=(T* val)
{
  if (object_)
    g_object_unref(object_);
  object_ = val;

  return *this;
}

template <typename T>
Object<T>& Object<T>::operator=(Object const& other)
{
  if (object_)
    g_object_unref(object_);
  object_ = other.object_;
  if (object_)
    g_object_ref(object_);

  return *this;
}

template <typename T>
Object<T>::operator T*()
{
  return object_;
}

template <typename T>
T* Object<T>::operator->()
{
  return object_;
}

template <typename T>
Object<T>::operator bool()
{
  return bool(object_);
}

template <typename T>
T* Object<T>::RawPtr()
{
  return object_;
}

template <typename T>
T* Object<T>::Release()
{
  T* result = object_;
  object_ = 0;
  return result;
}


}
}

#endif
