/*
 *      unity-drag.vala
 *      Copyright (C) 2010 Canonical Ltd
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 *      
 *      
 *      Authored by Gordon Allott <gord.allott@canonical.com>
 */


namespace Unity.Drag
{
  public interface Model : Object
  {
    public abstract Clutter.Actor get_icon ();
    public abstract string get_drag_data ();
  }

  public Controller? controller_singleton;
  
  public class Controller : Object
  {
    // returns the controller singleton. we only want one
    public static unowned Unity.Drag.Controller get_default ()
    {
      if (Unity.Drag.controller_singleton == null) {
        Unity.Drag.controller_singleton = new Unity.Drag.Controller ();
      }
        return Unity.Drag.controller_singleton;
    }

    // normal class starts here
    Unity.Drag.Model? model;
    Unity.Drag.View? view;
    
    /* on drag_start clients should inspect model.get_data () - if the data is
     * data the model is interested in, it should connect to the drag_motion and
     * drag_drop signals
     */
    public signal void drag_start (Model model);
    public signal void drag_motion (Model model, float x, float y);
    public signal void drag_drop (Model model, float x, float y);

    public bool is_dragging {
      get { return this._is_dragging; }
    }
    private bool _is_dragging;

    construct
    {
      this._is_dragging = false;
    }
    
    public void start_drag (Unity.Drag.Model model, float offset_x, float offset_y)
    {
      if (!(this.view is View)) {
        this.view = new View (model.get_icon ().get_stage () as Clutter.Stage);
      }
      this.view.hook_actor_to_cursor (model.get_icon (), offset_x, offset_y);
      this.model = model;
      this.drag_start (model);
      this.view.motion.connect (on_view_motion);
      this.view.end.connect (on_view_end);
      this._is_dragging = true;
    }

    private void on_view_motion (float x, float y)
    {
      this.drag_motion (this.model, x, y);
    }

    private void on_view_end (float x, float y)
    {
      this.view.unhook_actor ();
      this.drag_drop (this.model, x, y);
      this.view.motion.disconnect (on_view_motion);
      this.view.end.disconnect (on_view_end);
      this.model = null;
      this._is_dragging = false;
    }
    

  }

}
