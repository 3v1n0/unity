/*
* Copyright (C) 2010 Canonical Ltd
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
* Authored by: Jay Taoko <jay.taoko@canonical.com>
* Authored by: Mirco Müller <mirco.mueller@canonical.com
*/

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/WindowThread.h"
#include "Nux/WindowCompositor.h"
#include "Nux/BaseWindow.h"
#include "Nux/Button.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/TextureArea.h"
#include "NuxImage/CairoGraphics.h"

#include "QuicklistView.h"
#include "QuicklistMenuItem.h"


NUX_IMPLEMENT_OBJECT_TYPE (QuicklistView);

QuicklistView::QuicklistView ()
{
  _texture_bg = 0;
  _texture_mask = 0;
  _texture_outline = 0;
  _cairo_text_has_changed = true;

  _anchorX   = 0;
  _anchorY   = 0;
  _labelText = TEXT ("QuicklistView 1234567890");

  _anchor_width   = 10;
  _anchor_height  = 18;
  _corner_radius  = 4;
  _padding        = 13;
  _top_size       = 4;

  _hlayout         = new nux::HLayout (TEXT(""), NUX_TRACKER_LOCATION);
  _vlayout         = new nux::VLayout (TEXT(""), NUX_TRACKER_LOCATION);
  _item_layout     = new nux::VLayout (TEXT(""), NUX_TRACKER_LOCATION);
  _default_item_layout = new nux::VLayout (TEXT(""), NUX_TRACKER_LOCATION);
  
  _left_space = new nux::SpaceLayout (_padding + _anchor_width + _corner_radius, _padding + _anchor_width + _corner_radius, 1, 1000);
  _right_space = new nux::SpaceLayout (_padding + _corner_radius, _padding + _corner_radius, 1, 1000);

  _top_space = new nux::SpaceLayout (1, 1000, _padding + _corner_radius, _padding + _corner_radius);
  _bottom_space = new nux::SpaceLayout (1, 1000, _padding + _corner_radius, _padding + _corner_radius);

  _vlayout->AddLayout (_top_space, 0);

  _vlayout->AddLayout (_item_layout, 0);
  
  _vlayout->AddLayout (_default_item_layout, 0);
  
  for (int i = 0; i < 2; i++)
  {
    nux::StaticCairoText* item_text;
    item_text = new nux::StaticCairoText (TEXT ("Default Item"), NUX_TRACKER_LOCATION);

    item_text->sigTextChanged.connect (sigc::mem_fun (this, &QuicklistView::RecvCairoTextChanged));
    item_text->sigTextColorChanged.connect (sigc::mem_fun (this, &QuicklistView::RecvCairoTextColorChanged));
    _default_item_layout->AddView(item_text, 1, nux::eCenter, nux::eFull);
    _default_item_list.push_back (item_text);
    item_text->Reference();
  }

  _vlayout->AddLayout (_bottom_space, 0);

  _hlayout->AddLayout (_left_space, 0);
  _hlayout->AddLayout (_vlayout, 1, nux::eCenter, nux::eFull);
  _hlayout->AddLayout (_right_space, 0);

  SetWindowSizeMatchLayout (true);
  SetLayout (_hlayout);
  
  OnMouseDownOutsideArea.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseDownOutsideOfQuicklist));
  OnMouseDown.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseDown));
  OnMouseUp.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseUp));
  OnMouseClick.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseClick));
  OnMouseMove.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseMove));
  OnMouseDrag.connect (sigc::mem_fun (this, &QuicklistView::RecvMouseDrag));
  
}

QuicklistView::~QuicklistView ()
{
  if (_texture_bg)
    _texture_bg->UnReference ();

  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    (*it)->UnReference();
  }
  _item_list.clear ();
}

void QuicklistView::ShowQuicklistWithTipAt (int anchor_tip_x, int anchor_tip_y)
{
  int window_width;
  int window_height;
  
  window_width = nux::GetWindow ().GetWindowWidth ();
  window_height = nux::GetWindow ().GetWindowHeight ();
  
  _anchorX = anchor_tip_x;
  _anchorY = anchor_tip_y;
  
  int x = _anchorX - _padding;
  int y = anchor_tip_y - _anchor_height/2 - _top_size - _corner_radius - _padding;
  
  SetBaseX (x);
  SetBaseY (y);
  
  ShowWindow (true);
}

