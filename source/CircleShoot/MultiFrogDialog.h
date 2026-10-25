#ifndef __MULTIFROGDIALOG_H__
#define __MULTIFROGDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    class MultiFrogDialog : public CircleDialog, SliderListener
    {
    public:
        MultiFrogDialog(int initialCount);
        virtual ~MultiFrogDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetFrogCount() const { return mFrogCount; }

        Slider *mCountSlider;
        int mFrogCount;

    private:
        void SetCountFromSlider(double theVal);
        static int SnapCount(float theValue);
        static double CountToSlider(int theCount);
    };
};

#endif
