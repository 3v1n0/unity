#ifndef LAUNCHERMODEL_H
#define LAUNCHERMODEL_H

#include "LauncherIcon.h"

#include <sigc++/sigc++.h>

class LauncherModel : public sigc::trackable
{

public:
    typedef std::list<LauncherIcon*> Base;
    typedef Base::iterator iterator; 
    typedef Base::reverse_iterator reverse_iterator; 
    typedef bool (*SortFunc) (LauncherIcon *first, LauncherIcon *second);
    
    LauncherModel();
    ~LauncherModel();

    void AddIcon (LauncherIcon *icon);
    void RemoveIcon (LauncherIcon *icon);
    void Sort (SortFunc func);
    int  Size ();

    void OnIconRemove (void *icon);
    
    iterator begin ();
    iterator end ();
    reverse_iterator rbegin ();
    reverse_iterator rend ();
    
    sigc::signal<void, void *> icon_added;
    sigc::signal<void, void *> icon_removed;
    sigc::signal<void> order_changed;
    
private:
    Base _inner;
    
    static gboolean RemoveCallback (gpointer data);
};

#endif // LAUNCHERMODEL_H