void QuicklistView::ShowWindow (bool b, bool start_modal)
{
  BaseWindow::ShowWindow (b, start_modal);
  
  // Reset all colors to white
  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    (*it)->SetTextColor (nux::Color::White);
  }
}

long QuicklistView::ProcessEvent (nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;
  long ProcEvInfo = 0;

  nux::IEvent window_event = ievent;
  nux::Geometry base = GetGeometry();
  window_event.e_x_root = base.x;
  window_event.e_y_root = base.y;

  // The child layout get the Mouse down button only if the MouseDown happened inside the client view Area
  nux::Geometry viewGeometry = GetGeometry();

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!viewGeometry.IsPointInside (ievent.e_x - ievent.e_x_root, ievent.e_y - ievent.e_y_root) )
    {
      ProcEvInfo = nux::eDoNotProcess;
    }
  }

  // We choose to test the quicklist items ourselves instead of processing them as it is usual in nux.
  // This is meantto be easier since the quicklist has a atypical way of working.
  // if (m_layout)
  // ret = m_layout->ProcessEvent (window_event, ret, ProcEvInfo);

  // PostProcessEvent2 must always have its last parameter set to 0
  // because the m_BackgroundArea is the real physical limit of the window.
  // So the previous test about IsPointInside do not prevail over m_BackgroundArea
  // testing the event by itself.
  ret = PostProcessEvent2 (ievent, ret, 0);
  return ret;    
}

void QuicklistView::Draw (nux::GraphicsEngine& gfxContext, bool forceDraw)
{
  nux::Geometry base = GetGeometry();

  // the elements position inside the window are referenced to top-left window
  // corner. So bring base to (0, 0).
  base.SetX (0);
  base.SetY (0);
  gfxContext.PushClippingRectangle (base);

  nux::GetGraphicsEngine().GetRenderStates().SetBlend (false, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  nux::TexCoordXForm texxform_bg;
  texxform_bg.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform_bg.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  nux::TexCoordXForm texxform_mask;
  texxform_mask.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform_mask.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);


  gfxContext.QRP_GLSL_2TexMod (base.x,
    base.y,
    base.width,
    base.height,
    _texture_bg->GetDeviceTexture(),
    texxform_bg,
    nux::Color(1.0f, 1.0f, 1.0f, 1.0f),
    _texture_mask->GetDeviceTexture(),
    texxform_mask,
    nux::Color(1.0f, 1.0f, 1.0f, 1.0f));


  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);

  nux::GetGraphicsEngine().GetRenderStates().SetBlend (true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  gfxContext.QRP_GLSL_1Tex (base.x,
    base.y,
    base.width,
    base.height,
    _texture_outline->GetDeviceTexture(),
    texxform,
    nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  nux::GetGraphicsEngine().GetRenderStates().SetBlend (false);

  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    (*it)->ProcessDraw(gfxContext, forceDraw);
  }

  for (it = _default_item_list.begin(); it != _default_item_list.end(); it++)
  {
    (*it)->ProcessDraw(gfxContext, forceDraw);
  }
  

  gfxContext.PopClippingRectangle ();
}

void QuicklistView::DrawContent (nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void QuicklistView::PreLayoutManagement ()
{
  int MaxItemWidth = 0;
  int TotalItemHeight = 0;
  
  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    int  textWidth  = 0;
    int  textHeight = 0;
    (*it)->GetTextExtents(textWidth, textHeight);
    if (textWidth > MaxItemWidth)
      MaxItemWidth = textWidth;
    TotalItemHeight += textHeight;
  }

  for (it = _default_item_list.begin(); it != _default_item_list.end(); it++)
  {
    int  textWidth  = 0;
    int  textHeight = 0;
    (*it)->GetTextExtents(textWidth, textHeight);
    if (textWidth > MaxItemWidth)
      MaxItemWidth = textWidth;
    TotalItemHeight += textHeight;
  }
  
  if(TotalItemHeight < _anchor_height)
  {
    _top_space->SetMinMaxSize(1, (_anchor_height - TotalItemHeight)/2 +1 + _padding + _corner_radius);
    _bottom_space->SetMinMaxSize(1, (_anchor_height - TotalItemHeight)/2 +1 + _padding + _corner_radius);
  }

  BaseWindow::PreLayoutManagement ();
}

