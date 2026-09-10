#ifndef __KILLERBALLDIALOG_H__
#define __KILLERBALLDIALOG_H__

#include "CircleDialog.h"
#include <SexyAppFramework/SliderListener.h>
#include <string>

namespace Sexy
{
    class WidgetManager;
    class Graphics;
    class Slider;

    ///////////////////////////////////////////////////////////////////////////////
    // Killer Ball mode picker: spawn interval and flight duration.
    ///////////////////////////////////////////////////////////////////////////////
    class KillerBallDialog : public CircleDialog, SliderListener
    {
    public:
        KillerBallDialog(float initialIntervalSec, float initialFlightSec);
        virtual ~KillerBallDialog();

        virtual void Resize(int theX, int theY, int theWidth, int theHeight);
        virtual int GetPreferredHeight(int theWidth);

        virtual void AddedToManager(WidgetManager *theWidgetManager);
        virtual void RemovedFromManager(WidgetManager *theWidgetManager);

        virtual void SliderVal(int theId, double theVal);
        virtual void Draw(Graphics *g);

        float GetIntervalSec() const { return mIntervalSec; }
        float GetFlightSec() const { return mFlightSec; }

        Slider *mIntervalSlider;
        Slider *mFlightSlider;
        float mIntervalSec;
        float mFlightSec;

    private:
        void SetIntervalFromSlider(double theVal);
        void SetFlightFromSlider(double theVal);
        static float SnapInterval(float theValue);
        static float SnapFlight(float theValue);
        static double IntervalToSlider(float theSec);
        static double FlightToSlider(float theSec);
        static std::string FormatSecLabel(const char *thePrefix, float theSec);
    };
};

#endif
