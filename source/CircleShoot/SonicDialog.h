#ifndef __SONICDIALOG_H__
#define __SONICDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class SonicDialog : public CircleDialog, SliderListener
    {
    public:
        SonicDialog(float initialMultiplier);
        virtual ~SonicDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        float GetChainSpeedMultiplier() const { return mMultiplier; }

        Slider *mSpeedSlider;
        float mMultiplier;

    private:
        void SetMultiplierFromSlider(double theVal);
        static float SnapMultiplier(float theValue);
        static double MultiplierToSlider(float theMultiplier);
    };
};

#endif
