#ifndef __INVISIBLEDIALOG_H__
#define __INVISIBLEDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>
#include <string>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    // Invisible mode picker: duration, wave interval, and % of chain balls hidden.
    ///////////////////////////////////////////////////////////////////////////////
    class InvisibleDialog : public CircleDialog, SliderListener
    {
    public:
        InvisibleDialog(float initialDurationSec, float initialIntervalSec, int initialPercent);
        virtual ~InvisibleDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        float GetDurationSec() const { return mDurationSec; }
        float GetIntervalSec() const { return mIntervalSec; }
        int GetPercent() const { return mPercent; }

        Slider *mDurationSlider;
        Slider *mIntervalSlider;
        Slider *mPercentSlider;
        float mDurationSec;
        float mIntervalSec;
        int mPercent;

    private:
        void SetDurationFromSlider(double theVal);
        void SetIntervalFromSlider(double theVal);
        void SetPercentFromSlider(double theVal);
        static float SnapSec(float theValue);
        static double SecToSlider(float theSec);
        static int SnapPercent(float theValue);
        static double PercentToSlider(int thePercent);
        static std::string FormatSecLabel(const char *thePrefix, float theSec);
    };
};

#endif