long QuicklistView::PostLayoutManagement (long LayoutResult)
{
  long result = BaseWindow::PostLayoutManagement (LayoutResult);
  UpdateTexture ();

  int x = _padding + _anchor_width + _corner_radius;
  int y = _padding + _corner_radius;

  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    (*it)->SetBaseX (x);
    (*it)->SetBaseY (y);

    y += (*it)->GetBaseHeight ();
  }

  for (it = _default_item_list.begin(); it != _default_item_list.end(); it++)
  {
    (*it)->SetBaseX (x);
    (*it)->SetBaseY (y);

    y += (*it)->GetBaseHeight ();
  }
  
  return result;
}

void QuicklistView::RecvCairoTextChanged (nux::StaticCairoText& cairo_text)
{
  _cairo_text_has_changed = true;
}

void QuicklistView::RecvCairoTextColorChanged (nux::StaticCairoText& cairo_text)
{
  NeedRedraw ();
}

void QuicklistView::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
//     if (IsVisible ())
//     {
//       CaptureMouseDownAnyWhereElse (false);
//       ForceStopFocus (1, 1);
//       UnGrabPointer ();
//       EnableInputWindow (false);
//       ShowWindow (false);
//     }
}

void QuicklistView::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  
}

void QuicklistView::RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (IsVisible ())
  {
    CaptureMouseDownAnyWhereElse (false);
    ForceStopFocus (1, 1);
    UnGrabPointer ();
    EnableInputWindow (false);
    ShowWindow (false);
  }
}

void QuicklistView::RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    if ((*it)->GetGeometry ().IsPointInside (x, y))
    {
      (*it)->SetTextColor (nux::Color::DarkGray);
    }
    else
    {
      (*it)->SetTextColor (nux::Color::White);
    }
  }
  
  for (it = _default_item_list.begin(); it != _default_item_list.end(); it++)
  {
    if ((*it)->GetGeometry ().IsPointInside (x, y))
    {
      (*it)->SetTextColor (nux::Color::DarkGray);
    }
    else
    {
      (*it)->SetTextColor (nux::Color::White);
    }
  }  
}

void QuicklistView::RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  std::list<nux::StaticCairoText*>::iterator it;
  for (it = _item_list.begin(); it != _item_list.end(); it++)
  {
    if ((*it)->GetGeometry ().IsPointInside (x, y))
    {
      (*it)->SetTextColor (nux::Color::DarkGray);
    }
    else
    {
      (*it)->SetTextColor (nux::Color::White);
    }
  }
  
  for (it = _default_item_list.begin(); it != _default_item_list.end(); it++)
  {
    if ((*it)->GetGeometry ().IsPointInside (x, y))
    {
      (*it)->SetTextColor (nux::Color::DarkGray);
    }
    else
    {
      (*it)->SetTextColor (nux::Color::White);
    }
  }  
}
  
void QuicklistView::RecvMouseDownOutsideOfQuicklist (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  if (IsVisible ())
  {
    CaptureMouseDownAnyWhereElse (false);
    ForceStopFocus (1, 1);
    UnGrabPointer ();
    EnableInputWindow (false);
    ShowWindow (false);
  }
}

void QuicklistView::RemoveAllMenuItem ()
{
  _item_list.clear ();
  _item_layout->Clear ();
  _cairo_text_has_changed = true;
  nux::GetGraphicsThread ()->AddObjectToRefreshList (this);
}

void QuicklistView::AddMenuItem (nux::NString str)
{
  nux::StaticCairoText* item_text;
  item_text = new nux::StaticCairoText (str.GetTCharPtr (), NUX_TRACKER_LOCATION);

  item_text->sigTextChanged.connect (sigc::mem_fun (this, &QuicklistView::RecvCairoTextChanged));
  item_text->sigTextColorChanged.connect (sigc::mem_fun (this, &QuicklistView::RecvCairoTextColorChanged));
  _item_layout->AddView(item_text, 1, nux::eCenter, nux::eFull);
  _item_list.push_back (item_text);
  item_text->Reference();
  
  _cairo_text_has_changed = true;
  nux::GetGraphicsThread ()->AddObjectToRefreshList (this);
  NeedRedraw ();
}

