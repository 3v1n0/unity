#ifndef PLUGINADAPTER_H
#define PLUGINADAPTER_H

/* Compiz */
#include <core/core.h>

#include <sigc++/sigc++.h>

class PluginAdapter : public sigc::trackable
{
public:
    static PluginAdapter * Default ();

    static void Initialize (CompScreen *screen);

    ~PluginAdapter();
    
    std::string * MatchStringForXids (std::list<Window> *windows);
    
    void SetScaleAction (CompAction *scale);
    
    void SetExpoAction (CompAction *expo);
    
    void InitiateScale (std::string *match);
    
    void InitiateExpo ();
    
protected:
    PluginAdapter(CompScreen *screen);

private:
    CompScreen *m_Screen;
    CompAction *m_ExpoAction;
    CompAction *m_ScaleAction;
    
    static PluginAdapter *_default;
};

#endif
