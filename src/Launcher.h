#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <Nux/View.h>
#include <Nux/BaseWindow.h>
#include "LauncherIcon.h"
#include "NuxGraphics/IOpenGLAsmShader.h"
#include "Nux/TimerProc.h"

class LauncherModel;

class Launcher : public nux::View
{
public:
    Launcher(NUX_FILE_LINE_PROTO);
    ~Launcher();

    virtual long ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    LauncherIcon* GetActiveTooltipIcon() {return m_ActiveTooltipIcon;}
    LauncherIcon* GetActiveMenuIcon() {return m_ActiveMenuIcon;}

    bool TooltipNotify(LauncherIcon* Icon);
    bool MenuNotify(LauncherIcon* Icon);
    
    void SetIconSize(int tile_size, int icon_size, nux::BaseWindow *parent);
    void NotifyMenuTermination(LauncherIcon* Icon);
    
    void SetModel (LauncherModel *model);
    
    virtual void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    virtual void RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags);

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

  void OnIconAdded (void *icon_pointer);
  void OnIconRemoved (void *icon_pointer);
  void OnOrderChanged ();


  void OnIconNeedsRedraw (void *icon);

  void FoldingCallback(void* v);
  void RevealCallback(void* v);

  void RenderIcon (nux::GraphicsEngine& GfxContext, LauncherIcon* launcher_view);
  void RenderIconImage(nux::GraphicsEngine& GfxContext, LauncherIcon* launcher_view);
  void UpdateIconXForm ();
  LauncherIcon* MouseIconIntersection (int x, int y);
  void EventLogic ();
  void MouseDownLogic ();
  void MouseUpLogic ();

  void SlideDown(float stepy, int mousedy);
  void SlideUp(float stepy, int mousedy);

  virtual void PreLayoutManagement();
  virtual long PostLayoutManagement(long LayoutResult);
  virtual void PositionChildLayout(float offsetX, float offsetY);

  void OrderRevealedIcons();
  void OrderFoldedIcons(int FocusIconIndex);
  void ScheduleRevealAnimation ();
  void ScheduleFoldAnimation ();

  nux::HLayout* m_Layout;
  int m_ContentOffsetY;

  LauncherIcon* m_ActiveTooltipIcon;
  LauncherIcon* m_ActiveMenuIcon;

  float _folding_angle;
  float _angle_rate;
  float _timer_intervals;
  int   _space_between_icons;
  int   _anim_duration;
  float _folded_angle;
  float _neg_folded_angle;
  float _folded_z_distance;
  float _launcher_top_y;
  float _launcher_bottom_y;
  LauncherState _launcher_state;
  LauncherActionState _launcher_action_state;
  LauncherIcon* _icon_under_mouse;
  LauncherIcon* _icon_mouse_down;
  int _icon_size;
  int _icon_image_size;
  int _icon_image_size_delta;
  nux::BaseTexture* _icon_bkg_texture;
  nux::BaseTexture* _icon_outline_texture;
  int _dnd_delta;
  int _dnd_security;

  nux::TimerFunctor* _folding_functor;
  nux::TimerHandle _folding_timer_handle;
  nux::TimerFunctor* _reveal_functor;
  nux::TimerHandle _reveal_timer_handle;

  nux::Matrix4  _view_matrix;
  nux::Matrix4  _projection_matrix;
  nux::Point2   _mouse_position;
  nux::IntrusiveSP<nux::IOpenGLShaderProgram>    _shader_program_uv_persp_correction;
  nux::IntrusiveSP<nux::IOpenGLAsmShaderProgram> _AsmShaderProg;
  nux::BaseTexture* m_RunningIndicator;
  nux::BaseTexture* m_ActiveIndicator;
  nux::AbstractPaintLayer* m_BackgroundLayer;
  LauncherModel* _model;
};

#endif // LAUNCHER_H


