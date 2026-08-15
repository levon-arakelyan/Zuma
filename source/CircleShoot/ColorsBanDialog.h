#ifndef __COLORSBANDIALOG_H__
#define __COLORSBANDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/CheckboxListener.h>
#include <SexyAppFramework/SliderListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Checkbox;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////
    class ColorsBanDialog : public CircleDialog, CheckboxListener, SliderListener
    {
    public:
        ColorsBanDialog(const bool initialBanned[MAX_BALL_COLORS], bool random, int randomCount);
        virtual ~ColorsBanDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void CheckboxChecked(int theId, bool checked);
        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        void GetBannedColors(bool outBanned[MAX_BALL_COLORS]) const;
        bool GetRandom() const;
        int GetRandomCount() const { return mRandomCount; }

        Checkbox *mColorCheckboxes[MAX_BALL_COLORS];
        Checkbox *mRandomCheckbox;
        Slider *mCountSlider;
        int mRandomCount;

    private:
        int CountUncheckedColors() const;
        void UpdateColorCheckboxesForRandom();
        void SetCountFromSlider(double theVal);
        static int SnapCount(float theValue);
        static double CountToSlider(int theCount);
    };
};

#endif
