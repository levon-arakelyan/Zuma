#include "Zuma_Prefix.pch"

#include <SexyAppFramework/Graphics.h>
#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/SexyVector.h>
#include <SexyAppFramework/Image.h>
#include <SexyAppFramework/Font.h>

#include "Ball.h"
#include "BlendedImage.h"
#include "CircleCommon.h"
#include "CircleShootApp.h"
#include "DataSync.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

const int Ball::COLOR_SHIFT_BLEND_FRAMES = 30; // 0.3s at ~100 updates/sec
const int Ball::INVISIBLE_BLEND_FRAMES = 30;  // 0.3s fade in/out

BlendedImage *gBlendedBombLights[MAX_BALL_COLORS];
BlendedImage *gBlendedPowerupLights[4];
BlendedImage *gBlendedPowerups[4][MAX_BALL_COLORS];
BlendedImage *gBlendedBalls[MAX_BALL_COLORS];

int Ball::mIdGen = 0;
bool gSpeedUp = false;
bool gWasReset = false;
bool gBallBlink = false;
bool gForceTreasure = false;
int gForceTreasureNum = 0;
int gColorOverride = 0;
int gNumColors = 4;
bool gCheckCollision = true;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BlendedImage *CreateBlendedBall(int theType)
{
    if (gBlendedBalls[theType] == NULL)
    {
        Image *image = Sexy::GetImageById((ResourceId)(theType + Sexy::IMAGE_BLUE_BALL_ID));
        int aCelHeight = image->mHeight / image->mNumRows;
        Rect aRect(0, aCelHeight * (image->mNumRows / 2), image->mWidth, aCelHeight);

        gBlendedBalls[theType] = new BlendedImage(image, aRect);
    }

    return gBlendedBalls[theType];
}

BlendedImage *CreateBlendedBombLight(int theType)
{
    if (gBlendedBombLights[theType] == NULL)
    {
        Image *aImage = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_LIGHT_ID + theType));
        Rect srcRect(0, 0, aImage->GetWidth(), aImage->GetHeight());

        gBlendedBombLights[theType] = new BlendedImage(aImage, srcRect);
    }

    return gBlendedBombLights[theType];
}

static BlendedImage *CreateBlendedPowerup(int thePowerupType, int theType, Image *theImage)
{
    if (gBlendedPowerups[thePowerupType][theType] == NULL)
    {
        Rect srcRect(0, 0, theImage->GetWidth(), theImage->GetHeight());

        gBlendedPowerups[thePowerupType][theType] = new BlendedImage(theImage, srcRect);
    }

    return gBlendedPowerups[thePowerupType][theType];
}

static BlendedImage *CreateBlendedPowerupLight(int thePowerType, Image *theImage)
{
    if (gBlendedPowerupLights[thePowerType] == NULL)
    {
        Rect srcRect(0, 0, theImage->GetWidth(), theImage->GetHeight());

        gBlendedPowerupLights[thePowerType] = new BlendedImage(theImage, srcRect);
    }

    return gBlendedPowerupLights[thePowerType];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void Sexy::BallDeleteGlobals()
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        delete gBlendedBalls[i];
        gBlendedBalls[i] = NULL;
        delete gBlendedBombLights[i];
        gBlendedBombLights[i] = NULL;

        for (int j = 0; j < 4; j++)
        {
            delete gBlendedPowerups[j][i];
            gBlendedPowerups[j][i] = NULL;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        delete gBlendedPowerupLights[i];
        gBlendedPowerupLights[i] = NULL;
    }
}

