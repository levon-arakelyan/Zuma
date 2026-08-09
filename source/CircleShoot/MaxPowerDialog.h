#ifndef __MAXPOWERDIALOG_H__
#define __MAXPOWERDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    // Template for future mode picker dialogs (Modes catalog + DialogType_*):
    // 1. Add ModeId + kModes[] row with opensPicker
    // 2. Add DialogType_* + DoX / FinishX / CheckYesNo cases
    // 3. Append new CircleShootApp fields at the END of existing mode members
    //    (do not insert mid-class — incremental MSVC builds can desync offsets)
    // 4. Picker lifetime = MovingHole style:
    //    AddedToManager → AddWidget; RemovedFromManager → RemoveWidget; delete in ~
    // 5. One listener interface max; Apply-only checkboxes use NULL listener
    ///////////////////////////////////////////////////////////////////////////////
    class MaxPowerDialog : public CircleDialog, SliderListener
    {
    public:
        MaxPowerDialog(int initialPercent, bool quietSounds);
        virtual ~MaxPowerDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        int GetPercent() const { return mPercent; }
        bool GetQuietSounds() const;

        Slider *mPercentSlider;
        Checkbox *mQuietSoundsCheckbox;
        int mPercent;

    private:
        void SetPercentFromSlider(double theVal);
        static int SnapPercent(float theValue);
        static double PercentToSlider(int thePercent);
    };
};

#endif
