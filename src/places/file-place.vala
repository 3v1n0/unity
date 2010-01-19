
namespace Unity.Places.File
{

  const string FILES_FILE = Unity.PKGDATADIR + "/files.png";
  const string FAVOURITES_ICON = Unity.PKGDATADIR + "/favourites.png";

  public class FileIcon
  {
    private Unity.Places.Bar.Model model;
    public Ctk.Image view;

    public FileIcon (int width, /*int height,*/ string name, string icon_name, string tooltip)
    {
      model = new Unity.Places.Bar.Model (name, icon_name, tooltip);
      view = new Ctk.Image.from_filename (width, icon_name);
      view.set_reactive (true);
    }
  }

  public class PlacesHSeparator : Ctk.Bin
  {
    public int Width = 0;
    public int Height = 0;

    public Clutter.CairoTexture cairotxt;
    public PlacesHSeparator ()
    {
    }

    public void CreateSeparator (int W, int H)
    {
      Width = W;
      Height = H;

      if (cairotxt != null)
        this.remove_actor (cairotxt);

      cairotxt = new Clutter.CairoTexture(Width, Height);
      Cairo.Context cairoctx = cairotxt.create();
      {
        cairoctx.set_source_rgba (1, 1, 1, 1.0);
        cairoctx.set_line_width (1.0);

        cairoctx.move_to (0, Height/2.0);
        cairoctx.line_to (Width, Height/2.0);

        cairoctx.stroke ();
        cairoctx.set_source_rgba (1, 1, 1, 0.15);
      }

      cairotxt.set_opacity (0xFF);
      this.add_actor (cairotxt);

      Ctk.EffectGlow effect_glow = new Ctk.EffectGlow ();
      Clutter.Color c = Clutter.Color ()
      {
        red = 255,
        green = 255,
        blue = 255,
        alpha = 255
      };

      effect_glow.set_color (c);
      effect_glow.set_factor (1.0f);
      effect_glow.set_margin (5);
      this.add_effect (effect_glow);
    }

    construct
    {
    }
  }

  public class FileGroup : Ctk.Box
  {
    public Ctk.Text GroupName;
    public Ctk.Image Star;
    public PlacesHSeparator Separator;
    private Gee.ArrayList<FileIcon> FileIconArray;


    public override void allocate (Clutter.ActorBox        box, 
                                   Clutter.AllocationFlags flags)
    {
      int IconSize = 48;
      int IconWidth = 48;
      int IconHeight = 48;
      int FileIconSpacing = 24;

      int HeaderBorderPositionX = 100;
      int HeaderBorderPositionY = 60;
      int IconBorderPositionX = 100;
      int IconBorderPositionY = 100;
      int LineBorderPositionX = 100;
      int LineBorderPositionY = 90;
      
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };
     
      base.allocate (box, flags);

      child_box.x1 = IconBorderPositionX;
      child_box.x2 = 0;
      child_box.y1 = IconBorderPositionY;
      child_box.y2 = child_box.y1 + IconHeight;

      int i;
      for (i = 0; i < this.FileIconArray.size; i++)
        {
          child_box.x2 = child_box.x1 + IconWidth;
          this.FileIconArray[i].view.allocate(child_box, flags);
          child_box.x1 += IconWidth + FileIconSpacing;
        }

      if (this.Separator.Width != 800)
        {
          this.Separator.CreateSeparator ( 800, 5);
        }
      child_box.x1 = LineBorderPositionX;
      child_box.x2 = child_box.x1 + box.x2 - box.x1;
      child_box.y1 = LineBorderPositionY;
      child_box.y2 = LineBorderPositionY + 5;
      this.Separator.allocate (child_box, flags);

      child_box.x1 = HeaderBorderPositionX;
      child_box.x2 = child_box.x1 + 22;
      child_box.y1 = HeaderBorderPositionY;
      child_box.y2 = HeaderBorderPositionY+ 23;
      this.Star.allocate (child_box, flags);

      child_box.x1 = HeaderBorderPositionX + 22;
      child_box.x2 = child_box.x1 + 100;
      child_box.y1 = HeaderBorderPositionY;
      child_box.y2 = HeaderBorderPositionY + 23;
      this.GroupName.allocate (child_box, flags);
    }

    public FileGroup ()
    {
      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL;

      GroupName = new Ctk.Text ("Recent Files");
      this.Star = new Ctk.Image.from_filename (23, FAVOURITES_ICON);
      this.Separator = new PlacesHSeparator ();
      FileIconArray = new Gee.ArrayList<FileIcon> ();
      for (int i = 0; i < 5; i++)
      {
        FileIcon file = new FileIcon (48, "Files",
                                      FILES_FILE,
                                      "Your files stored locally");
        this.FileIconArray.add (file);
        this.add_actor (file.view);
      }

      this.add_actor (this.Star);
      this.add_actor (this.Separator);
      this.add_actor (this.GroupName);
      this.show_all ();
    }

    construct
    {
    }
  }

  public class FileView : Ctk.Box
  {
    public FileGroup file_group;
    public override void allocate (Clutter.ActorBox        box, 
                                   Clutter.AllocationFlags flags)
    {
      base.allocate (box, flags);
      
      file_group.allocate (box, flags);
    }

    public FileView ()
    {
      file_group = new FileGroup ();
      
      this.add_actor (file_group);
      this.show_all ();
    }

    construct
    {
    }
  }
}
