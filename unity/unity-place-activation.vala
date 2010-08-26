/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

/*
 * IMPLEMENTATION NOTE:
 * We want the generatedd C API to be nice and not too Vala-ish. We must
 * anticipate that place daemons consuming libunity will be written in
 * both Vala and C.
 *
 */
namespace Unity.Place {

  /**
   * UnityPlaceActivation:
   *
   * Interface for async launching of URIs. Instances implementing
   * this interface can be registered with a Unity.Place.Controller by
   * calling set_activation (activation) on the controller object.
   */
  [DBus (name = "com.canonical.Unity.Activation")]
  public interface Activation : GLib.Object
  {
    /**
     * Returns 0 if the URI has not been activated, 1 if it has been
     * activated and the dash should not hide, and 2 if the URI has
     * been activated and the dash should hide.
     *
     * You can use the ActivationStatus enumeration to have type safe
     * return values.
     */
    public async abstract uint32 activate (string uri) throws DBus.Error;
  }
  
  /**
   * Enumeration of return values for Activation.activate().
   */
  public enum ActivationStatus
  {
    NOT_ACTIVATED,
    ACTIVATED_SHOW_DASH,
    ACTIVATED_HIDE_DASH
  }

} /* namespace */
