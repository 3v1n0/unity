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

template <typename R, typename G>
Signal0<R, G>::Signal0()
{}

template <typename R, typename G>
void Signal0<R, G>::Connect(G                  object,
                            std::string const& signal_name,
                            SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK(Callback), this);
}

template <typename R, typename G>
R Signal0<R, G>::Callback(G object, Signal0* self)
{
  return self->callback_(object);
}

template <typename R, typename G, typename T>
Signal1<R, G, T>::Signal1()
{}

template <typename R, typename G, typename T>
void Signal1<R, G, T>::Connect(G                  object,
                               std::string const& signal_name,
                               SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK(Callback), this);
}

template <typename R, typename G, typename T>
R Signal1<R, G, T>::Callback(G object, T data1, Signal1* self)
{
  return self->callback_(object, data1);
}

template <typename R, typename G, typename T1, typename T2>
Signal2<R, G, T1, T2>::Signal2()
{}

template <typename R, typename G, typename T1, typename T2>
void Signal2<R, G, T1, T2>::Connect(G                  object,
                                    std::string const& signal_name,
                                    SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename G, typename T1, typename T2>
R Signal2<R, G, T1, T2>::Callback(G object,
                                  T1       data1,
                                  T2       data2,
                                  Signal2* self)
{
  return self->callback_(object, data1, data2);
}

template <typename R, typename G, typename T1, typename T2, typename T3>
Signal3<R, G, T1, T2, T3>::Signal3()
{}

template <typename R, typename G, typename T1, typename T2, typename T3>
void Signal3<R, G, T1, T2, T3>::Connect(G                  object,
                                        std::string const& signal_name,
                                        SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename G, typename T1, typename T2, typename T3>
R Signal3<R, G, T1, T2, T3>::Callback(G object,
                                      T1       data1,
                                      T2       data2,
                                      T3       data3,
                                      Signal3* self)
{
  return self->callback_(object, data1, data2, data3);
}

template <typename R, typename G, typename T1, typename T2, typename T3, typename T4>
Signal4<R, G, T1, T2, T3, T4>::Signal4()
{}

template <typename R, typename G, typename T1, typename T2, typename T3, typename T4>
void Signal4<R, G, T1, T2, T3, T4>::Connect(G                  object,
                                            std::string const& signal_name,
                                            SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename G, typename T1, typename T2, typename T3, typename T4>
R Signal4<R, G, T1, T2, T3, T4>::Callback(G object,
                                          T1       data1,
                                          T2       data2,
                                          T3       data3,
                                          T4       data4,
                                          Signal4* self)
{
  return self->callback_(object, data1, data2, data3, data4);
}


template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5>
Signal5<R, G, T1, T2, T3, T4, T5>::Signal5()
{}

template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5 >
void Signal5<R, G, T1, T2, T3, T4, T5>::Connect(G                  object,
                                                std::string const& signal_name,
                                                SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5>
R Signal5<R, G, T1, T2, T3, T4, T5>::Callback(G object,
                                              T1       data1,
                                              T2       data2,
                                              T3       data3,
                                              T4       data4,
                                              T5       data5,
                                              Signal5* self)
{
  return self->callback_(object, data1, data2, data3, data4, data5);
}

template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5 , typename T6>
Signal6<R, G, T1, T2, T3, T4, T5, T6>::Signal6()
{}

template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5 , typename T6>
void Signal6<R, G, T1, T2, T3, T4, T5, T6>::Connect(G                  object,
                                                    std::string const& signal_name,
                                                    SignalCallback     callback)
{
  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object, signal_name.c_str (),
                                    G_CALLBACK (Callback), this);
}

template <typename R, typename G, typename T1, typename T2,
          typename T3, typename T4, typename T5, typename T6>
R Signal6<R, G, T1, T2, T3, T4, T5, T6>::Callback(G object,
                                                  T1       data1,
                                                  T2       data2,
                                                  T3       data3,
                                                  T4       data4,
                                                  T5       data5,
                                                  T6       data6,
                                                  Signal6* self)
{
  return self->callback_(object, data1, data2, data3, data4, data5, data6);
}

template<typename R, typename G>
Signal<R, G>::Signal()
{}

template<typename R, typename G>
Signal<R, G>::Signal(G object, std::string signal_name, SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1>
Signal<R, G, T1>::Signal()
{}

template<typename R, typename G, typename T1>
Signal<R, G, T1>::Signal(G object, std::string signal_name, SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1, typename T2>
Signal<R, G, T1, T2>::Signal()
{}

template<typename R, typename G, typename T1, typename T2>
Signal<R, G, T1, T2>::Signal(G              object,
                             std::string    signal_name,
                             SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1, typename T2, typename T3>
Signal<R, G, T1, T2, T3>::Signal()
{}

template<typename R, typename G, typename T1, typename T2, typename T3>
Signal<R, G, T1, T2, T3>::Signal(G              object,
                                 std::string    signal_name,
                                 SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4>
Signal<R, G, T1, T2, T3, T4>::Signal()
{}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4>
Signal<R, G, T1, T2, T3, T4>::Signal(G              object,
                                     std::string    signal_name,
                                     SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4,
         typename T5>
Signal<R, G, T1, T2, T3, T4, T5>::Signal()
{}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4,
         typename T5>
Signal<R, G, T1, T2, T3, T4, T5>::Signal(G              object,
                                         std::string    signal_name,
                                         SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6>
Signal<R, G, T1, T2, T3, T4, T5, T6>::Signal()
{}

template<typename R, typename G, typename T1, typename T2, typename T3, typename T4,
         typename T5, typename T6>
Signal<R, G, T1, T2, T3, T4, T5, T6>::Signal(G              object,
                                             std::string    signal_name,
                                             SignalCallback callback)
{
  Connect(object, signal_name, callback);
}

}
}

#endif
