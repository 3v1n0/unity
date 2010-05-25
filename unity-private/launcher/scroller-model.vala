/*
 *      scroller-model.vala
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
using Gee;

namespace Unity.Launcher
{
  /* the idea here is that this model should find the best order of the
   * ScrollerChild's but at the same time, can be re-ordered by other objects.
   *
   * At the moment its almost a very basic list model just because
   * there is no good order code to use right now. in the future we will expand
   * on this to support saving the position, grouping and such.
   */
  public class ScrollerModel : Object
  {
    private ArrayList<ScrollerChild> children;
    public int size { get { return children.size; } }

    public signal void child_added (ScrollerChild child);
    public signal void child_removed (ScrollerChild child);
    public signal void order_changed (); // called when the order of items changes

    public ScrollerModel ()
    {
    }

    construct
    {
      Unity.Testing.ObjectRegistry.get_default ().register ("UnityScrollerModel", this);
      children = new ArrayList<ScrollerChild> ();
    }

    public string to_string ()
    {
      return "a ScrollerModel model with %i entries".printf (children.size);
    }

    /* we proxy through nice gee list handling methods because
     * we are not an abstract list, we are a model for a certain data type
     * but want to provide a list interface to that data, technically we are not
     * a Model but a Collection
     */
    public Iterator iterator ()
    {
      return new Iterator (children);
    }

    public class Iterator {
        private int iter_index = 0;
        private ArrayList<ScrollerChild> array;

        public Iterator(ArrayList arraylist)
          {
            array = arraylist;
          }

        public bool next()
          {
            if (iter_index >= array.size)
              return false;
            return true;
          }

        public ScrollerChild get()
          {
            iter_index++;
            return array[iter_index - 1];
          }
    }

    public bool contains(ScrollerChild child) {
        return child in children;
    }

    public void add (ScrollerChild child)
    {
      children.add (child);
      child_added (child);
      order_changed ();
    }

    public void remove (ScrollerChild child)
    {
			var tempchild = child;
      children.remove (child);
      child_removed (tempchild);
      order_changed ();
    }

    public void insert (ScrollerChild child, int i)
    {
      children.insert (i, child);
      child_added (child);
    }

		public void move (ScrollerChild child, int i)
			{
				if (!(child in children))
					return;

				children.remove (child);
				children.insert (i, child);
				order_changed ();
			}

    public void sort (CompareFunc compare)
    {
      children.sort (compare);
    }

    public new ScrollerChild get (int i)
    {
      return children[i];
    }

    public new void set (int i, ScrollerChild item)
    {
      children[i] = item;
    }

  }

}
