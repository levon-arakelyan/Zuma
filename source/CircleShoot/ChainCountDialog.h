#ifndef __CHAINCOUNTDIALOG_H__
#define __CHAINCOUNTDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    // Chain count picker: hits needed before chain bonus (1-20), optional disable.
    // Same lifetime pattern as MaxPowerDialog.
    ///////////////////////////////////////////////////////////////////////////////
    class ChainCountDialog : public CircleDialog, SliderListener
    {
    public:
        ChainCountDialog(int initialThreshold, bool disableBonus);
        virtual ~ChainCountDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetThreshold() const { return mThreshold; }
        bool GetDisableBonus() const;

        Slider *mThresholdSlider;
        Checkbox *mDisableBonusCheckbox;
        int mThreshold;

    private:
        void SetThresholdFromSlider(double theVal);
        static int SnapThreshold(float theValue);
        static double ThresholdToSlider(int theThreshold);
    };
};

#endif
