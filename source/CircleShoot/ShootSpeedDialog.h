#ifndef __SHOOTSPEEDDIALOG_H__
#define __SHOOTSPEEDDIALOG_H__

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
    // Shoot speed picker: 0.1x-5x multiplier, or instant arrival (exclusive).
    ///////////////////////////////////////////////////////////////////////////////
    class ShootSpeedDialog : public CircleDialog, SliderListener, CheckboxListener
    {
    public:
        ShootSpeedDialog(float initialMultiplier, bool instant);
        virtual ~ShootSpeedDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        float GetMultiplier() const { return mMultiplier; }
        bool GetInstant() const;

        Slider *mSpeedSlider;
        Checkbox *mInstantCheckbox;
        float mMultiplier;

    private:
        void SetMultiplierFromSlider(double theVal);
        void UpdateExclusiveState(bool fromSlider);
        static int NearestMultiplierIndex(float theValue);
        static double MultiplierToSlider(float theMultiplier);
        static float SnapMultiplier(float theValue);
    };
};

#endif
