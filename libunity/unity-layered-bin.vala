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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity
{
  /*
   * Everything added is allocated at the same size (minus padding on the bin)
   * and then painted in the direction of first-added is bottom, last-added
   * is painted last. You can changed the order using the Clutter.Actor.raise*
   * Clutter.Actor.lower* functions.
   */
  public class LayeredBin : Ctk.Actor, Clutter.Container
  {
    private Gee.List<Clutter.Actor> _children = new Gee.LinkedList<Clutter.Actor>();

    public unowned Gee.List<Clutter.Actor> children {
      get { return _children; }
    }

    public LayeredBin ()
    {
      Object ();
    }

    private override void dispose ()
    {
      foreach (Clutter.Actor child in _children)
        {
          child.unparent ();
        }

      _children.clear ();
      
      /* Chain up */
      base.dispose ();
    }

    construct
    {

    }
    
    /* ClutterActor */
    private override void allocate (Clutter.ActorBox        box,
                                    Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = Clutter.ActorBox ();
      child_box.x1 = padding.left;
      child_box.x2 = box.x2 - box.x1 - padding.left - padding.right;
      child_box.y1 = padding.top;
      child_box.y2 = box.y2 - box.y1 - padding.top - padding.bottom;

      base.allocate (box, flags);
      
      foreach (Clutter.Actor child in _children)
        {
          child.allocate (child_box, flags);
        }
    }

    private override void get_preferred_width (float for_height,
                                               out float mwidth,
                                               out float nwidth)
    {
      foreach (Clutter.Actor child in _children)
        {
          float m = 0.0f, n = 0.0f;

          child.get_preferred_width (for_height, out m, out n);

          mwidth = Math.fmaxf (m, mwidth);
          nwidth = Math.fmaxf (n, nwidth);
        }

      mwidth += padding.left + padding.right;
      nwidth += padding.left + padding.right;
    }

    private override void get_preferred_height (float for_width,
                                                out float mheight,
                                                out float nheight)
    {
      foreach (Clutter.Actor child in _children)
        {
          float m = 0.0f, n = 0.0f;

          child.get_preferred_height (for_width, out m, out n);

          mheight = Math.fmaxf (m, mheight);
          nheight = Math.fmaxf (n, nheight);
        }

      mheight += padding.top + padding.bottom;
      nheight += padding.top + padding.bottom;
    }

    private override void paint ()
    {
      base.paint ();
      foreach (Clutter.Actor child in _children)
        child.paint ();
    }

    private override void pick (Clutter.Color color)
    {
      base.pick (color);
      foreach (Clutter.Actor child in _children)
        child.paint ();
    }

    /* ClutterContainerIface */
    private void add (Clutter.Actor actor)
    {
      actor.set_parent (this);
      _children.add (actor);

      queue_relayout ();

      actor_added (actor);
    }

    private void remove (Clutter.Actor actor)
    {
      actor.unparent ();
      _children.remove (actor);

      queue_relayout ();

      actor_removed (actor);
    }

    private void foreach (Clutter.Callback callback,
                          void *           data)
    {
      foreach (Clutter.Actor actor in _children)
        callback (actor, data);
    }

    private void foreach_with_internals (Clutter.Callback callback,
                                         void *           data)
    {
      this.foreach (callback, data);
    }

    private new void raise (Clutter.Actor actor, Clutter.Actor sibling)
    {
      if (sibling is Clutter.Actor &&
          _children.index_of (sibling) != -1)
        {
          _children.remove (actor);
          _children.insert (_children.index_of (sibling) + 1, actor);
        }
      else
        {
          _children.remove (actor);
          _children.add (actor);
        }
    }

    private new void lower (Clutter.Actor actor, Clutter.Actor sibling)
    {
      if (sibling is Clutter.Actor &&
          _children.index_of (sibling) != -1)
        {
          _children.remove (actor);
          _children.insert (_children.index_of (sibling) - 1, actor);
        }
      else
        {
          _children.remove (actor);
          _children.insert (0, actor);
        }
    }

    private void sort_depth_order ()
    {
      /* Nah */
    }

    private void create_child_meta (Clutter.Actor actor)
    {
      ;
    }

    private void destroy_child_meta (Clutter.Actor actor)
    {
      ;
    }

    private Clutter.ChildMeta? get_child_meta (Clutter.Actor actor)
    {
      return null;
    }
  }
}