Ball::Ball()
{
    mId = ++mIdGen;
    mx = my = 0.0f;
    mType = 0;
    mColorShiftFrom = -1;
    mColorShiftFrame = 0;
    mInvisibleFrames = 0;
    mInvisibleFadeFrame = 0;
    mRotateBombDraw = false;
    mBullet = NULL;
    mList = NULL;
    mCollidesWithNext = false;
    mSuckCount = 0;
    mClearCount = 0;
    mBackwardsCount = 0;
    mBackwardsSpeed = 0.0f;
    mComboCount = 0;
    mComboScore = 0;
    mRotation = 0.0f;
    mRotationInc = 0.0f;
    mNeedCheckCollision = false;
    mSuckPending = false;
    mStartFrame = 0;
    mWayPoint = 0.0f;
    mPowerType = PowerType_Max;
    mDestPowerType = PowerType_Max;
    mPowerCount = 0;
    mPowerFade = 0;
    mGapBonus = 0;
    mNumGaps = 0;
    mParticles = NULL;
}

Ball::~Ball()
{
    delete mParticles;
}

void Ball::SyncState(DataSync &theSync)
{
    theSync.RegisterPointer(this);
    theSync.SyncLong(mId);
    theSync.SyncByte(mType);
    theSync.SyncFloat(mWayPoint);
    theSync.SyncFloat(mRotation);
    theSync.SyncFloat(mDestRotation);
    theSync.SyncFloat(mRotationInc);
    theSync.SyncFloat(mx);
    theSync.SyncFloat(my);
    theSync.SyncBool(mCollidesWithNext);
    theSync.SyncBool(mNeedCheckCollision);
    theSync.SyncBool(mSuckPending);
    theSync.SyncShort(mClearCount);
    theSync.SyncShort(mSuckCount);
    theSync.SyncShort(mBackwardsCount);
    theSync.SyncFloat(mBackwardsSpeed);
    theSync.SyncByte(mComboCount);
    theSync.SyncLong(mComboScore);
    theSync.SyncByte(mStartFrame);
    theSync.SyncShort(mPowerCount);
    theSync.SyncShort(mPowerFade);
    theSync.SyncShort(mGapBonus);
    theSync.SyncByte(mNumGaps);

    if (theSync.mReader)
    {
        theSync.SyncBytes(&mPowerType, 4u);

#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mPowerType = (PowerType)ByteSwap((unsigned int)mPowerType);
#endif
    }
    else
    {
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mPowerType = (PowerType)ByteSwap((unsigned int)mPowerType);
#endif

        theSync.SyncBytes(&mPowerType, 4u);
    }

    if (theSync.mReader)
    {
        theSync.SyncBytes(&mDestPowerType, 4u);

#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mDestPowerType = (PowerType)ByteSwap((unsigned int)mDestPowerType);
#endif
    }
    else
    {
#ifdef CIRCLE_ENDIAN_SWAP_ENABLED
        mDestPowerType = (PowerType)ByteSwap((unsigned int)mDestPowerType);
#endif

        theSync.SyncBytes(&mDestPowerType, 4u);
    }

    theSync.SyncPointer((void **)&mBullet);
}

void Ball::SetFrame(int theFrame)
{
    Image *anImage = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_BALL_ID + mType));
    mStartFrame = anImage->mNumRows - (int)((float)theFrame + mWayPoint) % anImage->mNumRows;
}

void Ball::IncFrame(int theInc)
{
    Image *anImage = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_BALL_ID + mType));
    mStartFrame = (mStartFrame + theInc) % anImage->mNumRows;

    if (mStartFrame < 0)
        mStartFrame += anImage->mNumRows;
}

void Ball::RandomizeFrame()
{
    mStartFrame = Sexy::AppRand() % 50;
}

void Ball::SetPowerType(PowerType theType, bool delay)
{
    if (theType == mPowerType)
        return;

    if (delay)
    {
        mDestPowerType = theType;
        mPowerFade = 100;
    }
    else
    {
        mDestPowerType = PowerType_None;
        mPowerType = theType;
    }
}

void Ball::SetRotation(float theRot, bool immediate)
{
    if (immediate)
    {
        mRotation = theRot;
        return;
    }

    while (fabs(theRot - mRotation) > SEXY_PI)
    {
        if (theRot > mRotation)
        {
            theRot -= 6.2831802f;
        }
        else
        {
            theRot += 6.2831802f;
        }
    }

    mDestRotation = theRot;
    mRotationInc = 0.104719669f;

    if (theRot < mRotation)
    {
        mRotationInc = -mRotationInc;
    }
}

