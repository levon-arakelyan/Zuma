#ifndef __COLORSHIFTDIALOG_H__
#define __COLORSHIFTDIALOG_H__

#include "CircleDialog.h"
#include "CircleCommon.h"
#include <SexyAppFramework/SliderListener.h>
#include <SexyAppFramework/CheckboxListener.h>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;
    class Checkbox;

    ///////////////////////////////////////////////////////////////////////////////
    // Color shift picker: interval, per-color enable + destination, optional random.
    ///////////////////////////////////////////////////////////////////////////////
    class ColorShiftDialog : public CircleDialog, SliderListener, CheckboxListener
    {
    public:
        ColorShiftDialog(float initialSec,
                         const int initialMap[MAX_BALL_COLORS],
                         const bool initialEnabled[MAX_BALL_COLORS],
                         bool randomMap);
        virtual ~ColorShiftDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void CheckboxChecked(int theId, bool checked);
        virtual void Draw(Graphics *g);

        float GetIntervalSec() const { return mIntervalSec; }
        float GetHz() const { return mIntervalSec; }
        bool GetRandomMap() const;
        void GetColorMap(int outMap[MAX_BALL_COLORS]) const;
        void GetEnabledColors(bool outEnabled[MAX_BALL_COLORS]) const;

        void OnTargetPicked(int theSrcColor, int theDestColor, bool apply);

        Slider *mSecSlider;
        Checkbox *mRandomCheckbox;
        Checkbox *mColorCheckboxes[MAX_BALL_COLORS];
        int mMap[MAX_BALL_COLORS];
        bool mEnabled[MAX_BALL_COLORS];
        float mIntervalSec;
        int mPendingTargetSrc;

    private:
        void SetSecFromSlider(double theVal);
        void UpdateColorCheckboxesForRandom();
        static float SnapSec(float theValue);
        static double SecToSlider(float theSec);
    };
};

#endif
