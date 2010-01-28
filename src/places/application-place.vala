
namespace Unity.Places.Application
{

  const string APP_ICON = Unity.PKGDATADIR + "/applications.png";
  const string FAVOURITES_ICON = Unity.PKGDATADIR + "/favourites.png";
  const string MINIMIZE_ICON = Unity.PKGDATADIR + "/minimize_up.png";
  const string MAXIMIZE_ICON = Unity.PKGDATADIR + "/maximize_up.png";

  public class ApplicationIcon
  {
    private Unity.Places.Bar.Model model;
    public Ctk.Image view;
    public Ctk.Text label;

    public ApplicationIcon (int width, /*int height,*/ string name, string icon_name, string tooltip)
    {
      model = new Unity.Places.Bar.Model (name, icon_name, tooltip);

      if (icon_name[0] == '/')
        view = new Ctk.Image.from_filename (width, icon_name);
      else if (icon_name[-4] == '.')
        view = new Ctk.Image.from_filename (width,
                                            "/usr/share/pixmaps/" + icon_name);
      else
        view = new Ctk.Image.from_stock (width, icon_name);

      view.set_reactive (true);
      label = new Ctk.Text (name);
      label.set_line_wrap (true);
      label.set_line_alignment (Pango.Alignment.CENTER);
    }
  }

  public class ApplicationGroup : Ctk.Box
  {
    public  int n_items = 0;

    private Ctk.Text GroupName;
    private Ctk.Image Star;
    private Unity.Places.CairoDrawing.PlacesHSeparator Separator;
    private Ctk.Image maximize_button;
    private Ctk.Image minimize_button;
    private bool is_open;
    private Gee.ArrayList<ApplicationIcon> application_icon_array;


    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      int width = (int)(box.x2 - box.x1);
      int IconWidth = 120;
      int IconHeight = 48;
      int ApplicationIconSpacing = 24;

      int HeaderBorderPositionX = 0;
      int HeaderBorderPositionY = 0;
      int IconBorderPositionX = 15;
      int IconBorderPositionY = 30;
      int LineBorderPositionX = 0;
      int LineBorderPositionY = 20;

      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };

      base.allocate (box, flags);

      child_box.x1 = IconBorderPositionX;
      child_box.x2 = IconWidth;
      child_box.y1 = IconBorderPositionY;
      child_box.y2 = child_box.y1 + IconHeight;

      int i;
      for (i = 0; i < this.application_icon_array.size; i++)
        {
          child_box.x2 = child_box.x1 + IconWidth;
          this.application_icon_array[i].view.allocate(child_box, flags);

          Clutter.ActorBox label_box = { 0.0f, 0.0f, 0.0f, 0.0f };
          label_box.x1 = child_box.x1;
          label_box.x2 = child_box.x2;
          label_box.y1 = IconBorderPositionY + IconHeight + 12.0f;
          label_box.y2 = label_box.y1 + 64;
          this.application_icon_array[i].label.allocate(label_box, flags);

          child_box.x1 += IconWidth + ApplicationIconSpacing;
        }

      if (this.Separator.Width != width)
        {
          this.Separator.CreateSeparator ( width, 5);
        }
      child_box.x1 = LineBorderPositionX;
      child_box.x2 = child_box.x1 + width;
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

