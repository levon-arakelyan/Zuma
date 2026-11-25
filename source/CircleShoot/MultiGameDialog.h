#ifndef __MULTIGAMEDIALOG_H__
#define __MULTIGAMEDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    class MultiGameDialog : public CircleDialog, SliderListener
    {
    public:
        MultiGameDialog(int initialCount);
        virtual ~MultiGameDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetGameCount() const { return mGameCount; }

        Slider *mCountSlider;
        int mGameCount;

    private:
        void SetCountFromSlider(double theVal);
        static int SnapCount(float theValue);
        static double CountToSlider(int theCount);
    };
};

#endif
