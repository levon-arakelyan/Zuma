#include "Zuma_Prefix.pch"

#include <SexyAppFramework/Graphics.h>

#include "CircleScrollbar.h"

using namespace Sexy;

namespace
{
    // Same palette as the Modes dialog list and tooltip.
    const Color kTrackColor(16, 36, 16);        // dark green
    const Color kTrackPressedColor(41, 73, 24); // green, while paging
    const Color kButtonColor(41, 73, 24);       // green
    const Color kYellow(213, 229, 32);          // dialog text yellow (0xD5E520)
    const Color kYellowDim(213, 229, 32, 90);   // disabled, like other dimmed dialog text
    const Color kPurple(173, 40, 198);          // selection purple
    const Color kPurpleLight(214, 120, 232);
    const Color kPurpleDark(110, 20, 128);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CircleScrollbutton::CircleScrollbutton(int theId, ButtonListener *theButtonListener)
    : ScrollbuttonWidget(theId, theButtonListener)
{
}

void CircleScrollbutton::Draw(Graphics *g)
{
    bool aPressed = mIsDown && mIsOver && !mDisabled;
    int anOffset = aPressed ? 1 : 0;

    g->SetColor(aPressed ? kPurple : kButtonColor);
    g->FillRect(0, 0, mWidth, mHeight);
    g->SetColor(kYellow);
    g->DrawRect(0, 0, mWidth - 1, mHeight - 1);

    // Arrow, same shape as the stock button (id 0 = up, 1 = down).
    g->SetColor(mDisabled ? kYellowDim : kYellow);
    for (int i = 0; i < 4; i++)
    {
        if (mId == 0)
            g->FillRect(mWidth / 2 - i - 1 + anOffset, i + (mHeight - 4) / 2 + anOffset, 1 + i * 2, 1);
        else
            g->FillRect(mWidth / 2 - i - 1 + anOffset, (3 - i) + (mHeight - 4) / 2 + anOffset, 1 + i * 2, 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
CircleScrollbar::CircleScrollbar(int theId, ScrollListener *theScrollListener)
    : ScrollbarWidget(theId, theScrollListener)
{
    // Swap the stock arrow buttons for styled ones (same ids and listener).
    RemoveWidget(mUpButton);
    delete mUpButton;
    mUpButton = new CircleScrollbutton(0, this);
    mUpButton->SetDisabled(true);
    AddWidget(mUpButton);

    RemoveWidget(mDownButton);
    delete mDownButton;
    mDownButton = new CircleScrollbutton(1, this);
    mDownButton->SetDisabled(true);
    AddWidget(mDownButton);
}

void CircleScrollbar::DrawThumb(Graphics *g, int theX, int theY, int theWidth, int theHeight)
{
    g->SetColor(kPurple);
    g->FillRect(theX, theY, theWidth, theHeight);

    // Light top-left and dark bottom-right edges give it the same bevel as before.
    g->SetColor(kPurpleLight);
    g->FillRect(theX + 1, theY + 1, theWidth - 2, 1);
    g->FillRect(theX + 1, theY + 1, 1, theHeight - 2);
    g->SetColor(kPurpleDark);
    g->FillRect(theX + 1, theY + theHeight - 2, theWidth - 2, 1);
    g->FillRect(theX + theWidth - 2, theY + 1, 1, theHeight - 2);

    g->SetColor(kYellow);
    g->DrawRect(theX, theY, theWidth - 1, theHeight - 1);
}

void CircleScrollbar::Draw(Graphics *g)
{
    int aThumbSize = GetThumbSize();
    int aThumbPosition = GetThumbPosition();

    // Vertical only (the game has no horizontal scrollbars); the base class
    // handles horizontal if ever needed.
    if (mHorizontal)
    {
        ScrollbarWidget::Draw(g);
        return;
    }

    g->SetColor(mUpdateMode == UPDATE_MODE_PGUP ? kTrackPressedColor : kTrackColor);
    g->FillRect(0, 0, mWidth, aThumbPosition);

    g->SetColor(mUpdateMode == UPDATE_MODE_PGDN ? kTrackPressedColor : kTrackColor);
    g->FillRect(0, aThumbPosition + aThumbSize, mWidth, mHeight - aThumbPosition - aThumbSize);

    // Thin yellow frame along the track, matching the list outline.
    g->SetColor(kYellow);
    g->FillRect(0, 0, 1, mHeight);
    g->FillRect(mWidth - 1, 0, 1, mHeight);

    if (aThumbSize > 0)
        DrawThumb(g, 0, aThumbPosition, mWidth, aThumbSize);
}