void QuicklistView::RenderQuicklistView ()
{
  
}

int QuicklistView::GetNumItems ()
{
  return _item_list.size () + _default_item_list.size ();
}

QuicklistMenuItem* QuicklistView::GetNthItems (int index)
{
  return 0;
}

QuicklistMenuItem* QuicklistView::GetNthType  (int index)
{
  return 0;
}

std::list<QuicklistMenuItem*> QuicklistView::GetChildren ()
{
  std::list<QuicklistMenuItem*> l;
  return l;
}

  
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

static inline void ql_blurinner (guchar* pixel,
  gint   *zR,
  gint   *zG,
  gint   *zB,
  gint   *zA,
  gint    alpha,
  gint    aprec,
  gint    zprec)
{
  gint R;
  gint G;
  gint B;
  guchar A;

  R = *pixel;
  G = *(pixel + 1);
  B = *(pixel + 2);
  A = *(pixel + 3);

  *zR += (alpha * ((R << zprec) - *zR)) >> aprec;
  *zG += (alpha * ((G << zprec) - *zG)) >> aprec;
  *zB += (alpha * ((B << zprec) - *zB)) >> aprec;
  *zA += (alpha * ((A << zprec) - *zA)) >> aprec;

  *pixel       = *zR >> zprec;
  *(pixel + 1) = *zG >> zprec;
  *(pixel + 2) = *zB >> zprec;
  *(pixel + 3) = *zA >> zprec;
}

static inline void ql_blurrow (guchar* pixels,
  gint    width,
  gint    height,
  gint    channels,
  gint    line,
  gint    alpha,
  gint    aprec,
  gint    zprec)
{
  gint    zR;
  gint    zG;
  gint    zB;
  gint    zA;
  gint    index;
  guchar* scanline;

  scanline = &(pixels[line * width * channels]);

  zR = *scanline << zprec;
  zG = *(scanline + 1) << zprec;
  zB = *(scanline + 2) << zprec;
  zA = *(scanline + 3) << zprec;

  for (index = 0; index < width; index ++)
    ql_blurinner (&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
    zprec);

  for (index = width - 2; index >= 0; index--)
    ql_blurinner (&scanline[index * channels], &zR, &zG, &zB, &zA, alpha, aprec,
    zprec);
}

