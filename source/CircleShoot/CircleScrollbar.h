#ifndef __CIRCLESCROLLBAR_H__
#define __CIRCLESCROLLBAR_H__

#include <SexyAppFramework/ScrollbarWidget.h>
#include <SexyAppFramework/ScrollbuttonWidget.h>

namespace Sexy
{
    ///////////////////////////////////////////////////////////////////////////////
    // Scrollbar drawn in the game's dialog colors (green / yellow / purple)
    // instead of the framework's stock Windows greys. Behaviour is unchanged.
    ///////////////////////////////////////////////////////////////////////////////
    class CircleScrollbutton : public ScrollbuttonWidget
    {
    public:
        CircleScrollbutton(int theId, ButtonListener *theButtonListener);
        virtual void Draw(Graphics *g);
    };

    class CircleScrollbar : public ScrollbarWidget
    {
    public:
        CircleScrollbar(int theId, ScrollListener *theScrollListener);

        virtual void Draw(Graphics *g);
        virtual void DrawThumb(Graphics *g, int theX, int theY, int theWidth, int theHeight);
    };
}

#endif