      child_box.x1 += width - 50;
      child_box.x2 = child_box.x1 + 16;
      child_box.y1 = HeaderBorderPositionY;
      child_box.y2 = HeaderBorderPositionY + 19;
      this.maximize_button.allocate (child_box, flags);
      this.minimize_button.allocate (child_box, flags);
    }

    public override void get_preferred_width (float for_height,
                                              out float minimum_width,
                                              out float natural_width)
    {
      minimum_width = 800.0f;
      natural_width = 800.0f;
    }

    public override void get_preferred_height (float for_width,
                                               out float minimum_height,
                                               out float natural_height)
    {
      minimum_height = 135.0f;
      natural_height = 135.0f;
    }

    public ApplicationGroup (string group_name)
    {
      this.homogeneous  = false;
      this.orientation  = Ctk.Orientation.HORIZONTAL;

      this.GroupName = new Ctk.Text (group_name);
      this.Star = new Ctk.Image.from_filename (23, FAVOURITES_ICON);
      this.Separator = new Unity.Places.CairoDrawing.PlacesHSeparator ();
      this.maximize_button = new Ctk.Image.from_filename (16, MAXIMIZE_ICON);
      this.minimize_button = new Ctk.Image.from_filename (16, MINIMIZE_ICON);

      application_icon_array = new Gee.ArrayList<ApplicationIcon> ();
      /*
      for (int i = 0; i < 5; i++)
      {
        ApplicationIcon app = new ApplicationIcon (48, "App",
                                      APP_ICON,
                                      "Your apps stored locally");
        this.application_icon_array.add (app);
        this.add_actor (app.view);
        this.add_actor (app.label);
      }
*/
      this.add_actor (this.Star);
      this.add_actor (this.Separator);
      this.add_actor (this.GroupName);
      this.add_actor (this.maximize_button);
      this.add_actor (this.minimize_button);

      this.is_open = true;
      this.show_all ();

      this.maximize_button.set_reactive (true);
      this.minimize_button.set_reactive (true);
      this.maximize_button.hide();
      this.minimize_button.show();

      this.maximize_button.button_release_event.connect (this.on_maximize);
      this.minimize_button.button_release_event.connect (this.on_minimize);
    }

    public void add_icon (ApplicationIcon app)
    {
      this.application_icon_array.add (app);
      this.add_actor (app.view);
      this.add_actor (app.label);

      this.n_items++;
    }

    construct
    {
    }

    public bool on_minimize (Clutter.Event event)
    {
      this.maximize_button.show();
      this.minimize_button.hide();
      this.is_open = false;
      return false;
    }

    public bool on_maximize (Clutter.Event event)
    {
      this.maximize_button.hide();
      this.minimize_button.show();
      this.is_open = true;
      return false;
    }
  }

  public class ApplicationView : Ctk.Box
  {
    public ApplicationGroup recent_app_group;
    public ApplicationGroup yesterday_app_group;
    public ApplicationGroup lastweek_app_group;
    public override void allocate (Clutter.ActorBox        box,
                                   Clutter.AllocationFlags flags)
    {
      Clutter.ActorBox child_box = { 0.0f, 0.0f, 0.0f, 0.0f };
      float            width, height;
      float            child_width, child_height;

      base.allocate (box, flags);

      width = box.x2 - box.x1;
      height = box.y2 - box.y1;

      this.recent_app_group.get_preferred_width (height,
                                                 out child_width,
                                                 out child_width);
      this.recent_app_group.get_preferred_height (width,
                                                  out child_height,
                                                  out child_height);
      child_box.x1 = (width - child_width)/2.0f;
      child_box.x2 = child_box.x1 + child_width;
      child_box.y1 = 0.0f;
      child_box.y2 = child_height;
      recent_app_group.allocate (child_box, flags);

      child_box.y1 += child_height;
      child_box.y2 = child_box.y1 + child_height;
      yesterday_app_group.allocate (child_box, flags);

      child_box.y1 += child_height;
      child_box.y2 = child_box.y1 + child_height;
      lastweek_app_group.allocate (child_box, flags);
    }

    public ApplicationView ()
    {
      recent_app_group = new ApplicationGroup ("Recent apps");
      yesterday_app_group = new ApplicationGroup ("Yesterday apps");
      lastweek_app_group = new ApplicationGroup ("Last week apps");

      this.add_actor (recent_app_group);
      this.add_actor (yesterday_app_group);
      this.add_actor (lastweek_app_group);
      this.show_all ();
    }

    construct
    {
    }
  }
}
