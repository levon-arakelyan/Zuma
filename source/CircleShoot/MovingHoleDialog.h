#ifndef __MOVINGHOLEDIALOG_H__
#define __MOVINGHOLEDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class MovingHoleDialog : public CircleDialog, SliderListener
    {
    public:
        MovingHoleDialog(int initialSpeed);
        virtual ~MovingHoleDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetSpeed() const { return mSpeed; }

        Slider *mSpeedSlider;
        int mSpeed;

    private:
        void SetSpeedFromSlider(double theVal);
        static int SnapSpeed(float theValue);
        static double SpeedToSlider(int theSpeed);
        static const char *SpeedLabel(int theSpeed);
    };
};

#endif
