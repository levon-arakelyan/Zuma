#ifndef __CHAINCOUNTDIALOG_H__
#define __CHAINCOUNTDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>
#include <SexyAppFramework/CheckboxListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    // Chain count picker: hits needed before chain bonus (1-20), optional disable.
    // Same lifetime pattern as other mode picker dialogs (MovingHole style).
    ///////////////////////////////////////////////////////////////////////////////
    // The disable checkbox and the slider are mutually exclusive: ticking the
    // checkbox disables the slider; moving the slider unticks the checkbox.
    class ChainCountDialog : public CircleDialog, SliderListener, CheckboxListener
    {
    public:
        ChainCountDialog(int initialThreshold, bool disableBonus);
        virtual ~ChainCountDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        int GetThreshold() const { return mThreshold; }
        bool GetDisableBonus() const;

        Slider *mThresholdSlider;
        Checkbox *mDisableBonusCheckbox;
        int mThreshold;

    private:
        void SetThresholdFromSlider(double theVal);
        void UpdateExclusiveState(bool fromSlider);
        static int SnapThreshold(float theValue);
        static double ThresholdToSlider(int theThreshold);
    };
};

#endif