void Ball::UpdateRotation()
{
    // Invisible hold starts after fade-out finishes; then fade back in.
    if (mInvisibleFrames > 0)
    {
        if (mInvisibleFadeFrame < INVISIBLE_BLEND_FRAMES)
            ++mInvisibleFadeFrame;
        else
            --mInvisibleFrames;
    }
    else if (mInvisibleFadeFrame > 0)
    {
        --mInvisibleFadeFrame;
    }

    if (mColorShiftFrom >= 0)
    {
        ++mColorShiftFrame;
        if (mColorShiftFrame >= COLOR_SHIFT_BLEND_FRAMES)
            mColorShiftFrom = -1;
    }

    if (mPowerFade > 0)
    {
        --mPowerFade;
        if (mPowerFade == 0)
        {
            mPowerType = mDestPowerType;
            mDestPowerType = PowerType_None;

            if (mPowerType != PowerType_None)
            {
                mPowerCount = 2000;
            }
        }
    }

    if (mPowerCount > 0)
    {
        --mPowerCount;
        if (mPowerCount <= 0 && mPowerType != PowerType_None)
        {
            mDestPowerType = PowerType_None;
            mPowerFade = 100;
        }
    }

    if (mRotationInc != 0.0f)
    {
        mRotation += mRotationInc;

        if (mRotationInc > 0.0f && mRotation > mDestRotation)
        {
            mRotation = mDestRotation;
            mRotationInc = 0.0f;
        }
        else if (mRotationInc < 0.0f && mRotation < mDestRotation)
        {
            mRotation = mDestRotation;
            mRotationInc = 0.0f;
        }
    }
}

PowerType Ball::GetPowerTypeWussy()
{
    if (mPowerType == PowerType_None)
        return mDestPowerType;

    return mPowerType;
}

void Ball::SetWayPoint(float thePoint)
{
    mWayPoint = thePoint;
}

void Ball::SetPos(float x, float y)
{
    mx = x;
    my = y;
}

void Ball::StartClearCount(bool inTunnel)
{
    if (mClearCount != 0)
        return;

    mClearCount = 1;

    if (!inTunnel)
    {
        if (mParticles == NULL)
            mParticles = new Particle[60];

        for (int i = 0; i < 60; i++)
        {
            Particle &ptcl = mParticles[i];
            float angle = (float)(Sexy::AppRand() % 360) * SEXY_PI / 180.0f;
            float speed = (float)(Sexy::AppRand() % 500) / 500.0f;
            ptcl.vx = sinf(angle) * speed;
            ptcl.vy = cosf(angle) * speed;

            int rnd = Sexy::AppRand() % 30;
            ptcl.x = rnd * ptcl.vx + mx;
            ptcl.y = rnd * ptcl.vy + my;
            ptcl.mSize = (int)(Sexy::AppRand() % 10 < 2) + 1;
        }
    }
}

void Ball::SetClearCount(int theCount)
{
    mClearCount = theCount;
}

int Ball::GetRadius()
{
    return Sexy::GetDefaultBallRadius();
}

void Ball::DrawShadow(Graphics *g)
{
    if (GetInvisibleDrawAlpha() < 128)
        return;

    if (!gSexyAppBase->Is3DAccelerated())
        return;

    if (mClearCount == 0)
    {
        g->DrawImageF(Sexy::IMAGE_BALL_SHADOW,
                      (mx - Sexy::IMAGE_BALL_SHADOW->mWidth / 2) - 3.0f,
                      (my - Sexy::IMAGE_BALL_SHADOW->mHeight / 2) + 5.0f);
    }
}

