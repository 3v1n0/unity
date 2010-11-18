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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <sys/time.h>

#include <Nux/View.h>
#include <Nux/BaseWindow.h>
#include "Introspectable.h"
#include "LauncherIcon.h"
#include "NuxGraphics/IOpenGLAsmShader.h"
#include "Nux/TimerProc.h"

class LauncherModel;

class Launcher : public Introspectable, public nux::View
{
public:
    Launcher(nux::BaseWindow *parent, NUX_FILE_LINE_PROTO);
    ~Launcher();

    virtual long ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    LauncherIcon* GetActiveTooltipIcon() {return m_ActiveTooltipIcon;}
    LauncherIcon* GetActiveMenuIcon() {return m_ActiveMenuIcon;}

    bool TooltipNotify(LauncherIcon* Icon);
    bool MenuNotify(LauncherIcon* Icon);
    
    void SetIconSize(int tile_size, int icon_size);
    void NotifyMenuTermination(LauncherIcon* Icon);
    
    void SetModel (LauncherModel *model);
    
    void SetFloating (bool floating);
    
    void SetAutohide (bool autohide, nux::View *show_trigger);
    bool AutohideEnabled ();
    
    virtual void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags);

protected:
	const gchar* 
	getName ();

	void
	addProperties (GVariantBuilder *builder);

private:
  typedef enum
  {
    LAUNCHER_FOLDED,
    LAUNCHER_UNFOLDED
  } LauncherState;

  typedef enum
  {
    ACTION_NONE,
    ACTION_DRAG_LAUNCHER,
    ACTION_DRAG_ICON,
  } LauncherActionState;
  
  typedef struct
  {
    LauncherIcon *icon;
    nux::Point3   center;
    float         folding_rads;
    float         alpha;
    float         backlight_intensity;
    float         glow_intensity;
    bool          running_arrow;
    bool          active_arrow;
    bool          skip;
    int           window_indicators;
  } RenderArg;
  
  static gboolean AnimationTimeout (gpointer data);
  static gboolean OnAutohideTimeout (gpointer data);
  static gboolean StrutHack (gpointer data);
  
  void OnTriggerMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnTriggerMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
  
  bool IconNeedsAnimation  (LauncherIcon *icon, struct timespec current);
  bool AnimationInProgress ();
  void SetTimeStruct       (struct timespec *timer, struct timespec *sister = 0, int sister_relation = 0);
  
  void EnsureAnimation    ();
  void SetupAutohideTimer ();
  
  float DnDExitProgress  ();
  float GetHoverProgress ();
  float AutohideProgress ();
  float IconPresentProgress (LauncherIcon *icon, struct timespec current);

  void SetHover   ();
  void UnsetHover ();
  void SetHidden  (bool hidden);
  
  void SetDndDelta (float x, float y, nux::Geometry geo, struct timespec current);
  void RenderArgs (std::list<Launcher::RenderArg> &launcher_args, 
                   std::list<Launcher::RenderArg> &shelf_args, 
                   nux::Geometry &box_geo, nux::Geometry &shelf_geo);

  void DrawRenderArg (nux::GraphicsEngine& GfxContext, RenderArg arg);

  void OnIconAdded    (void *icon_pointer);
  void OnIconRemoved  (void *icon_pointer);
  void OnOrderChanged ();

  void OnIconNeedsRedraw (void *icon);

  void RenderIcon (nux::GraphicsEngine& GfxContext, RenderArg arg, nux::BaseTexture *text, nux::Color bkg_color, float alpha);
  void RenderIconImage(nux::GraphicsEngine& GfxContext, RenderArg arg);
  void UpdateIconXForm (std::list<Launcher::RenderArg> args);
  LauncherIcon* MouseIconIntersection (int x, int y);
  void EventLogic ();
  void MouseDownLogic (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void MouseUpLogic (int x, int y, unsigned long button_flags, unsigned long key_flags);

  virtual void PreLayoutManagement();
  virtual long PostLayoutManagement(long LayoutResult);
  virtual void PositionChildLayout(float offsetX, float offsetY);

  nux::HLayout* m_Layout;
  int m_ContentOffsetY;

  LauncherIcon* m_ActiveTooltipIcon;
  LauncherIcon* m_ActiveMenuIcon;

  bool  _hovered;
  bool  _floating;
  bool  _autohide;
  bool  _hidden;

  float _folded_angle;
  float _neg_folded_angle;
  float _folded_z_distance;
  float _launcher_top_y;
  float _launcher_bottom_y;

  LauncherState _launcher_state;
  LauncherActionState _launcher_action_state;
  LauncherIcon* _icon_under_mouse;
  LauncherIcon* _icon_mouse_down;

  int _space_between_icons;
  int _icon_size;
  int _icon_image_size;
  int _icon_image_size_delta;
  int _dnd_delta;
  int _dnd_security;
  int _enter_y;

  nux::BaseTexture* _icon_bkg_texture;
  nux::BaseTexture* _icon_shine_texture;
  nux::BaseTexture* _icon_outline_texture;
  nux::BaseTexture* _icon_2indicator;
  nux::BaseTexture* _icon_3indicator;
  nux::BaseTexture* _icon_4indicator;

  guint _anim_handle;
  guint _autohide_handle;

  nux::Matrix4  _view_matrix;
  nux::Matrix4  _projection_matrix;
  nux::Point2   _mouse_position;
  nux::IntrusiveSP<nux::IOpenGLShaderProgram>    _shader_program_uv_persp_correction;
  nux::IntrusiveSP<nux::IOpenGLAsmShaderProgram> _AsmShaderProg;
  nux::BaseTexture* m_RunningIndicator;
  nux::BaseTexture* m_ActiveIndicator;
  nux::AbstractPaintLayer* m_BackgroundLayer;
  nux::BaseWindow* _parent;
  nux::View* _autohide_trigger;
  nux::Geometry _last_shelf_area;
  LauncherModel* _model;
  
  /* event times */
  struct timespec _enter_time;
  struct timespec _exit_time;
  struct timespec _drag_end_time;
  struct timespec _autohide_time;
};

#endif // LAUNCHER_H