static inline void ql_blurcol (guchar* pixels,
  gint    width,
  gint    height,
  gint    channels,
  gint    x,
  gint    alpha,
  gint    aprec,
  gint    zprec)
{
  gint zR;
  gint zG;
  gint zB;
  gint zA;
  gint index;
  guchar* ptr;

  ptr = pixels;

  ptr += x * channels;

  zR = *((guchar*) ptr    ) << zprec;
  zG = *((guchar*) ptr + 1) << zprec;
  zB = *((guchar*) ptr + 2) << zprec;
  zA = *((guchar*) ptr + 3) << zprec;

  for (index = width; index < (height - 1) * width; index += width)
    ql_blurinner ((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
    aprec, zprec);

  for (index = (height - 2) * width; index >= 0; index -= width)
    ql_blurinner ((guchar*) &ptr[index * channels], &zR, &zG, &zB, &zA, alpha,
    aprec, zprec);
}

//
// pixels   image-data
// width    image-width
// height   image-height
// channels image-channels
//
// in-place blur of image 'img' with kernel of approximate radius 'radius'
//
// blurs with two sided exponential impulse response
//
// aprec = precision of alpha parameter in fixed-point format 0.aprec
//
// zprec = precision of state parameters zR,zG,zB and zA in fp format 8.zprec
//
void ql_expblur (guchar* pixels,
  gint    width,
  gint    height,
  gint    channels,
  gint    radius,
  gint    aprec,
  gint    zprec)
{
  gint alpha;
  gint row = 0;
  gint col = 0;

  if (radius < 1)
    return;

  // calculate the alpha such that 90% of 
  // the kernel is within the radius.
  // (Kernel extends to infinity)
  alpha = (gint) ((1 << aprec) * (1.0f - expf (-2.3f / (radius + 1.f))));

  for (; row < height; row++)
    ql_blurrow (pixels, width, height, channels, row, alpha, aprec, zprec);

  for(; col < width; col++)
    ql_blurcol (pixels, width, height, channels, col, alpha, aprec, zprec);

  return;
}

/**
* ctk_surface_blur:
* @surface: pointer to a cairo image-surface
* @radius: unsigned integer acting as the blur-radius to apply
*
* Applies an exponential blur on the passed surface executed on the CPU. Not as
* nice as a real gaussian blur, but much faster.
**/
void ql_surface_blur (cairo_surface_t* surface,
                guint            radius)
{
guchar*        pixels;
guint          width;
guint          height;
cairo_format_t format;

// before we mess with the surface execute any pending drawing
cairo_surface_flush (surface);

pixels = cairo_image_surface_get_data (surface);
width  = cairo_image_surface_get_width (surface);
height = cairo_image_surface_get_height (surface);
format = cairo_image_surface_get_format (surface);

switch (format)
{
  case CAIRO_FORMAT_ARGB32:
    ql_expblur (pixels, width, height, 4, radius, 16, 7);
  break;

  case CAIRO_FORMAT_RGB24:
    ql_expblur (pixels, width, height, 3, radius, 16, 7);
  break;

  case CAIRO_FORMAT_A8:
    ql_expblur (pixels, width, height, 1, radius, 16, 7);
  break;

  default :
    // do nothing
  break;
}

// inform cairo we altered the surfaces contents
cairo_surface_mark_dirty (surface);	
}

  
void ql_tint_dot_hl (cairo_t* cr,
  gint    width,
  gint    height,
  gfloat  hl_x,
  gfloat  hl_y,
  gfloat  hl_size,
  gfloat* rgba_tint,
  gfloat* rgba_hl,
  gfloat* rgba_dot)
{
  cairo_surface_t* dots_surf    = NULL;
  cairo_t*         dots_cr      = NULL;
  cairo_pattern_t* dots_pattern = NULL;
  cairo_pattern_t* hl_pattern   = NULL;

  // create context for dot-pattern
  dots_surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 4, 4);
  dots_cr = cairo_create (dots_surf);

  // clear normal context
  cairo_scale (cr, 1.0f, 1.0f);
  cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  // prepare drawing for normal context
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  // create path in normal context
  cairo_rectangle (cr, 0.0f, 0.0f, (gdouble) width, (gdouble) height);  

  // fill path of normal context with tint
  cairo_set_source_rgba (cr,
    rgba_tint[0],
    rgba_tint[1],
    rgba_tint[2],
    rgba_tint[3]);
  cairo_fill_preserve (cr);

  // create pattern in dot-context
  cairo_set_operator (dots_cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (dots_cr);
  cairo_scale (dots_cr, 1.0f, 1.0f);
  cairo_set_operator (dots_cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba (dots_cr,
    rgba_dot[0],
    rgba_dot[1],
    rgba_dot[2],
    rgba_dot[3]);
  cairo_rectangle (dots_cr, 0.0f, 0.0f, 1.0f, 1.0f);
  cairo_fill (dots_cr);
  cairo_rectangle (dots_cr, 2.0f, 2.0f, 1.0f, 1.0f);
  cairo_fill (dots_cr);
  dots_pattern = cairo_pattern_create_for_surface (dots_surf);

  // fill path of normal context with dot-pattern
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source (cr, dots_pattern);
  cairo_pattern_set_extend (dots_pattern, CAIRO_EXTEND_REPEAT);
  cairo_fill_preserve (cr);
  cairo_pattern_destroy (dots_pattern);
  cairo_surface_destroy (dots_surf);
  cairo_destroy (dots_cr);

  // draw highlight
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  hl_pattern = cairo_pattern_create_radial (hl_x,
    hl_y,
    0.0f,
    hl_x,
    hl_y,
    hl_size);
  cairo_pattern_add_color_stop_rgba (hl_pattern,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
    0.65f);
  cairo_pattern_add_color_stop_rgba (hl_pattern, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_source (cr, hl_pattern);
  cairo_fill (cr);
  cairo_pattern_destroy (hl_pattern);
}

void ql_setup (cairo_surface_t** surf,
  cairo_t**         cr,
  gboolean          outline,
  gint              width,
  gint              height,
  gboolean          negative)
{
//     // create context
//     if (outline)
//       *surf = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
//     else
//       *surf = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
//     *cr = cairo_create (*surf);

  // clear context
  cairo_scale (*cr, 1.0f, 1.0f);
  if (outline)
  {
    cairo_set_source_rgba (*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    cairo_set_operator (*cr, CAIRO_OPERATOR_CLEAR);
  }  
  else
  {
    cairo_set_operator (*cr, CAIRO_OPERATOR_OVER);
    if (negative)
      cairo_set_source_rgba (*cr, 0.0f, 0.0f, 0.0f, 0.0f);
    else
      cairo_set_source_rgba (*cr, 1.0f, 1.0f, 1.0f, 1.0f);
  }
  cairo_paint (*cr);
}

void ql_compute_full_mask_path (cairo_t* cr,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     width,
  gint     height,
  gint     upper_size,
  gfloat   radius,
  guint    pad)
{
  //     0  1        2  3
  //     +--+--------+--+
  //     |              |
  //     + 14           + 4
  //     |              |
  //     |              |
  //     |              |
  //     + 13           |
  //    /               |
  //   /                |
  //  + 12              |
  //   \                |
  //    \               |
  //  11 +              |
  //     |              |
  //     |              |
  //     |              |
  //  10 +              + 5
  //     |              |
  //     +--+--------+--+ 6
  //     9  8        7


  gfloat padding  = pad;
  int ZEROPOINT5 = 0.0f;

  gfloat HeightToAnchor = 0.0f;
  HeightToAnchor = ((gfloat) height - 2.0f * radius - anchor_height -2*padding) / 2.0f;
  if (HeightToAnchor < 0.0f)
  {
    g_warning ("Anchor-height and corner-radius a higher than whole texture!");
    return;
  }

  //gint dynamic_size = height - 2*radius - 2*padding - anchor_height;
  //gint upper_dynamic_size = upper_size;
  //gint lower_dynamic_size = dynamic_size - upper_dynamic_size;

  if(upper_size >= 0)
  {
    if(upper_size > height - 2.0f * radius - anchor_height -2 * padding)
    {
      //g_warning ("[_compute_full_mask_path] incorrect upper_size value");
      HeightToAnchor = 0;
    }
    else
    {
      HeightToAnchor = height - 2.0f * radius - anchor_height -2 * padding - upper_size;
    }
  }
  else
  {
    HeightToAnchor = (height - 2.0f * radius - anchor_height -2*padding) / 2.0f;
  }

  cairo_translate (cr, -0.5f, -0.5f);

  // create path
  cairo_move_to (cr, padding + anchor_width + radius + ZEROPOINT5, padding + ZEROPOINT5); // Point 1
  cairo_line_to (cr, width - padding - radius, padding + ZEROPOINT5);   // Point 2
  cairo_arc (cr,
    width  - padding - radius + ZEROPOINT5,
    padding + radius + ZEROPOINT5,
    radius,
    -90.0f * G_PI / 180.0f,
    0.0f * G_PI / 180.0f);   // Point 4
  cairo_line_to (cr,
    (gdouble) width - padding + ZEROPOINT5,
    (gdouble) height - radius - padding + ZEROPOINT5); // Point 5
  cairo_arc (cr,
    (gdouble) width - padding - radius + ZEROPOINT5,
    (gdouble) height - padding - radius + ZEROPOINT5,
    radius,
    0.0f * G_PI / 180.0f,
    90.0f * G_PI / 180.0f);  // Point 7
  cairo_line_to (cr,
    anchor_width + padding + radius + ZEROPOINT5,
    (gdouble) height - padding + ZEROPOINT5); // Point 8

  cairo_arc (cr,
    anchor_width + padding + radius + ZEROPOINT5,
    (gdouble) height - padding - radius,
    radius,
    90.0f * G_PI / 180.0f,
    180.0f * G_PI / 180.0f); // Point 10

  cairo_line_to (cr,
    padding + anchor_width + ZEROPOINT5,
    (gdouble) height - padding - radius - HeightToAnchor + ZEROPOINT5 );  // Point 11
  cairo_line_to (cr,
    padding + ZEROPOINT5,
    (gdouble) height - padding - radius - HeightToAnchor - anchor_height / 2.0f + ZEROPOINT5); // Point 12
  cairo_line_to (cr,
    padding + anchor_width + ZEROPOINT5,
    (gdouble) height - padding - radius - HeightToAnchor - anchor_height + ZEROPOINT5);  // Point 13

  cairo_line_to (cr, padding + anchor_width + ZEROPOINT5, padding + radius  + ZEROPOINT5);  // Point 14
  cairo_arc (cr,
    padding + anchor_width + radius + ZEROPOINT5,
    padding + radius + ZEROPOINT5,
    radius,
    180.0f * G_PI / 180.0f,
    270.0f * G_PI / 180.0f);

  cairo_close_path (cr);
}

void ql_compute_mask (cairo_t* cr)
{
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_fill_preserve (cr);
}

void ql_compute_outline (cairo_t* cr,
  gfloat   line_width,
  gfloat*  rgba_line)
{
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba (cr,
    rgba_line[0],
    rgba_line[1],
    rgba_line[2],
    rgba_line[3]);
  cairo_set_line_width (cr, line_width);
  cairo_stroke (cr);
}

void ql_draw (cairo_t* cr,
  gboolean outline,
  gfloat   line_width,
  gfloat*  rgba,
  gboolean negative,
  gboolean stroke)
{
  // prepare drawing
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width (cr, line_width);
    cairo_set_source_rgba (cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke_preserve (cr);
  else
    cairo_fill_preserve (cr);
}

void ql_finalize (cairo_t** cr,
  gboolean  outline,
  gfloat    line_width,
  gfloat*   rgba,
  gboolean  negative,
  gboolean  stroke)
{
  // prepare drawing
  cairo_set_operator (*cr, CAIRO_OPERATOR_SOURCE);

  // actually draw the mask
  if (outline)
  {
    cairo_set_line_width (*cr, line_width);
    cairo_set_source_rgba (*cr, rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  else
  {
    if (negative)
      cairo_set_source_rgba (*cr, 1.0f, 1.0f, 1.0f, 1.0f);
    else
      cairo_set_source_rgba (*cr, 0.0f, 0.0f, 0.0f, 0.0f);
  }

  // stroke or fill?
  if (stroke)
    cairo_stroke (*cr);
  else
    cairo_fill (*cr);
}

void
  ql_compute_full_outline_shadow (
  cairo_t* cr,
  cairo_surface_t* surf,
  gint    width,
  gint    height,
  gfloat  anchor_width,
  gfloat  anchor_height,
  gint    upper_size,
  gfloat  corner_radius,
  guint   blur_coeff,
  gfloat* rgba_shadow,
  gfloat  line_width,
  gint    padding_size,
  gfloat* rgba_line)
{
  ql_setup (&surf, &cr, TRUE, width, height, FALSE);
  ql_compute_full_mask_path (cr,
    anchor_width,
    anchor_height,
    width,
    height,
    upper_size,
    corner_radius,
    padding_size);

  ql_draw (cr, TRUE, line_width, rgba_shadow, FALSE, FALSE);
  ql_surface_blur (surf, blur_coeff);
  ql_compute_mask (cr);
  ql_compute_outline (cr, line_width, rgba_line);
}

void ql_compute_full_mask (
  cairo_t* cr,
  cairo_surface_t* surf,
  gint     width,
  gint     height,
  gfloat   radius,
  guint    shadow_radius,
  gfloat   anchor_width,
  gfloat   anchor_height,
  gint     upper_size,
  gboolean negative,
  gboolean outline,
  gfloat   line_width,
  gint     padding_size,
  gfloat*  rgba)
{
  ql_setup (&surf, &cr, outline, width, height, negative);
  ql_compute_full_mask_path (cr,
    anchor_width,
    anchor_height,
    width,
    height,
    upper_size,
    radius,
    padding_size);
  ql_finalize (&cr, outline, line_width, rgba, negative, outline);
}

void QuicklistView::UpdateTexture ()
{
  if (_cairo_text_has_changed == false)
    return;

  int size_above_anchor = -1; // equal to sise below
  
  if ((_item_list.size () != 0) || (_default_item_list.size () != 0))
  {
    _top_size = 4;
    size_above_anchor = _top_size;
    int x = _anchorX - _padding;
    int y = _anchorY - _anchor_height/2 - _top_size - _corner_radius - _padding;

    SetBaseX (x);
    SetBaseY (y);
  }
  else
  {
    _top_size = 0;
    size_above_anchor = -1;
    int x = _anchorX - _padding;
    int y = _anchorY - _anchor_height/2 - _top_size - _corner_radius - _padding;

    SetBaseX (x);
    SetBaseY (y);    
  }
  
  float blur_coef         = 6.0f;

  nux::CairoGraphics* cairo_bg       = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, GetBaseWidth (), GetBaseHeight ());
  nux::CairoGraphics* cairo_mask     = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, GetBaseWidth (), GetBaseHeight ());
  nux::CairoGraphics* cairo_outline  = new nux::CairoGraphics (CAIRO_FORMAT_ARGB32, GetBaseWidth (), GetBaseHeight ());

  cairo_t *cr_bg      = cairo_bg->GetContext ();
  cairo_t *cr_mask    = cairo_mask->GetContext ();
  cairo_t *cr_outline = cairo_outline->GetContext ();

  float   tint_color[4]    = {0.0f, 0.0f, 0.0f, 0.80f};
  float   hl_color[4]      = {1.0f, 1.0f, 1.0f, 0.15f};
  float   dot_color[4]     = {1.0f, 1.0f, 1.0f, 0.20f};
  float   shadow_color[4]  = {0.0f, 0.0f, 0.0f, 1.00f};
  float   outline_color[4] = {1.0f, 1.0f, 1.0f, 0.75f};
  float   mask_color[4]    = {1.0f, 1.0f, 1.0f, 1.00f};
//   float   anchor_width      = 10;
//   float   anchor_height     = 18;

  ql_tint_dot_hl (cr_bg,
    GetBaseWidth (),
    GetBaseHeight (),
    GetBaseWidth () / 2.0f,
    0,
    nux::Max<float>(GetBaseWidth () / 1.3f, GetBaseHeight () / 1.3f),
    tint_color,
    hl_color,
    dot_color);

  ql_compute_full_outline_shadow
    (
    cr_outline,
    cairo_outline->GetSurface(),
    GetBaseWidth (),
    GetBaseHeight (),
    _anchor_width,
    _anchor_height,
    size_above_anchor,
    _corner_radius,
    blur_coef,
    shadow_color,
    1.0f,
    _padding,
    outline_color);

  ql_compute_full_mask (
    cr_mask,
    cairo_mask->GetSurface(),
    GetBaseWidth (),
    GetBaseHeight(),
    _corner_radius,  // radius,
    16,             // shadow_radius,
    _anchor_width,   // anchor_width,
    _anchor_height,  // anchor_height,
    size_above_anchor,             // upper_size,
    true,           // negative,
    false,          // outline,
    1.0,            // line_width,
    _padding,        // padding_size,
    mask_color);

  cairo_destroy (cr_bg);
  cairo_destroy (cr_outline);
  cairo_destroy (cr_mask);

  nux::NBitmapData* bitmap = cairo_bg->GetBitmap();

  if (_texture_bg)
    _texture_bg->UnReference ();
  _texture_bg = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _texture_bg->Update(bitmap);
  delete bitmap;

  bitmap = cairo_mask->GetBitmap();
  if (_texture_mask)
    _texture_mask->UnReference ();
  _texture_mask = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _texture_mask->Update(bitmap);
  delete bitmap;

  bitmap = cairo_outline->GetBitmap();
  if (_texture_outline)
    _texture_outline->UnReference ();
  _texture_outline = nux::GetThreadGLDeviceFactory()->CreateSystemCapableTexture ();
  _texture_outline->Update(bitmap);
  delete bitmap;

  delete cairo_bg;
  delete cairo_mask;
  delete cairo_outline;
  _cairo_text_has_changed = false;
  
  // Request a redraw, so this area will be added to Compiz list of dirty areas.
  NeedRedraw ();
}

void QuicklistView::PositionChildLayout (float offsetX, float offsetY)
{
}

void QuicklistView::LayoutWindowElements ()
{
}

void QuicklistView::NotifyConfigurationChange (int width, int height)
{
}

void QuicklistView::SetText (nux::NString text)
{
  if (_labelText == text)
    return;

  _labelText = text;
  UpdateTexture ();
}