bool Ball::CollidesWithPhysically(Ball *theBall, int thePad)
{
    float dx = theBall->GetX() - this->GetX();
    float dy = theBall->GetY() - this->GetY();
    float r = (float)theBall->GetRadius() + thePad;

    return dx * dx + dy * dy < r * (r * 4.0f);
}

bool Ball::CollidesWith(Ball *theBall, int thePad)
{
    return fabs((int)this->mWayPoint - (int)theBall->mWayPoint) < (2 * (thePad + Sexy::GetDefaultBallRadius()));
}

bool Ball::Intersects(const SexyVector3 &p1, const SexyVector3 &v1, float &t)
{
    SexyVector2 delta(p1.x - mx, p1.y - my);
    float a = SexyVector2(v1.y, v1.x).MagnitudeSquared();
    // float b = 2 * delta.Dot(SexyVector2(v1.x, v1.y));
    // Same as above, but this gives accurate assembly:
    float b = v1.x * delta.x;
    b += v1.y * delta.y;
    b = b + b;
    float disc = b * b - (delta.MagnitudeSquared() - (Sexy::GetDefaultBallRadius() * Sexy::GetDefaultBallRadius())) * a * 4;
    if (disc < 0.0f)
    {
        return false;
    }
    disc = sqrtf(disc);

    t = (-b - disc) / (2 * a);
    return true;
}

void Ball::SetBullet(Bullet *theBullet)
{
    mBullet = theBullet;
}

