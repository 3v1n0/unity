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
* Authored by: Neil Jagdish patel <neil.patel@canonical.com>
*/

#ifndef UNITY_GLIB_SIGNAL_INL_H
#define UNITY_GLIB_SIGNAL_INL_H

namespace unity {
namespace glib {

template <typename R, typename O>
void Signal0<R, O>::Connect(O                  object,
                            std::string const& signal_name,
                            SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK(Callback), this);
}

template <typename R, typename O>
R Signal0<R, O>::Callback(O object, Signal0* self)
{
  return self->callback_ (object);
}
  
template <typename R, typename O, typename T>
void Signal1<R, O, T>::Connect(O                  object,
                               std::string const& signal_name,
                               SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK(Callback), this);
}

template <typename R, typename O, typename T>
R Signal1<R, O, T>::Callback(O object, T data1, Signal1* self)
{
  return self->callback_ (object, data1);
}

template <typename R, typename O, typename T1, typename T2>
void Signal2<R, O, T1, T2>::Connect(O                  object,
                                    std::string const& signal_name,
                                    SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename O, typename T1, typename T2>
R Signal2<R, O, T1, T2>::Callback(O object,
                                  T1       data1,
                                  T2       data2,
                                  Signal2* self)
{
  return self->callback_ (object, data1, data2);
}

template <typename R, typename O, typename T1, typename T2, typename T3>
void Signal3<R, O, T1, T2, T3>::Connect(O                  object,
                                        std::string const& signal_name,
                                        SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename O, typename T1, typename T2, typename T3>
R Signal3<R, O, T1, T2, T3>::Callback(O object,
                                      T1       data1,
                                      T2       data2,
                                      T3       data3,
                                      Signal3* self)
{
  return self->callback_ (object, data1, data2, data3);
}

template <typename R, typename O, typename T1, typename T2, typename T3, typename T4>
void Signal4<R, O, T1, T2, T3, T4>::Connect(O                  object,
                                            std::string const& signal_name,
                                            SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename O, typename T1, typename T2, typename T3, typename T4>
R Signal4<R, O, T1, T2, T3, T4>::Callback(O object,
                                          T1       data1,
                                          T2       data2,
                                          T3       data3,
                                          T4       data4,
                                          Signal4* self)
{
  return self->callback_ (object, data1, data2, data3, data4);
}

template <typename R, typename O, typename T1, typename T2,
          typename T3, typename T4, typename T5 >
void Signal5<R, O, T1, T2, T3, T4, T5>::Connect(O                  object,
                                                std::string const& signal_name,
                                                SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename O, typename T1, typename T2,
          typename T3, typename T4, typename T5>
R Signal5<R, O, T1, T2, T3, T4, T5>::Callback(O object,
                                              T1       data1,
                                              T2       data2,
                                              T3       data3,
                                              T4       data4,
                                              T5       data5,
                                              Signal5* self)
{
  return self->callback_ (object, data1, data2, data3, data4, data5);
}

template <typename R, typename O, typename T1, typename T2,
          typename T3, typename T4, typename T5 , typename T6>
void Signal6<R, O, T1, T2, T3, T4, T5, T6>::Connect(O                  object,
                                                    std::string const& signal_name,
                                                    SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename O, typename T1, typename T2,
          typename T3, typename T4, typename T5, typename T6>
R Signal6<R, O, T1, T2, T3, T4, T5, T6>::Callback(O object,
                                                  T1       data1,
                                                  T2       data2,
                                                  T3       data3,
                                                  T4       data4,
                                                  T5       data5,
                                                  T6       data6,
                                                  Signal6* self)
{
  return self->callback_ (object, data1, data2, data3, data4, data5, data6);
}

template<typename R, typename O>
Signal<R, O>::Signal()
{}

template<typename R, typename O>
Signal<R, O>::Signal(O object, std::string signal_name, SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1>
Signal<R, O, T1>::Signal()
{}

template<typename R, typename O, typename T1>
Signal<R, O, T1>::Signal(O object, std::string signal_name, SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1, typename T2>
Signal<R, O, T1, T2>::Signal()
{}

template<typename R, typename O, typename T1, typename T2>
Signal<R, O, T1, T2>::Signal(O              object,
                             std::string    signal_name,
                             SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1, typename T2, typename T3>
Signal<R, O, T1, T2, T3>::Signal()
{}

template<typename R, typename O, typename T1, typename T2, typename T3>
Signal<R, O, T1, T2, T3>::Signal(O              object,
                                 std::string    signal_name,
                                 SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4>
Signal<R, O, T1, T2, T3, T4>::Signal()
{}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4>
Signal<R, O, T1, T2, T3, T4>::Signal(O              object,
                                     std::string    signal_name,
                                     SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4,
         typename T5>
Signal<R, O, T1, T2, T3, T4, T5>::Signal()
{}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4,
         typename T5>
Signal<R, O, T1, T2, T3, T4, T5>::Signal(O              object,
                                         std::string    signal_name,
                                         SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6>
Signal<R, O, T1, T2, T3, T4, T5, T6>::Signal()
{}

template<typename R, typename O, typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6>
Signal<R, O, T1, T2, T3, T4, T5, T6>::Signal(O              object,
                                             std::string    signal_name,
                                             SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

}
}

#endif
