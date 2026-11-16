#ifndef __LEVELSWAPDIALOG_H__
#define __LEVELSWAPDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    // How often Level swap changes the board: 10-60 seconds, step 1.
    ///////////////////////////////////////////////////////////////////////////////
    class LevelSwapDialog : public CircleDialog, SliderListener
    {
    public:
        LevelSwapDialog(int initialSec);
        virtual ~LevelSwapDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetIntervalSec() const { return mIntervalSec; }

        Slider *mIntervalSlider;
        int mIntervalSec;

    private:
        void SetIntervalFromSlider(double theVal);
        static int SnapInterval(float theValue);
        static double IntervalToSlider(int theSec);
    };
};

#endif
