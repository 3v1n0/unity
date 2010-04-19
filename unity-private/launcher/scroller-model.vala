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
  class ScrollerModel : Object
  {
    private ArrayList<ScrollerChild> children;

    construct
    {
      children = new ArrayList<ScrollerChild> ();
    }

    /* we proxy through nice gee list handling methods because
     * we are not an abstract list, we are a model for a certain data type
     * but want to provide a list interface to that data, technically we are not
     * a Model but a Collection
     */
    public Iterator iterator ()
    {
      return new Iterator (children);//children.iterator ();
    }

    public class Iterator {
        private int index;
        private ArrayList<ScrollerChild> array;

        public Iterator(ArrayList arraylist) {
            array = arraylist;
            index = 0;
        }

        public bool next() {
            return true;
        }

        public ScrollerChild get() {
            index++;
            return array[index];
        }
    }


    public void add (ScrollerChild child)
    {
      children.add (child);
    }

    public void remove (ScrollerChild child)
    {
      children.remove (child);
    }

    public void sort (CompareFunc compare)
    {
      children.sort (compare);
    }

    public new ScrollerChild get (int index)
    {
      return children[index];
    }

    public new void set (int index, ScrollerChild item)
    {
      children[index] = item;
    }

  }

}