void Ball::Draw(Graphics *g)
{
    CircleShootApp *app = GetCircleShootApp();
    if (app != NULL && app->mBaseMinimumMode)
    {
        if (mClearCount != 0)
            return;

        int invAlpha = GetInvisibleDrawAlpha();
        if (invAlpha <= 0)
            return;

        if (mColorShiftFrom >= 0 && mColorShiftFrom < MAX_BALL_COLORS && mColorShiftFrom != mType)
        {
            float t = (float)mColorShiftFrame / (float)COLOR_SHIFT_BLEND_FRAMES;
            if (t < 0.0f)
                t = 0.0f;
            if (t > 1.0f)
                t = 1.0f;
            int fromAlpha = (int)((1.0f - t) * invAlpha + 0.5f);
            int toAlpha = (int)(t * invAlpha + 0.5f);
            DrawBallType(g, mColorShiftFrom, fromAlpha);
            DrawBallType(g, mType, toAlpha);
        }
        else
        {
            DrawBallType(g, mType, invAlpha);
        }

        // Power-up letter on top of the circle (A/B/R/S).
        char powerLetter = 0;
        switch (GetPowerTypeWussy())
        {
        case PowerType_Accuracy:
            powerLetter = 'A';
            break;
        case PowerType_Bomb:
            powerLetter = 'B';
            break;
        case PowerType_MoveBackwards:
            powerLetter = 'R';
            break;
        case PowerType_SlowDown:
            powerLetter = 'S';
            break;
        default:
            break;
        }
        if (powerLetter != 0 && Sexy::FONT_DIALOG != NULL)
        {
            std::string aLabel(1, powerLetter);
            int cx = (int)(mx + 0.5f);
            int cy = (int)(my + 0.5f);
            int tw = Sexy::FONT_DIALOG->StringWidth(aLabel);
            int baseline = cy + Sexy::FONT_DIALOG->GetAscent() / 2 - 1;
            g->SetFont(Sexy::FONT_DIALOG);
            // Contrast against ball fill color.
            uint c = Sexy::gBallColors[mType];
            int lum = ((c >> 16) & 0xFF) + ((c >> 8) & 0xFF) + (c & 0xFF);
            g->SetColor(lum > 400 ? Color(0, 0, 0) : Color(255, 255, 255));
            g->DrawString(aLabel, cx - tw / 2, baseline);
        }

        if (ShouldEmphasizeBanned())
            DrawBannedCross(g);

        g->SetColorizeImages(false);
        g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
        return;
    }

    if (mClearCount != 0)
    {
        DrawExplosion(g);
        g->SetColorizeImages(false);
        g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
        return;
    }

    int invAlpha = GetInvisibleDrawAlpha();
    if (invAlpha <= 0)
    {
        g->SetColorizeImages(false);
        g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
        return;
    }

    bool banned = ShouldEmphasizeBanned();

    // Power sprites don't go through DrawBallType — tint the whole draw.
    int powerAlpha = invAlpha;
    if (banned)
        powerAlpha = (powerAlpha * 191 + 127) / 255; // 0.75 opacity
    bool tintPower = powerAlpha < 255 && mPowerType != PowerType_None && mPowerType != PowerType_Max;
    if (tintPower)
    {
        g->SetColorizeImages(true);
        g->SetColor(Color(255, 255, 255, powerAlpha));
    }

    DoDraw(g);
    if (mPowerFade && (mPowerFade & 0x10) != 0)
    {
        g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
        if (tintPower)
        {
            g->SetColorizeImages(true);
            g->SetColor(Color(255, 255, 255, powerAlpha));
        }
        DoDraw(g);
    }

    if (gBallBlink)
    {
        g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
        if (tintPower)
        {
            g->SetColorizeImages(true);
            g->SetColor(Color(255, 255, 255, powerAlpha));
        }
        DoDraw(g);
    }

    if (banned)
        DrawBannedCross(g);

    g->SetColorizeImages(false);
    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void Ball::SetCollidesWithPrev(bool collidesWithPrev)
{
    Ball *aPrevBall = GetPrevBall();

    if (aPrevBall != NULL)
    {
        aPrevBall->SetCollidesWithNext(collidesWithPrev);
    }
}

bool Ball::GetCollidesWithPrev()
{
    Ball *aPrevBall = GetPrevBall();

    if (aPrevBall != NULL)
    {
        return aPrevBall->GetCollidesWithNext();
    }

    return false;
}

void Ball::UpdateCollisionInfo(int thePad)
{
    Ball *aPrevBall = GetPrevBall();
    Ball *aNextBall = GetNextBall();

    if (aPrevBall != NULL)
    {
        aPrevBall->SetCollidesWithNext(aPrevBall->CollidesWith(this, thePad));
    }

    if (aNextBall != NULL)
    {
        SetCollidesWithNext(aNextBall->CollidesWith(this, thePad));
    }
    else
    {
        SetCollidesWithNext(false);
    }
}

void Ball::RemoveFromList()
{
    if (mList != NULL)
    {
        mList->erase(mListItr);
        mList = NULL;
    }
}

const BallList::iterator &Ball::InsertInList(BallList &theList, const BallList::iterator &theInsertItr)
{
    mList = &theList;
    mListItr = theList.insert(theInsertItr, this);
    return mListItr;
}

Ball *Ball::GetPrevBall(bool mustCollide)
{
    if (mList == NULL)
        return NULL;

    BallList::iterator anItr = GetListItr();

    if (anItr == mList->begin())
        return NULL;

    if (!mustCollide)
    {
        return *--anItr;
    }
    else
    {
        Ball *aBall = *--anItr;
        if (aBall->GetCollidesWithNext())
            return aBall;

        return NULL;
    }
}

Ball *Ball::GetNextBall(bool mustCollide)
{
    if (mList == NULL)
    {
        return NULL;
    }
    else
    {
        BallList::iterator anItr = GetListItr();
        anItr++;

        if (anItr == mList->end())
            return NULL;

        if (!mustCollide || GetCollidesWithNext())
            return *anItr;

        return NULL;
    }
}

void Ball::DrawBomb(Graphics *g)
{
    Image *image = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_BOMB_ID + mType));
    int width = mx - image->GetWidth() / 2;
    int height = my - image->GetHeight() / 2;

    if (mRotateBombDraw)
    {
        if (gSexyAppBase->Is3DAccelerated())
            g->DrawImageRotatedF(image, (float)width, (float)height, mRotation);
        else
            g->DrawImageRotated(image, width, height, mRotation);
    }
    else if (gSexyAppBase->Is3DAccelerated())
    {
        g->DrawImageF(image, width, height);
    }
    else
    {
        BlendedImage *blendedImage = CreateBlendedPowerup(0, mType, image);
        blendedImage->Draw(g, width, height);
    }

    int alpha = Sexy::GetBoardStateCount();

    if (alpha % 50 <= 9)
    {
        alpha = 0;
    }
    else if (alpha % 50 <= 24)
    {
        alpha = (200 * (alpha % 50) - 2000) / 15;
    }
    else if (alpha % 50 <= 34)
    {
        alpha = 200;
    }
    else if (alpha % 50 <= 49)
    {
        alpha = 200 - (200 * (alpha % 50) - 7000) / 15;
    }

    g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
    g->SetColorizeImages(true);
    g->SetColor(Color(alpha, alpha, alpha));

    if (mRotateBombDraw)
    {
        Image *lightImage = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_LIGHT_ID + mType));
        if (gSexyAppBase->Is3DAccelerated())
            g->DrawImageRotatedF(lightImage, width + 7.0f, height + 9.0f, mRotation);
        else
            g->DrawImageRotated(lightImage, width + 7, height + 9, mRotation);
    }
    else if (gSexyAppBase->Is3DAccelerated())
    {
        Image *lightImage = Sexy::GetImageById((ResourceId)((int)Sexy::IMAGE_BLUE_LIGHT_ID + mType));
        g->DrawImageF(lightImage, width + 7.0f, height + 9.0f);
    }
    else
    {
        BlendedImage *blendedImage = CreateBlendedBombLight(mType);
        blendedImage->Draw(g, width + 7.0f, height + 9.0f);
    }

    g->SetColorizeImages(false);
    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void Ball::DrawStandardPower(Graphics *g, int theBallImageId, int theBlinkImageId, int thePowerType)
{
    Image *ballImage = Sexy::GetImageById((ResourceId)(theBallImageId + mType));
    Image *blinkImage = Sexy::GetImageById((ResourceId)theBlinkImageId);
    float ballX = mx - (ballImage->mWidth / 2);
    float ballY = my - (ballImage->mHeight / 2);

    if (gSexyAppBase->Is3DAccelerated())
    {
        g->DrawImageRotatedF(ballImage, ballX, ballY, mRotation + SEXY_PI / 2);
    }
    else
    {
        BlendedImage *blendedImage = CreateBlendedPowerup(thePowerType, mType, ballImage);
        blendedImage->Draw(g, ballX, ballY);
    }

    int alpha;
    int time = Sexy::GetBoardStateCount() % 100;

    if (time < 20)
    {
        alpha = 0;
    }
    else if (time < 50)
    {
        alpha = (255 * time - 5100) / 30;
    }
    else if (time < 70)
    {
        alpha = 255;
    }
    else if (time < 100)
    {
        alpha = 255 - (255 * time - 17850) / 30;
    }

    Color ballColor = Sexy::gDarkBallColors[mType];
    ballColor.mAlpha = alpha;
    g->SetColorizeImages(true);
    g->SetColor(ballColor);

    if (gSexyAppBase->Is3DAccelerated())
    {
        g->DrawImageRotatedF(blinkImage, ballX, ballY, mRotation + SEXY_PI / 2);
    }
    else
    {
        BlendedImage *blendedImage = CreateBlendedPowerupLight(thePowerType, blinkImage);
        blendedImage->Draw(g, ballX, ballY);
    }

    g->SetColorizeImages(false);
    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

void Ball::DrawPower(Graphics *g)
{
    switch (mPowerType)
    {
    case PowerType_Bomb:
        DrawBomb(g);
        break;
    case PowerType_SlowDown:
        DrawStandardPower(g, Sexy::IMAGE_BLUE_SLOW_ID, Sexy::IMAGE_SLOW_LIGHT_ID, PowerType_SlowDown);
        break;
    case PowerType_Accuracy:
        DrawStandardPower(g, Sexy::IMAGE_BLUE_ACCURACY_ID, Sexy::IMAGE_ACCURACY_LIGHT_ID, PowerType_Accuracy);
        break;
    case PowerType_MoveBackwards:
        DrawStandardPower(g, Sexy::IMAGE_BLUE_BACKWARDS_ID, Sexy::IMAGE_BACKWARDS_LIGHT_ID, PowerType_MoveBackwards);
        break;
    }
}

void Ball::DrawExplosion(Graphics *g)
{
    g->SetColorizeImages(true);

    // maybe there should be Sexy::IMAGE_GRAY_EXPLOSION_ID->GetCelHeight(), but doesn't seem like it's inlinable?
    int aCelHeight = Sexy::IMAGE_GRAY_EXPLOSION->mHeight / Sexy::IMAGE_GRAY_EXPLOSION->mNumRows;
    int aImgX = mx - Sexy::IMAGE_GRAY_EXPLOSION->GetWidth() / 2;
    int aImgY = my - aCelHeight / 2;

    g->SetColor(Color(gBrightBallColors[mType]));

    if (gSexyAppBase->Is3DAccelerated())
    {
        int aCel = mClearCount / 3;
        if (aCel < Sexy::IMAGE_GRAY_EXPLOSION->mNumRows)
        {
            float anAngle = (mStartFrame * SEXY_PI) / 25.0f;
            int aImgWidth = Sexy::IMAGE_GRAY_EXPLOSION->mWidth;
            Rect aRect(0, aCel * aCelHeight, aImgWidth, aCelHeight);
            g->DrawImageRotatedF(Sexy::IMAGE_GRAY_EXPLOSION, aImgX, aImgY, anAngle, &aRect);
        }
    }
    else
    {
        g->DrawImageCel(Sexy::IMAGE_GRAY_EXPLOSION, aImgX, aImgY, mClearCount / 3);
    }

    if (mParticles != NULL)
    {
        int aRed = min((gBrightBallColors[mType] >> 16) & 0xff + 200, 255);
        int aGreen = min((gBrightBallColors[mType] >> 8) & 0xff + 200, 255);
        int aBlue = min((gBrightBallColors[mType]) & 0xff + 200, 255);
        int anAlpha = mClearCount > 20 ? 255 - (255 * mClearCount - 5100) / 20 : 255;
        g->SetColor(Color(aRed, aGreen, aBlue, anAlpha));

        for (int i = 0; i != 60; ++i)
        {
            Particle *aParticle = &mParticles[i];
            g->FillRect(
                mClearCount * aParticle->vx + aParticle->x,
                mClearCount * aParticle->vy + aParticle->y,
                aParticle->mSize,
                aParticle->mSize);
        }
    }
}

void Ball::BeginColorShift(int theNewType)
{
    if (theNewType < 0 || theNewType >= MAX_BALL_COLORS)
        return;
    if (theNewType == mType)
        return;

    mColorShiftFrom = mType;
    mColorShiftFrame = 0;
    mType = theNewType;
}

void Ball::SetInvisible(int theFrames)
{
    if (theFrames < 0)
        theFrames = 0;
    // Refresh hold; fade-out continues from current fade frame.
    mInvisibleFrames = theFrames;
}

int Ball::GetInvisibleDrawAlpha() const
{
    if (mInvisibleFadeFrame <= 0)
        return 255;
    if (mInvisibleFadeFrame >= INVISIBLE_BLEND_FRAMES)
        return 0;

    return 255 - (255 * mInvisibleFadeFrame + INVISIBLE_BLEND_FRAMES / 2) / INVISIBLE_BLEND_FRAMES;
}

bool Ball::ShouldEmphasizeBanned() const
{
    CircleShootApp *app = GetCircleShootApp();
    return app != NULL && app->IsColorBanned(mType);
}

void Ball::DrawBannedCross(Graphics *g)
{
    // Thick black X spanning to the ball rim.
    int cx = (int)(mx + 0.5f);
    int cy = (int)(my + 0.5f);
    // Diagonal half-length so endpoints sit on the circle: r / sqrt(2).
    const int arm = (Sexy::GetDefaultBallRadius() * 1000) / 1414; // ~11 for r=16
    const int thickness = 3;

    g->SetColorizeImages(false);
    g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
    g->SetColor(Color(0, 0, 0, 230));

    for (int t = -(thickness / 2); t <= thickness / 2; t++)
    {
        g->DrawLineAA(cx - arm + t, cy - arm, cx + arm + t, cy + arm);
        g->DrawLineAA(cx - arm, cy - arm + t, cx + arm, cy + arm + t);
        g->DrawLineAA(cx + arm + t, cy - arm, cx - arm + t, cy + arm);
        g->DrawLineAA(cx + arm, cy - arm + t, cx - arm, cy + arm + t);
    }
}

void Ball::DrawBallType(Graphics *g, int theType, int theAlpha)
{
    if (theType < 0 || theType >= MAX_BALL_COLORS || theAlpha <= 0)
        return;

    int invAlpha = GetInvisibleDrawAlpha();
    if (invAlpha <= 0)
        return;
    if (invAlpha < 255)
        theAlpha = (theAlpha * invAlpha + 127) / 255;

    CircleShootApp *app = GetCircleShootApp();
    if (app != NULL && app->IsColorBanned(theType) && !app->mBaseMinimumMode)
        theAlpha = (theAlpha * 191 + 127) / 255; // 0.75 opacity

    if (theAlpha <= 0)
        return;

    if (app != NULL && app->mBaseMinimumMode)
    {
        uint c = Sexy::gBallColors[theType];
        g->SetColor(Color((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, theAlpha));
        Sexy::FillCircle(g, (int)(mx + 0.5f), (int)(my + 0.5f), Sexy::GetDefaultBallRadius());
        return;
    }

    Image *image = Sexy::GetImageById((ResourceId)(theType + Sexy::IMAGE_BLUE_BALL_ID));
    if (image == NULL)
        return;

    float aBallX = mx - 16.0f;
    float aBallY = my - 16.0f;
    bool colorize = theAlpha < 255;
    if (colorize)
    {
        g->SetColorizeImages(true);
        g->SetColor(Color(255, 255, 255, theAlpha));
    }

    if (gSexyAppBase->Is3DAccelerated())
    {
        int aCelHeight = image->mHeight / image->mNumRows;
        int aFrame = (mStartFrame + (int)mWayPoint) % image->mNumRows;
        Rect aRect(0, aFrame * aCelHeight, image->mWidth, aCelHeight);
        g->DrawImageRotatedF(image, aBallX, aBallY, mRotation, &aRect);
    }
    else
    {
        BlendedImage *aBlendedBall = CreateBlendedBall(theType);
        aBlendedBall->Draw(g, aBallX, aBallY);
    }

    if (colorize)
        g->SetColorizeImages(false);
}

void Ball::DoDraw(Graphics *g)
{
    if (mPowerType == PowerType_None)
    {
        if (mColorShiftFrom >= 0 && mColorShiftFrom < MAX_BALL_COLORS &&
            mColorShiftFrom != mType)
        {
            float t = (float)mColorShiftFrame / (float)COLOR_SHIFT_BLEND_FRAMES;
            if (t < 0.0f)
                t = 0.0f;
            if (t > 1.0f)
                t = 1.0f;

            int fromAlpha = (int)((1.0f - t) * 255.0f + 0.5f);
            int toAlpha = (int)(t * 255.0f + 0.5f);
            DrawBallType(g, mColorShiftFrom, fromAlpha);
            DrawBallType(g, mType, toAlpha);
        }
        else
        {
            DrawBallType(g, mType, 255);
        }
    }
    else
    {
        DrawPower(g);
    }
}