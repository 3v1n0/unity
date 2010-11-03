#include "Nux/Nux.h"
#include "Nux/VScrollBar.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/MenuPage.h"
#include "Nux/WindowCompositor.h"
#include "Nux/BaseWindow.h"
#include "Nux/MenuPage.h"
#include "NuxCore/Color.h"

#include "LauncherIcon.h"
#include "Launcher.h"

#define DEFAULT_ICON "empathy"

LauncherIcon::LauncherIcon(Launcher* IconManager)
{
  _folding_angle = 0;
  m_IconManager = IconManager;
  m_TooltipText = "blank";
  _visible = true;
  _active = false;
  _running = false;
  _background_color = nux::Color::White;
  _mouse_inside = false;
  _tooltip = new nux::Tooltip ();
  _icon_type = LAUNCHER_ICON_TYPE_NONE;
  _sort_priority = 0;

  MouseEnter.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseEnter));
  MouseLeave.connect (sigc::mem_fun(this, &LauncherIcon::RecvMouseLeave));
}

LauncherIcon::~LauncherIcon()
{
}

nux::Color LauncherIcon::BackgroundColor ()
{
  return _background_color;
}

nux::BaseTexture * LauncherIcon::TextureForSize (int size)
{
  nux::BaseTexture * result = GetTextureForSize (size);
  
  return result;
}

nux::Color LauncherIcon::ColorForIcon (GdkPixbuf *pixbuf)
{
  unsigned int width = gdk_pixbuf_get_width (pixbuf);
  unsigned int height = gdk_pixbuf_get_height (pixbuf);
  unsigned int row_bytes = gdk_pixbuf_get_rowstride (pixbuf);
  
  long int rtotal = 0, gtotal = 0, btotal = 0;
  float total = 0.0f;
  
  
  guchar *img = gdk_pixbuf_get_pixels (pixbuf);
  
  for (unsigned int i = 0; i < width; i++)
  {
    for (unsigned int j = 0; j < height; j++)
    {
      guchar *pixels = img + ( j * row_bytes + i * 4);
      guchar r = *(pixels + 0);
      guchar g = *(pixels + 1);
      guchar b = *(pixels + 2);
      guchar a = *(pixels + 3);
      
      float saturation = (MAX (r, MAX (g, b)) - MIN (r, MIN (g, b))) / 255.0f;
      float relevance = .1 + .9 * (a / 255.0f) * saturation;
      
      rtotal += (guchar) (r * relevance);
      gtotal += (guchar) (g * relevance);
      btotal += (guchar) (b * relevance);
      
      total += relevance * 255;
    }
  }
  
  float r, g, b, h, s, v;
  r = rtotal / total;
  g = gtotal / total;
  b = btotal / total;
  
  nux::RGBtoHSV (r, g, b, h, s, v);
  
  if (s > .15f)
    s = MAX (s, 0.5f);
  v = .8f;
  
  nux::HSVtoRGB (r, g, b, h, s, v);
  
  return nux::Color (r, g, b);
}

nux::BaseTexture * LauncherIcon::TextureFromGtkTheme (const char *icon_name, int size)
{
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  nux::BaseTexture *result;
  
  theme = gtk_icon_theme_get_default ();
      
  if (!icon_name)
    icon_name = g_strdup (DEFAULT_ICON);
   
  info = gtk_icon_theme_lookup_icon (theme,
                                     icon_name,
                                     size,
                                     (GtkIconLookupFlags) 0);            
  if (!info)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
        
  if (gtk_icon_info_get_filename (info) == NULL)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       DEFAULT_ICON,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
  
  pbuf = gtk_icon_info_load_icon (info, NULL);
  result = nux::CreateTextureFromPixbuf (pbuf);
  
  _background_color = ColorForIcon (pbuf);
  
  g_object_unref (pbuf);
  
  return result;
}

void LauncherIcon::SetTooltipText(const TCHAR* text)
{
    m_TooltipText = text;
    _tooltip->SetText (m_TooltipText);
}

nux::NString LauncherIcon::GetTooltipText()
{
    return m_TooltipText;
}

void
LauncherIcon::RecvMouseEnter ()
{
  int icon_x = _xform_screen_coord[0].x;
  int icon_y = _xform_screen_coord[0].y;
  int icon_w = _xform_screen_coord[2].x - _xform_screen_coord[0].x;
  int icon_h = _xform_screen_coord[2].y - _xform_screen_coord[0].y;

  _tooltip->SetBaseX (icon_x + icon_w);
  _tooltip->SetBaseY (icon_y +
                      24 + // TODO: HARCODED, replace m_IconManager->GetBaseY ()
                      (icon_h / 2) -
                      (_tooltip->GetBaseHeight () / 2));
  _tooltip->ShowWindow (true);
}

void LauncherIcon::RecvMouseLeave ()
{
  _tooltip->ShowWindow (false);
}

void LauncherIcon::HideTooltip ()
{
  _tooltip->ShowWindow (false);
}

void
LauncherIcon::SetVisible (bool visible)
{
  if (visible == _visible)
    return;
      
  _visible = visible;
  
  if (visible)
    show.emit (this);
  else
    hide.emit (this);
}

void
LauncherIcon::SetActive (bool active)
{
  if (active == _active)
    return;
    
  _active = active;
  needs_redraw.emit (this);
}

void LauncherIcon::SetRunning (bool running)
{
  if (running == _running)
    return;
    
  _running = running;
  needs_redraw.emit (this);
}

void 
LauncherIcon::SetIconType (LauncherIconType type)
{
  _icon_type = type;
}

void 
LauncherIcon::SetSortPriority (int priority)
{
  _sort_priority = priority;
}

int 
LauncherIcon::SortPriority ()
{
  return _sort_priority;
}
    
LauncherIconType 
LauncherIcon::Type ()
{
  return _icon_type;
}

bool
LauncherIcon::Visible ()
{
  return _visible;
}

bool
LauncherIcon::Active ()
{
  return _active;
}

bool
LauncherIcon::Running ()
{
  return _running;
}
