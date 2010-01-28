namespace Unity
{
  [CCode (cheader_filename = "unity-places/unity-places.h")]
  public class Place
  {
    public virtual signal void view_changed (string view_name, GLib.HashTable <string,string> view_properties);
  }
}
