#include "Zuma_Prefix.pch"

#include <SexyAppFramework/SexyAppBase.h>
#include <SexyAppFramework/SoundManager.h>
#include <SexyAppFramework/SoundInstance.h>

#include "Board.h"
#include "Bullet.h"
#include "CurveMgr.h"
#include "CircleShootApp.h"
#include "CircleCommon.h"
#include "CurveData.h"
#include "DataSync.h"
#include "LevelParser.h"
#include "WayPoint.h"
#include "ParticleMgr.h"
#include "ProfileMgr.h"
#include "SpriteMgr.h"
#include "SoundMgr.h"
#include "Res.h"

#include <math.h>

using namespace Sexy;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void BallDrawer::Reset()
{
    for (int i = 0; i < 5; i++)
    {
        mNumBalls[i] = 0;
        mNumShadows[i] = 0;
    }
}

// void BallDrawer::AddBall(Ball *theBall, int thePriority)
// {
// }

// void BallDrawer::AddShadow(Ball *theBall, int thePriority)
// {
// }

void BallDrawer::Draw(Graphics *g, SpriteMgr *theSpriteMgr, ParticleMgr *theParticleMgr)
{
    const bool minReq = GetCircleShootApp()->mBaseMinimumMode;

    for (int i = 0; i < 5; i++)
    {
        if (!minReq)
            theSpriteMgr->DrawSprites(g, i);

        // Draw crawling holes in the same priority layer as balls on that path
        // segment, so tunnel/mask overlays still correctly cover lower layers.
        // Base Minimum still draws holes (as gray circles).
        if (GetCircleShootApp()->mMovingHoleMode)
            theSpriteMgr->DrawHoles(g, i);

        if (!minReq)
            theParticleMgr->Draw(g, i);

        if (!minReq)
        {
            int aNumShadows = mNumShadows[i];
            for (int j = 0; j < aNumShadows; j++)
            {
                mShadows[i][j]->DrawShadow(g);
            }
        }

        int aNumBalls = mNumBalls[i];
        for (int j = 0; j < aNumBalls; j++)
        {
            mBalls[i][j]->Draw(g);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CurveMgr::CurveMgr(Board *theBoard)
{
    mApp = theBoard->mApp;
    mBoard = theBoard;
    mSpriteMgr = NULL; // ??
    mWayPointMgr = new WayPointMgr();
    mEffectiveEndPoint = 0;
    mEffectiveEndPointF = 0.0f;
    mHoleRotationOffset = 0.0f;
    mHoleCaughtLead = false;
}

CurveMgr::~CurveMgr()
{
    DeleteBalls();

    delete mWayPointMgr;
}

void CurveMgr::SetLosing()
{
    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); anItr++)
    {
        Bullet *aBullet = *anItr;
        delete aBullet;
    }

    mBulletList.clear();

    for (BallList::iterator anItr = mBallList.begin(); anItr != mBallList.end(); anItr++)
    {
        Ball *aBall = *anItr;
        aBall->SetSuckCount(mAdvanceSpeed * 4.0f);
    }
}

void CurveMgr::SetupLevel(LevelDesc *theDesc, SpriteMgr *theSpriteMgr, int theCurveNum, MirrorType theMirror)
{
    mLevelDesc = theDesc;
    mCurveDesc = &theDesc->mCurveDesc[theCurveNum];
    mSpriteMgr = theSpriteMgr;
    mWayPointMgr->LoadCurve(theDesc->mCurveDesc[theCurveNum].mPath, theMirror);
    mCurveNum = theCurveNum;

    float aSkullRotation = mCurveDesc->mSkullRotation;
    if (aSkullRotation >= 0.0f)
    {
        aSkullRotation = aSkullRotation * SEXY_PI / 180.0f;
    }

    int aHoleX = 0;
    int aHoleY = 0;

    if (mWayPointMgr->GetWayPointList().empty())
    {
        // Unused curve slot (levels with < 3 paths). Do not place a hole at (0,0).
        mEffectiveEndPoint = 0;
        mEffectiveEndPointF = 0.0f;
        mHoleRotationOffset = 0.0f;
        RecalcDangerPoint();
        return;
    }

    mWayPointMgr->CalcPerpendicularForPoint(mWayPointMgr->GetEndPoint());
    WayPoint const &aPoint = mWayPointMgr->GetWayPointList().back();

    aHoleX = aPoint.x;
    aHoleY = aPoint.y;

    if (aSkullRotation < 0.0f)
        aSkullRotation = aPoint.mRotation;

    mSpriteMgr->PlaceHole(mCurveNum, aHoleX, aHoleY, aSkullRotation,
                          mWayPointMgr->GetPriority(mWayPointMgr->GetEndPoint()));
    mEffectiveEndPoint = mWayPointMgr->GetEndPoint();
    mEffectiveEndPointF = (float)mEffectiveEndPoint;
    // End-of-path CalcPerpendicular looks backward; mid-path looks forward (180° flip).
    // Keep a stable facing offset from the authored/placed rotation.
    mHoleRotationOffset = aSkullRotation - GetHoleFacingRotation(mEffectiveEndPoint);
    RecalcDangerPoint();
}

void CurveMgr::SetupLevelDesc(LevelDesc *theDesc)
{
    mLevelDesc = theDesc;
    mCurveDesc = &theDesc->mCurveDesc[mCurveNum];

    if (mEffectiveEndPoint <= 0 || mEffectiveEndPoint > mWayPointMgr->GetEndPoint())
    {
        mEffectiveEndPoint = mWayPointMgr->GetEndPoint();
        mEffectiveEndPointF = (float)mEffectiveEndPoint;
    }
    RecalcDangerPoint();
}

int CurveMgr::GetEffectiveEndPoint() const
{
    if (!mApp->mMovingHoleMode)
        return mWayPointMgr->GetEndPoint();

    int end = mEffectiveEndPoint;
    int realEnd = mWayPointMgr->GetEndPoint();
    if (end < 0)
        end = 0;
    if (end > realEnd)
        end = realEnd;
    return end;
}

void CurveMgr::RecalcDangerPoint()
{
    int end = GetEffectiveEndPoint();
    mDangerPoint = end - mCurveDesc->mDangerDistance;
    if (mDangerPoint < 0)
        mDangerPoint = 0;
    if (mDangerPoint > end)
        mDangerPoint = end;
}

float CurveMgr::GetHoleFacingRotation(int thePoint) const
{
    const WayPointList &pts = mWayPointMgr->GetWayPointList();
    if (pts.empty())
        return 0.0f;

    int p1Idx = thePoint;
    if (p1Idx < 0)
        p1Idx = 0;
    if (p1Idx >= (int)pts.size())
        p1Idx = (int)pts.size() - 1;

    // Same convention as CalcPerpendicular at the true end: look toward the path start
    // so the hole faces the approaching chain (not flipped 180° mid-path).
    int p2Idx = p1Idx - 1;
    if (p2Idx < 0)
        p2Idx = p1Idx + 1;
    if (p2Idx >= (int)pts.size())
        p2Idx = p1Idx;

    const WayPoint &p1 = pts[p1Idx];
    const WayPoint &p2 = pts[p2Idx];

    SexyVector3 perp(p2.y - p1.y, p1.x - p2.x, 0.0f);
    perp = perp.Normalize();
    SexyVector3 v1(1, 0, 0);
    float rot = acosf(perp.Dot(v1));
    if (perp.y > 0.0f)
        rot = -rot;
    if (rot < 0.0f)
        rot += 2 * SEXY_PI;
    return rot;
}

void CurveMgr::UpdateHoleAtEffectiveEnd()
{
    if (!mApp->mMovingHoleMode)
        return;

    if (mSpriteMgr == NULL || mWayPointMgr->GetWayPointList().empty())
        return;

    const WayPointList &pts = mWayPointMgr->GetWayPointList();
    float wp = mEffectiveEndPointF;
    if (wp < 0.0f)
        wp = 0.0f;
    if (wp > (float)mWayPointMgr->GetEndPoint())
        wp = (float)mWayPointMgr->GetEndPoint();

    int i0 = (int)wp;
    if (i0 < 0)
        i0 = 0;
    if (i0 >= (int)pts.size())
        i0 = (int)pts.size() - 1;

    int i1 = i0 + 1;
    if (i1 >= (int)pts.size())
        i1 = i0;

    float t = wp - (float)i0;
    if (i0 == i1)
        t = 0.0f;

    float x = pts[i0].x + (pts[i1].x - pts[i0].x) * t;
    float y = pts[i0].y + (pts[i1].y - pts[i0].y) * t;

    float r0 = GetHoleFacingRotation(i0) + mHoleRotationOffset;
    float r1 = GetHoleFacingRotation(i1) + mHoleRotationOffset;
    while (r1 - r0 > SEXY_PI)
        r1 -= 2 * SEXY_PI;
    while (r1 - r0 < -SEXY_PI)
        r1 += 2 * SEXY_PI;

    float rot = r0 + (r1 - r0) * t;
    while (rot < 0.0f)
        rot += 2 * SEXY_PI;
    while (rot > 2 * SEXY_PI)
        rot -= 2 * SEXY_PI;

    mSpriteMgr->MoveHole(mCurveNum, (int)(x + 0.5f), (int)(y + 0.5f), rot,
                         mWayPointMgr->GetPriority(i0));
}

int CurveMgr::GetMovingHoleMinEnd() const
{
    // Crawl all the way to the path start (frog end).
    return 0;
}

int CurveMgr::GetMovingHoleStepSize() const
{
    int realEnd = mWayPointMgr->GetEndPoint();
    // ~3% of the full path per interval (at least 25 waypoints).
    int step = realEnd / 33;
    if (step < 25)
        step = 25;
    return step;
}

bool CurveMgr::HasHoleCaughtLeadBall() const
{
    if (!mApp->mMovingHoleMode || mBallList.empty())
        return false;

    if (mHoleCaughtLead)
        return true;

    // Lose only when the hole reaches the lead ball's center (classic Zuma).
    // Using the ball front/radius blocked shooting while still savable.
    return mEffectiveEndPointF <= mBallList.back()->GetWayPoint();
}

void CurveMgr::UpdateMovingHole()
{
    if (!mApp->mMovingHoleMode)
        return;

    if (mBoard->mGameState != GameState_Playing)
        return;

    int speedLevel = mApp->mMovingHoleSpeed;
    int minEnd = GetMovingHoleMinEnd();

    if (!mBallList.empty())
    {
        float leadWP = mBallList.back()->GetWayPoint();

        // Sticky catch: once centers meet, stay locked under the lead until lose —
        // or until a clear/suck pulls the lead back behind the hole (saved).
        if (mHoleCaughtLead)
        {
            if (leadWP < mEffectiveEndPointF)
            {
                mHoleCaughtLead = false;
            }
            else
            {
                mEffectiveEndPointF = leadWP;
                mEffectiveEndPoint = (int)leadWP;
                if (mEffectiveEndPoint < minEnd)
                    mEffectiveEndPoint = minEnd;
                RecalcDangerPoint();
                UpdateHoleAtEffectiveEnd();
                mInDanger = true;
                return;
            }
        }
    }
    else
    {
        mHoleCaughtLead = false;
    }

    if (speedLevel <= 0)
        return;

    if (mEffectiveEndPointF <= (float)minEnd)
    {
        mEffectiveEndPointF = (float)minEnd;
        mEffectiveEndPoint = minEnd;

        // Still lose if the chain has reached the parked hole.
        if (!mBallList.empty())
        {
            float leadWP = mBallList.back()->GetWayPoint();
            if (mEffectiveEndPointF <= leadWP)
            {
                mHoleCaughtLead = true;
                mEffectiveEndPointF = leadWP;
                mEffectiveEndPoint = (int)leadWP;
                if (mEffectiveEndPoint < minEnd)
                    mEffectiveEndPoint = minEnd;
                RecalcDangerPoint();
                UpdateHoleAtEffectiveEnd();
                mInDanger = true;
            }
        }
        return;
    }

    if (speedLevel > 100)
        speedLevel = 100;

    // Speed 100 covers one stepSize in ~1.0s (10x slower than the prior 0.1s mapping).
    // Board updates ~100 times per second → max = stepSize / 100 waypoints per update.
    float maxSpeed = (float)GetMovingHoleStepSize() * 2 / 100.0f;
    float speed = maxSpeed * ((float)speedLevel / 100.0f);
    mEffectiveEndPointF -= speed;
    if (mEffectiveEndPointF < (float)minEnd)
        mEffectiveEndPointF = (float)minEnd;

    // If the hole reaches the lead ball center, snap under it and stop crawling
    // through the chain. Lose waits for clears / sucks / in-flight shots.
    if (!mBallList.empty())
    {
        float leadWP = mBallList.back()->GetWayPoint();
        if (mEffectiveEndPointF <= leadWP)
        {
            mHoleCaughtLead = true;
            mEffectiveEndPointF = leadWP;
            mEffectiveEndPoint = (int)leadWP;
            if (mEffectiveEndPoint < minEnd)
                mEffectiveEndPoint = minEnd;
            RecalcDangerPoint();
            UpdateHoleAtEffectiveEnd();
            mInDanger = true;
            return;
        }
    }

    int newEnd = (int)(mEffectiveEndPointF + 0.5f);
    if (newEnd < minEnd)
        newEnd = minEnd;

    if (newEnd != mEffectiveEndPoint)
    {
        mEffectiveEndPoint = newEnd;
        RecalcDangerPoint();

        if (!mBallList.empty())
            mInDanger = mBallList.back()->GetWayPoint() >= mDangerPoint;
    }

    UpdateHoleAtEffectiveEnd();
}

void MakeCombo(BallList &theList, int theNumBalls, int theComboSize, int theNumColors)
{
    std::vector<int> aBallCounts;
    std::vector<int> aBallTypes;

    aBallTypes.resize(theComboSize);
    aBallCounts.resize(theComboSize);

    int aBallCount = theNumBalls;
    int aPrevColor = -1;

    for (int i = 0; i < theComboSize; i++)
    {
        int aNewColor;
        do
        {
            aNewColor = Sexy::AppRand() % theNumColors;
        } while (aPrevColor == aNewColor);

        aBallTypes[i] = aNewColor;
        aBallCounts[i] = 3;
        aBallCount -= 3;
        aPrevColor = aNewColor;
    }

    for (int i = 0; i < aBallCount; i++)
    {
        aBallCounts[Sexy::AppRand() % theComboSize]++;
    }

    BallList::iterator aBallItr = theList.begin();
    for (int i = 0; i < theComboSize; i++)
    {
        int aType = aBallTypes[i];
        int aCount = aBallCounts[i];

        for (int j = 0; j < aCount; j++)
        {
            bool insertNext = (Sexy::AppRand() % 2) != 0;
            Ball *aBall = new Ball();
            aBall->SetType(aType);
            aBall->RandomizeFrame();

            if (insertNext)
            {
                aBallItr = theList.insert(aBallItr, aBall);
            }
            else
            {
                theList.insert(theList.begin(), aBall);
            }
        }
    }
}

void CurveMgr::StartLevel()
{
    mLevelDesc = mBoard->mLevelDesc;
    mCurveDesc = &mLevelDesc->mCurveDesc[mCurveNum];
    mPathLightEndFrame = 0;
    mLastPathShowTick = Sexy::BoardGetTickCount() - 1000000;
    mLastClearedBallPoint = 0;

    for (int i = 0; i < (int)PowerType_Max; i++)
    {
        mLastPowerUpFrame[i] = mBoard->mStateCount - 1000;
    }

    mSpriteMgr->UpdateHole(mCurveNum, 0.0f);
    DeleteBalls();
    AppRand();

    int aNumBalls = mCurveDesc->mNumBalls;
    if (aNumBalls == 0)
        aNumBalls = 10;

    if (mBoard->mApp->mProfile->mMaxLevel < 2 && mLevelDesc->mStage < 1 && !mApp->mUglyChainMode)
    {
        MakeCombo(mPendingBalls, 0, 2, mCurveDesc->mNumColors);
        MakeCombo(mPendingBalls, 0, 2, mCurveDesc->mNumColors);
        MakeCombo(mPendingBalls, 0, 2, mCurveDesc->mNumColors);
        MakeCombo(mPendingBalls, 0, 2, mCurveDesc->mNumColors);
        MakeCombo(mPendingBalls, 0, 2, mCurveDesc->mNumColors);
    }

    for (int i = 0; i < aNumBalls; i++)
    {
        AddPendingBall();
    }

    mStopTime = 0;
    mSlowCount = 0;
    mBackwardCount = 0;
    mTotalBalls = mCurveDesc->mNumBalls;
    mStopAddingBalls = false;
    mInDanger = false;
    mFirstChainEnd = 0;
    mFirstBallMovedBackwards = false;

    mEffectiveEndPoint = mWayPointMgr->GetEndPoint();
    mEffectiveEndPointF = (float)mEffectiveEndPoint;
    mHoleCaughtLead = false;
    RecalcDangerPoint();
    UpdateHoleAtEffectiveEnd();

    RollBallsIn();
}

void CurveMgr::UpdatePlaying()
{
    bool ballsAtBeginning = mBallList.empty() || mBallList.back()->GetWayPoint() < 50.0f;

    if (mStopTime > 0)
    {
        mStopTime--;

        if (ballsAtBeginning)
            mStopTime = 0;

        if (mStopTime == 0)
            mAdvanceSpeed = 0;
    }

    if (mSlowCount > 0)
    {
        mSlowCount--;

        if (ballsAtBeginning)
            mSlowCount = 0;
    }

    if (mBackwardCount > 0)
    {
        mBackwardCount--;

        if (ballsAtBeginning)
            mBackwardCount = 0;
    }

    // Move the hole before advance/danger so sparkles + danger music use the new end.
    UpdateMovingHole();

    AddBall();
    UpdateBallRotation();
    AdvanceBullets();
    UpdateSuckingBalls();
    AdvanceBalls();
    AdvanceBackwardBalls();
    RemoveBallsAtFront();
    UpdateSets();
    UpdatePowerUps();

    if (mBallList.empty())
    {
        SetFarthestBall(0);
    }
    else
    {
        SetFarthestBall(mBallList.back()->GetWayPoint());
    }
}

void CurveMgr::UpdateLosing()
{
    BallList::iterator aBallItr = mBallList.begin();
    float anEndPoint = (float)GetEffectiveEndPoint();
    bool isDirty = false;

    while (aBallItr != mBallList.end())
    {
        Ball *aBall = *aBallItr;

        if (aBall->GetWayPoint() >= anEndPoint)
        {
            int aSuckCount = aBall->GetSuckCount();
            if (aSuckCount < 0)
            {
                aBall->SetSuckCount(aSuckCount + 1);
            }
            else
            {
                delete aBall;
                BallList::iterator aDeleteItr = aBallItr;
                ++aBallItr;
                mBallList.erase(aDeleteItr);

                isDirty = true;
                continue;
            }
        }
        else
        {
            mWayPointMgr->SetWayPoint(aBall, aBall->GetWayPoint() + (aBall->GetSuckCount() / 4));
            aBall->SetSuckCount(aBall->GetSuckCount() + 1);

            if (aBall->GetWayPoint() > anEndPoint)
            {
                aBall->SetSuckCount(0);
            }
        }

        aBallItr++;
        isDirty = true;
    }

    if (!mBallList.empty())
    {
        SetFarthestBall(mBallList.back()->GetWayPoint());
    }

    if (isDirty)
    {
        mBoard->MarkDirty();
    }
}

void CurveMgr::DrawBalls(BallDrawer &theDrawer)
{
    for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); ++aBallItr)
    {
        Ball *aBall = *aBallItr;
        int aPriority = mWayPointMgr->GetPriority(aBall);
        Ball *aNextBall = aBall->GetNextBall(true);
        int aNextPriority = (aNextBall && aPriority > mWayPointMgr->GetPriority(aNextBall))
                                ? mWayPointMgr->GetPriority(aNextBall)
                                : aPriority;

        int aNumBalls = theDrawer.mNumBalls[aPriority]++;
        theDrawer.mBalls[aPriority][aNumBalls] = aBall;

        int aNumShadows = theDrawer.mNumShadows[aNextPriority]++;
        theDrawer.mShadows[aNextPriority][aNumShadows] = aBall;
    }

    for (BulletList::iterator aBulletItr = mBulletList.begin(); aBulletItr != mBulletList.end(); ++aBulletItr)
    {
        Bullet *aBullet = *aBulletItr;
        int aPriority = mWayPointMgr->GetPriority(aBullet);

        int aNumBalls = theDrawer.mNumBalls[aPriority]++;
        theDrawer.mBalls[aPriority][aNumBalls] = reinterpret_cast<Ball *>(aBullet);

        int aNumShadows = theDrawer.mNumShadows[aPriority]++;
        theDrawer.mShadows[aPriority][aNumShadows] = reinterpret_cast<Ball *>(aBullet);
    }
}

void CurveMgr::DrawBallsInTunnel(Graphics *g, bool drawInTunnel)
{
    for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); ++aBallItr)
    {
        Ball *aBall = *aBallItr;
        bool inTunnel = mWayPointMgr->InTunnel((int)aBall->GetWayPoint());
        if (inTunnel == drawInTunnel)
            aBall->Draw(g);
    }

    for (BulletList::iterator aBulletItr = mBulletList.begin(); aBulletItr != mBulletList.end(); ++aBulletItr)
    {
        Bullet *aBullet = *aBulletItr;
        bool inTunnel = mWayPointMgr->InTunnel(aBullet);
        if (inTunnel == drawInTunnel)
            aBullet->Draw(g);
    }
}

bool CurveMgr::CheckCollision(Bullet *theBullet)
{
    Bullet *aBullet = theBullet;
    Ball *ball;
    bool flag;

    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); anItr++)
    {
        aBullet = *anItr;

        if (theBullet->CollidesWithPhysically(aBullet))
        {
            aBullet->Update();
            AdvanceMergingBullet(anItr);
            break;
        }
    }

    BallList::iterator aBallItr;
    for (aBallItr = mBallList.begin();; ++aBallItr)
    {
        if (aBallItr == mBallList.end())
        {
            return false;
        }

        ball = *aBallItr;

        if (ball->CollidesWithPhysically(theBullet) && ball->GetBullet() == NULL && ball->GetClearCount() == 0)
        {
            Ball *aPrevBall = ball->GetPrevBall(true);
            if (aPrevBall == NULL || aPrevBall->GetBullet() == NULL)
            {
                Ball *aNextBall = ball->GetNextBall(true);
                if (aNextBall == NULL || aNextBall->GetBullet() == NULL)
                {
                    SexyVector3 v(ball->GetX(), ball->GetY(), 0.0f);
                    SexyVector3 impliedObject(theBullet->GetX(), theBullet->GetY(), 0.0f);
                    SexyVector3 v2 = mWayPointMgr->CalcPerpendicular(ball->GetWayPoint());

                    flag = (impliedObject - v).Cross(v2).z < 0.0f;
                    if (!mWayPointMgr->InTunnel(ball, flag))
                    {
                        break;
                    }
                }
            }
        }
    }

    if (aBallItr != mBallList.end())
    {
        theBullet->SetHitBall(ball, flag);
        theBullet->SetMergeSpeed(mCurveDesc->mMergeSpeed);

        Ball *nextBall2 = ball->GetNextBall(false);
        if (!flag)
        {
            theBullet->RemoveGapInfoForBall(ball->GetId());
        }
        else if (nextBall2 != NULL)
        {
            theBullet->RemoveGapInfoForBall(nextBall2->GetId());
        }

        mApp->PlaySample(Sexy::SOUND_BALLCLICK2);
        mBulletList.push_back(theBullet);

        return true;
    }
    return false;
}

bool CurveMgr::CheckGapShot(Bullet *theBullet)
{
    int aBulRadius = theBullet->GetRadius();
    int aBulDiameter = aBulRadius * 2;
    float aBulDiameterSq = (float)aBulDiameter * (float)aBulDiameter;
    float aBulX = theBullet->GetX();
    float aBulY = theBullet->GetY();
    int aNumWayPoints = (int)mWayPointMgr->GetNumPoints();
    int aBallIdx = theBullet->GetCurCurvePoint(mCurveNum);

    if (aBallIdx > 0 && aBallIdx < aNumWayPoints)
    {
        const WayPoint *aWayPoint = &mWayPointMgr->GetWayPointList()[aBallIdx];
        if (aBulDiameterSq > ((aWayPoint->y - aBulY) * (aWayPoint->y - aBulY) +
                              (aWayPoint->x - aBulX) * (aWayPoint->x - aBulX)))
        {
            return false;
        }

        theBullet->SetCurCurvePoint(mCurveNum, 0);
    }

    for (int i = 1; i < aNumWayPoints; i += aBulDiameter)
    {
        const WayPoint *aWayPoint = &mWayPointMgr->GetWayPointList()[i];
        if (!aWayPoint->mInTunnel && (aBulDiameterSq > (aWayPoint->y - aBulY) * (aWayPoint->y - aBulY) +
                                                           (aWayPoint->x - aBulX) * (aWayPoint->x - aBulX)))
        {
            theBullet->SetCurCurvePoint(mCurveNum, i);

            for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); ++aBallItr)
            {
                Ball *aBall = *aBallItr;

                if (aBall->GetWayPoint() > i)
                {
                    Ball *aPrevBall = aBall->GetPrevBall();
                    if (aPrevBall == NULL)
                    {
                        return false;
                    }

                    int aBallDist = aBall->GetWayPoint() - aPrevBall->GetWayPoint();
                    if (aBallDist <= 0)
                    {
                        return false;
                    }

                    return theBullet->AddGapInfo(mCurveNum, aBallDist, aBall->GetId());
                }
            }

            return false;
        }
    }

    return false;
}

int CurveMgr::GetRandomPendingBallColor(int theMaxNumBalls)
{
    return Sexy::AppRand() % theMaxNumBalls;
}

bool CurveMgr::HasPendingBallOfType(int theType, int theMaxNumBalls)
{
    int aCount = 0;
    for (BallList::iterator aBallItr = mPendingBalls.begin(); aBallItr != mPendingBalls.end(); ++aBallItr)
    {
        if (aCount >= theMaxNumBalls)
        {
            break;
        }

        Ball *aBall = *aBallItr;
        if (aBall->GetType() == theType)
        {
            return true;
        }

        aCount++;
    }

    return false;
}

bool CurveMgr::IsLosing()
{
    if (mHaveSets ||
        mBallList.empty() ||
        mBackwardCount > 0)
    {
        return false;
    }

    // A clear may have just started this frame (clearCount set, mHaveSets not
    // updated yet if called mid-update). Never lose while head balls are clearing.
    Ball *aLead = mBallList.back();
    if (aLead->GetClearCount() > 0)
        return false;

    if (mApp->mMovingHoleMode)
    {
        // Once the hole is on the lead, merging bullets further back must not
        // freeze lose — only clears/sucks and in-flight board shots can save.
        if (!HasHoleCaughtLeadBall())
            return false;
    }
    else if (GetEffectiveEndPoint() > mBallList.back()->GetWayPoint() ||
             !mBulletList.empty())
    {
        return false;
    }

    Ball *aBall = mBallList.back();
    while (aBall != NULL)
    {
        if (aBall->GetSuckCount() > 0)
            return false;

        aBall = aBall->GetPrevBall(true);
    }

    return true;
}

bool CurveMgr::IsWinning()
{
    bool is_empty = mBallList.empty() && mPendingBalls.empty();

    return is_empty;
}

bool CurveMgr::CanRestart()
{
    return mBallList.empty();
}

bool CurveMgr::CanFire()
{
    if (mBallList.empty())
        return true;

    // Allow shooting until the lead center reaches the hole. Do not use the sticky
    // catch flag here — near-miss overlap must still let the player save with a match.
    float endWP = mApp->mMovingHoleMode ? mEffectiveEndPointF : (float)GetEffectiveEndPoint();
    return mBallList.back()->GetWayPoint() < endWP;
}

Ball *CurveMgr::CheckBallIntersection(const SexyVector3 &p1, const SexyVector3 &v1, float &t)
{
    BallList::iterator aBallItr = mBallList.begin();
    Ball *anIntersectBall = NULL;

    while (aBallItr != mBallList.end())
    {
        Ball *aBall = *aBallItr;

        if (!mWayPointMgr->InTunnel(aBall->GetWayPoint()))
        {
            float t2;
            if (aBall->Intersects(p1, v1, t2))
            {
                if (t2 < t && t2 > 0.0f)
                {
                    t = t2;
                    anIntersectBall = aBall;
                }
            }
        }

        ++aBallItr;
    }

    return anIntersectBall;
}

bool gGotPowerUp[(int)PowerType_Max] = {false, false, false, false};

void CurveMgr::ActivatePower(Ball *theBall)
{
    PowerType aPowerType = theBall->GetPowerTypeWussy();
    if (aPowerType >= 0 && aPowerType < PowerType_Max)
        gGotPowerUp[aPowerType] = true;

    if (aPowerType == PowerType_Bomb)
    {
        ActivateBomb(theBall);
    }
    else if (aPowerType == PowerType_MoveBackwards)
    {
        if (!mBallList.empty())
        {
            mBackwardCount = 300;
        }
    }
    else if (aPowerType == PowerType_SlowDown)
    {
        if (mSlowCount < 1000)
        {
            mSlowCount = 800;
        }
    }
}

int gBoardColor;
int gCurveColor;

void CurveMgr::DrawCurve(CurveDrawer &theDrawer)
{
    mWayPointMgr->DrawCurve(theDrawer, Color(gCurveColor), mDangerPoint);
}

void CurveMgr::DrawTunnel(CurveDrawer &theDrawer)
{
    mWayPointMgr->DrawTunnel(theDrawer);
}

void CurveMgr::DrawBaseMinimumOverlays(Graphics *g)
{
    // Semi-transparent rectangles along tunnel / underpass segments where
    // chains intersect or pass under scenery (mInTunnel waypoints).
    if (mWayPointMgr == NULL || g == NULL)
        return;

    const WayPointList &pts = mWayPointMgr->GetWayPointList();
    if (pts.empty())
        return;

    const int pad = Sexy::GetDefaultBallRadius();
    g->SetColor(Color(180, 180, 180, 70));

    int i = 0;
    const int n = (int)pts.size();
    while (i < n)
    {
        if (!pts[i].mInTunnel)
        {
            i++;
            continue;
        }

        int start = i;
        while (i < n && pts[i].mInTunnel)
            i++;

        // Chunk long snaking tunnels so overlays stay local to the path.
        const int chunk = 40;
        for (int c = start; c < i; c += chunk)
        {
            int cEnd = c + chunk;
            if (cEnd > i)
                cEnd = i;
            float cMinX = pts[c].x, cMaxX = pts[c].x;
            float cMinY = pts[c].y, cMaxY = pts[c].y;
            for (int j = c; j < cEnd; j++)
            {
                if (pts[j].x < cMinX)
                    cMinX = pts[j].x;
                if (pts[j].x > cMaxX)
                    cMaxX = pts[j].x;
                if (pts[j].y < cMinY)
                    cMinY = pts[j].y;
                if (pts[j].y > cMaxY)
                    cMaxY = pts[j].y;
            }
            int rx = (int)(cMinX - pad);
            int ry = (int)(cMinY - pad);
            int rw = (int)(cMaxX - cMinX) + pad * 2;
            int rh = (int)(cMaxY - cMinY) + pad * 2;
            if (rw < pad * 2)
                rw = pad * 2;
            if (rh < pad * 2)
                rh = pad * 2;
            g->FillRect(rx, ry, rw, rh);
        }
    }
}

void CurveMgr::DeleteBalls()
{
    for (BulletList::iterator aBulletItr = mBulletList.begin(); aBulletItr != mBulletList.end(); aBulletItr++)
    {
        Bullet *aBullet = *aBulletItr;
        delete aBullet;
    }

    for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); aBallItr++)
    {
        Ball *aBall = *aBallItr;
        delete aBall;
    }

    for (BallList::iterator aBallItr = mPendingBalls.begin(); aBallItr != mPendingBalls.end(); aBallItr++)
    {
        Ball *aBall = *aBallItr;
        delete aBall;
    }

    mBallList.clear();
    mPendingBalls.clear();
    mBulletList.clear();
}

void CurveMgr::GetPoint(int thePoint, int &x, int &y, int &pri)
{
    if (thePoint < 0)
        thePoint = 0;

    if (thePoint >= mWayPointMgr->GetNumPoints())
        thePoint = mWayPointMgr->GetEndPoint();

    const WayPoint *aWayPoint = &mWayPointMgr->GetWayPointList()[thePoint];

    x = aWayPoint->x;
    y = aWayPoint->y;
    pri = aWayPoint->mPriority;
}

int CurveMgr::GetCurveLength()
{
    return mWayPointMgr->GetNumPoints();
}

int CurveMgr::GetTotalBalls()
{
    if (mCurveDesc->mNumBalls == 0)
    {
        return 0;
    }

    return mTotalBalls;
}

void CurveMgr::SetStopAddingBalls(bool stop)
{
    if (mStopAddingBalls == stop)
        return;

    if (GetFarthestBallPercent() > 50)
    {
        mBackwardCount = mCurveDesc->mZumaBack;
        mSlowCount = mCurveDesc->mZumaSlow;
    }

    mStopAddingBalls = stop;
    if (stop)
    {
        for (BallList::iterator aBallItr = mPendingBalls.begin();
             aBallItr != mPendingBalls.end();
             aBallItr++)
            delete *aBallItr;

        mPendingBalls.clear();
    }
}

void CurveMgr::DoEndlessZumaEffect()
{
    if (GetFarthestBallPercent() > 50)
    {
        mBackwardCount = mCurveDesc->mZumaBack;
    }
}

void CurveMgr::DetonateBalls()
{
    for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); ++aBallItr)
    {
        Ball *aBall = *aBallItr;
        if (aBall->GetClearCount() == 0)
        {
            aBall->StartClearCount(true);
            mBoard->UpdateBallColorMap(aBall, false);
        }
    }
}

int CurveMgr::GetFarthestBallPercent()
{
    if (mBallList.empty())
        return 0;

    float aWayPoint = mBallList.back()->GetWayPoint();
    int end = GetEffectiveEndPoint();
    if (end <= 0)
        return 0;

    return (int)(aWayPoint * 100.0f / (float)end);
}

int CurveMgr::DrawPathSparkles(int theStartPoint, int theStagger, bool addSound)
{
    if (mApp->mBaseMinimumMode)
        return theStagger;

    int aPathHiliteWP = theStartPoint;
    bool forwardPitch = ((mCurveNum ^ 1) & 1) != 0;
    int aPathHilitePitch = forwardPitch ? 0 : -20;
    int aSoundCtr = 0;

    while (aPathHiliteWP <= GetEffectiveEndPoint())
    {
        int aSparkleX, aSparkleY, aSparklePriority;

        GetPoint(aPathHiliteWP, aSparkleX, aSparkleY, aSparklePriority);
        mBoard->mParticleMgr->AddSparkle(aSparkleX, aSparkleY, 0, 0, aSparklePriority, 0, theStagger, 0xFFFF00);

        if (addSound && (aSoundCtr % 25) == 0)
        {
            if (forwardPitch)
            {
                if (aPathHilitePitch > -20)
                    aPathHilitePitch--;
            }
            else
            {
                if (aPathHilitePitch < 0)
                    aPathHilitePitch++;
            }

            mBoard->mSoundMgr->AddSound(Sexy::SOUND_TRAIL_LIGHT, theStagger, 0, aPathHilitePitch * 0.8f);
        }

        aPathHiliteWP += 11;
        ++theStagger;
        ++aSoundCtr;
    }

    if (addSound)
        mBoard->mSoundMgr->AddSound(Sexy::SOUND_TRAIL_LIGHT_END, theStagger);

    mSpriteMgr->AddHoleFlash(mCurveNum, theStagger);

    return theStagger + 60;
}

int CurveMgr::DrawEndLevelBonus(int theStagger)
{
    FloatingTextHelper aFloat;

    int aEndPoint = GetEffectiveEndPoint();
    int aPoint = mLastClearedBallPoint;
    if (aEndPoint > aPoint)
        aPoint += (aEndPoint - aPoint) % 60;
    if (aPoint > aEndPoint)
        aPoint = aEndPoint;

    int aStagger = theStagger;

    while (aPoint <= aEndPoint)
    {
        int aExplodeX, aExplodeY, aExplodePriority;
        GetPoint(aPoint, aExplodeX, aExplodeY, aExplodePriority);
        for (int i = 0; i < 5; i++)
        {
            int ox = Sexy::AppRand() % 21 - 10;
            int oy = Sexy::AppRand() % 21 - 10;

            mBoard->mParticleMgr->AddExplosion(
                aExplodeX + ox,
                aExplodeY + oy,
                0,
                0,
                aStagger);
        }

        aFloat.Clear();
        aFloat.AddText(Sexy::StrFormat("+%d", 100), Sexy::FONT_FLOAT_ID, 0xFFFF00);
        aFloat.AddToMgr(mBoard->mParticleMgr, aExplodeX, aExplodeY, aStagger + 10, 100);

        if (aPoint >= aEndPoint)
            break;

        aPoint += 60;
        if (aPoint > aEndPoint)
            aPoint = aEndPoint;
        aStagger += 4;
    }

    for (int i = theStagger + 1; i < aStagger; i += 10)
    {
        mBoard->mSoundMgr->AddSound(Sexy::SOUND_BONUS_EXPLOSION, i);
    }

    return aStagger;
}

bool CurveMgr::HasReachedCruisingSpeed()
{
    // Compare against the effective cruising speed (Sonic multiplier included).
    // Using the unscaled curve speed left the roll-in loop running until the
    // 500-frame timeout whenever Sonic made the real cruise speed higher.
    float aCruisingSpeed = mCurveDesc->mSpeed * mApp->GetChainSpeedMultiplier();
    return (mAdvanceSpeed - aCruisingSpeed) < 0.1f;
}

void CurveMgr::AddPowerUp(PowerType thePower)
{
    if (mApp->IsPowerUpDisabled((int)thePower))
        return;

    int aBallIdx = Sexy::AppRand() % mBallList.size();
    BallList::iterator aBallItr = mBallList.begin();
    for (int i = 0; i < aBallIdx; ++i)
    {
        ++aBallItr;
    }

    Ball *b = *aBallItr;

    if (b->GetPowerType() == PowerType_None && b->GetDestPowerType() == PowerType_None)
        b->SetPowerType(thePower);
}

void CurveMgr::SyncState(DataSync &theSync)
{
    DataReader *aReader = theSync.mReader;
    DataWriter *aWriter = theSync.mWriter;

    if (aReader)
    {
        DeleteBalls();

        int aBulCount = aReader->ReadShort();
        for (int i = 0; i < aBulCount; i++)
        {
            Bullet *aBullet = new Bullet();
            aBullet->SyncState(theSync);
            mBulletList.push_back(aBullet);
        }

        int aPendingBallCount = aReader->ReadShort();
        for (int i = 0; i < aPendingBallCount; i++)
        {
            Ball *aBall = new Ball();
            aBall->SyncState(theSync);
            mPendingBalls.push_back(aBall);
        }

        int aBallCount = aReader->ReadShort();
        for (int i = 0; i < aBallCount; i++)
        {
            Ball *aBall = new Ball();
            aBall->SyncState(theSync);
            aBall->InsertInList(mBallList, mBallList.end());
        }
    }
    else
    {
        aWriter->WriteShort((short)mBulletList.size());
        for (BulletList::iterator aBulletItr = mBulletList.begin(); aBulletItr != mBulletList.end(); aBulletItr++)
        {
            Bullet *aBullet = *aBulletItr;
            aBullet->SyncState(theSync);
        }

        aWriter->WriteShort((short)mPendingBalls.size());
        for (BallList::iterator aBallItr = mPendingBalls.begin(); aBallItr != mPendingBalls.end(); aBallItr++)
        {
            Ball *aBall = *aBallItr;
            aBall->SyncState(theSync);
        }

        aWriter->WriteShort((short)mBallList.size());
        for (BallList::iterator aBallItr = mBallList.begin(); aBallItr != mBallList.end(); aBallItr++)
        {
            Ball *aBall = *aBallItr;
            aBall->SyncState(theSync);
        }
    }

    for (int i = 0; i < (int)PowerType_Max; i++)
    {
        theSync.SyncLong(mLastPowerUpFrame[i]);
    }

    theSync.SyncLong(mStopTime);
    theSync.SyncLong(mSlowCount);
    theSync.SyncLong(mBackwardCount);
    theSync.SyncLong(mTotalBalls);
    theSync.SyncFloat(mAdvanceSpeed);
    theSync.SyncShort(mFirstChainEnd);
    theSync.SyncBool(mFirstBallMovedBackwards);
    theSync.SyncBool(mHaveSets);
    theSync.SyncLong(mPathLightEndFrame);
    theSync.SyncBool(mHadPowerUp);
    theSync.SyncLong(mLastPathShowTick);
    theSync.SyncLong(mLastClearedBallPoint);
    theSync.SyncBool(mStopAddingBalls);
    theSync.SyncBool(mInDanger);
    theSync.SyncLong(mEffectiveEndPoint);
    theSync.SyncFloat(mEffectiveEndPointF);
    theSync.SyncFloat(mHoleRotationOffset);
    theSync.SyncBool(mHoleCaughtLead);

    if (aReader)
    {
        int realEnd = mWayPointMgr->GetEndPoint();
        if (realEnd < 0)
            realEnd = 0;
        if (mEffectiveEndPoint < 0)
            mEffectiveEndPoint = 0;
        if (mEffectiveEndPoint > realEnd)
            mEffectiveEndPoint = realEnd;
        if (mEffectiveEndPointF < 0.0f)
            mEffectiveEndPointF = 0.0f;
        if (mEffectiveEndPointF > (float)realEnd)
            mEffectiveEndPointF = (float)realEnd;
        RecalcDangerPoint();
        // Hole sprite is restored after Board::LoadGame finishes waiting on the
        // level-load worker — MoveHole here can race SetupLevel and freeze.
    }
}

void CurveMgr::RestoreMovingHoleAfterLoad()
{
    RecalcDangerPoint();
    UpdateHoleAtEffectiveEnd();
}

void CurveMgr::DeleteBullet(Bullet *theBullet)
{
    if (theBullet == NULL)
        return;

    BulletList::iterator anItr = std::find(mBulletList.begin(), mBulletList.end(), theBullet);

    if (anItr != mBulletList.end())
    {
        mBulletList.erase(anItr);
    }

    delete theBullet;
}

void CurveMgr::DeleteBall(Ball *theBall)
{
    Bullet *aBullet = theBall->GetBullet();
    if (aBullet != NULL)
    {
        aBullet->MergeFully();
        BulletList::iterator anItr = std::find(mBulletList.begin(), mBulletList.end(), aBullet);
        if (anItr != mBulletList.end())
            AdvanceMergingBullet(anItr);
    }

    DeleteBullet(aBullet);
    theBall->SetCollidesWithPrev(false);
    delete theBall;
}

void CurveMgr::SetFarthestBall(int thePoint)
{
    int aLastPoint = mDangerPoint;
    if (aLastPoint < 0)
        aLastPoint = 0;

    int anEnd = GetEffectiveEndPoint();
    float aPercentOpen = 0.0f;
    if (aLastPoint < anEnd && aLastPoint <= thePoint)
        aPercentOpen = (float)(thePoint - aLastPoint) / (float)(anEnd - aLastPoint);
    if (aPercentOpen > 1.0f)
        aPercentOpen = 1.0f;

    mSpriteMgr->UpdateHole(mCurveNum, aPercentOpen);
}

int CurveMgr::GetNumInARow(Ball *theBall, int theColor, Ball **theNextEnd, Ball **thePrevEnd)
{
    if (theBall->GetType() != theColor)
        return 0;

    Ball *aBall = theBall;
    int aColor = theColor;
    int aCount = 1;

    Ball *aNextEnd = aBall;
    while (true)
    {
        Ball *aNextBall = aNextEnd->GetNextBall(true);
        if (aNextBall == NULL || aNextBall->GetType() != aColor)
            break;

        aNextEnd = aNextBall;
        aCount++;
    }

    Ball *aPrevEnd = aBall;
    while (true)
    {
        Ball *aPrevBall = aPrevEnd->GetPrevBall(true);
        if (aPrevBall == NULL || aPrevBall->GetType() != aColor)
            break;

        aPrevEnd = aPrevBall;
        aCount++;
    }

    if (theNextEnd != NULL)
        *theNextEnd = aNextEnd;
    if (thePrevEnd != NULL)
        *thePrevEnd = aPrevEnd;

    return aCount;
}

bool CurveMgr::CheckSet(Ball *theBall)
{
    mHadPowerUp = false;
    Ball *aPrevEnd = NULL;
    Ball *aNextEnd = NULL;
    int aComboCount = theBall->GetComboCount();
    if (mApp->mCombolessMode)
        aComboCount = 0;

    int aCount = GetNumInARow(theBall, theBall->GetType(), &aNextEnd, &aPrevEnd);

    if (aCount < 3)
    {
        return false;
    }

    mBoard->mNumCleared = 0;
    mBoard->mClearedXSum = 0;
    mBoard->mClearedYSum = 0;
    mBoard->mCurComboCount = aComboCount;
    mBoard->mCurComboScore = theBall->GetComboScore();
    mBoard->mNeedComboCount.clear();

    for (int i = 0; i < PowerType_Max; i++)
        gGotPowerUp[i] = false;

    int aGapBonus = 0;
    int aNumGaps = 0;
    Ball *anEndBall = aNextEnd->GetNextBall();
    Ball *aBall = aPrevEnd;

    while (aBall != anEndBall)
    {
        if (aBall->GetSuckPending())
        {
            aBall->SetSuckPending(false);
            mBoard->mNumClearsInARow++;
        }

        StartClearCount(aBall);
        aGapBonus += aBall->GetGapBonus();
        if (aBall->GetNumGaps() > aNumGaps)
            aNumGaps = aBall->GetNumGaps();

        aBall->SetGapBonus(0, 0);
        aBall = aBall->GetNextBall();
    }

    DoScoring(theBall, mBoard->mNumCleared, aComboCount, aGapBonus, aNumGaps);

    if (mBoard->mCurComboCount > mBoard->mLevelStats.mMaxCombo ||
        mBoard->mCurComboCount == mBoard->mLevelStats.mMaxCombo &&
            mBoard->mCurComboScore >= mBoard->mLevelStats.mMaxComboScore)
    {
        mBoard->mLevelStats.mMaxCombo = mBoard->mCurComboCount;
        mBoard->mLevelStats.mMaxComboScore = mBoard->mCurComboScore;
    }

    aBall = aPrevEnd;
    while (aBall != anEndBall)
    {
        aBall->SetComboCount(aComboCount, mBoard->mCurComboScore);
        aBall = aBall->GetNextBall();
    }

    BallList::iterator anItr = mBoard->mNeedComboCount.begin();
    for (;
         anItr != mBoard->mNeedComboCount.end();
         anItr++)
    {
        Ball *aBall = *anItr;
        aBall->SetComboCount(aComboCount, mBoard->mCurComboScore);
    }

    mBoard->mNeedComboCount.clear();

    // Normally power clears skip the destroy SFX (power SFX play instead).
    // With Max power quiet-sounds, keep a single regular destroy sound instead.
    if (!mHadPowerUp || (mApp->mMaxPowerMode && mApp->mMaxPowerQuietSounds))
    {
        int *destroySound = &Sexy::SOUND_BALLDESTROYED1;

        switch (aComboCount)
        {
        case 0:
            destroySound = &Sexy::SOUND_BALLDESTROYED1;
            break;
        case 1:
            destroySound = &Sexy::SOUND_BALLDESTROYED2;
            break;
        case 2:
            destroySound = &Sexy::SOUND_BALLDESTROYED3;
            break;
        case 3:
            destroySound = &Sexy::SOUND_BALLDESTROYED4;
            break;
        default:
            destroySound = &Sexy::SOUND_BALLDESTROYED5;
            break;
        }

        mApp->PlaySample(*destroySound);

        SoundInstance *aSound = mApp->mSoundManager->GetSoundInstance(Sexy::SOUND_COMBO);
        if (aSound != NULL)
        {
            aSound->AdjustPitch(2 * aComboCount);
            aSound->SetVolume(min(1.0, aComboCount * 0.2 + 0.4));
            aSound->Play(false, true);
        }
    }

    mBoard->mCurComboCount = 0;
    mBoard->mCurComboScore = 0;

    return true;
}

void CurveMgr::DoScoring(Ball *theBall, int theNumBalls, int theComboCount, int theGapBonus, int theNumGaps)
{
    if (theNumBalls == 0)
    {
        return;
    }

    FloatingTextHelper aFloat;
    int aComboCount = theComboCount;
    int aGapBonus = theGapBonus;
    int aNumGaps = theNumGaps;

    if (mApp->mCombolessMode)
        aComboCount = 0;
    if (mApp->mGapFreeMode)
    {
        aGapBonus = 0;
        aNumGaps = 0;
    }

    int aNumPoints = 100 * aComboCount + 10 * theNumBalls + aGapBonus;
    bool inARow = false;
    int aRowBonus = 0;

    int chainThreshold = 5; // vanilla: bonus when clears-in-a-row > 4
    bool chainBonusAllowed = true;
    if (mApp->mChainCountMode)
    {
        if (mApp->mChainBonusDisabled)
            chainBonusAllowed = false;
        else
            chainThreshold = mApp->mChainBonusThreshold;
    }

    if (chainBonusAllowed && mBoard->mNumClearsInARow >= chainThreshold && aComboCount == 0)
    {
        aRowBonus = 10 * mBoard->mNumClearsInARow + 50;
        aNumPoints += aRowBonus;
        mBoard->mCurInARowBonus += aRowBonus;
        inARow = true;
    }

    mBoard->mCurComboScore += aNumPoints;
    mBoard->IncScore(aNumPoints);

    if (aComboCount > 0)
        ++mBoard->mLevelStats.mNumCombos;

    if (aGapBonus)
        ++mBoard->mLevelStats.mNumGaps;

    int theColor = gTextBallColors[theBall->GetType()];
    aFloat.AddText(Sexy::StrFormat("+%d", aNumPoints), Sexy::FONT_FLOAT_ID, theColor);

    if (aComboCount > 0)
    {
        aFloat.AddText(Sexy::StrFormat("COMBO x%d", aComboCount + 1), Sexy::FONT_FLOAT_ID, theColor);
    }

    if (aGapBonus > 0)
    {
        std::string scoreString;

        if (aNumGaps > 1)
        {
            mBoard->mSoundMgr->AddSound(Sexy::SOUND_GAP_BONUS, 15, 0, 2.0);

            if (aNumGaps > 2)
            {
                scoreString = "TRIPLE GAP BONUS";
            }
            else
            {
                mBoard->mSoundMgr->AddSound(Sexy::SOUND_GAP_BONUS, 30, 0, 3.0);
                scoreString = "DOUBLE GAP BONUS";
            }
        }
        else
        {
            scoreString = "GAP BONUS";
        }

        aFloat.AddText(scoreString, Sexy::FONT_FLOAT_ID, theColor);
        mBoard->mApp->PlaySample(Sexy::SOUND_GAP_BONUS);
    }

    if (inARow)
    {
        aFloat.AddText(Sexy::StrFormat("CHAIN BONUS x%d", mBoard->mNumClearsInARow), Sexy::FONT_FLOAT_ID, theColor);
        int chainPitch = mBoard->mNumClearsInARow - chainThreshold;
        if (chainPitch < 0)
            chainPitch = 0;
        mBoard->mSoundMgr->AddSound(Sexy::SOUND_CHAIN_BONUS, 0, 0, chainPitch);
    }

    int aClrX = mBoard->mClearedXSum / theNumBalls;
    int aClrY = mBoard->mClearedYSum / theNumBalls;

    if (gGotPowerUp[PowerType_SlowDown])
    {
        aFloat.AddText("SLOWDOWN Ball", Sexy::FONT_FLOAT_ID, theColor);
    }

    if (gGotPowerUp[PowerType_MoveBackwards])
    {
        aFloat.AddText("BACKWARDS Ball", Sexy::FONT_FLOAT_ID, theColor);
    }

    if (gGotPowerUp[PowerType_Accuracy])
    {
        aFloat.AddText("ACCURACY Ball", Sexy::FONT_FLOAT_ID, theColor);
    }

    aFloat.AddToMgr(mBoard->mParticleMgr, aClrX, aClrY);
}

static void GetNumPendingSinglesHelper(int aColor, int &aNumGroups, int &aPrevColor, int &aNumSingles, int &aGroupCount)
{
    if (aColor == aPrevColor)
    {
        ++aGroupCount;
    }
    else
    {
        if (aGroupCount == 1)
            ++aNumSingles;
        aGroupCount = 1;
        ++aNumGroups;
        aPrevColor = aColor;
    }
}

int CurveMgr::GetNumPendingSingles(int theNumGroups)
{
    int aNumGroups = 0;
    int aPrevColor = -1;
    int aNumSingles = 0;
    int aGroupCount = 0;

    BallList::reverse_iterator anRItr = mPendingBalls.rbegin();
    while (anRItr != mPendingBalls.rend())
    {
        Ball *aBall = *anRItr;
        if (aNumGroups > theNumGroups)
            break;

        GetNumPendingSinglesHelper(aBall->GetType(), aNumGroups, aPrevColor, aNumSingles, aGroupCount);

        anRItr++;
    }

    for (BallList::iterator anItr = mPendingBalls.begin(); anItr != mPendingBalls.end(); ++anItr)
    {
        Ball *aBall = *anItr;

        GetNumPendingSinglesHelper(aBall->GetType(), aNumGroups, aPrevColor, aNumSingles, aGroupCount);
    }

    return aNumSingles;
}

void CurveMgr::AddPendingBall()
{
    int aNewColor = 0;
    int aPrevColor = 0;
    int aNumColors = mCurveDesc->mNumColors;
    Ball *aBall = new Ball();
    aBall->RandomizeFrame();

    if (gColorOverride)
        aNumColors = gNumColors;

    if (!mPendingBalls.empty())
    {
        aPrevColor = mPendingBalls.back()->GetType();
    }
    else if (!mBallList.empty())
    {
        aPrevColor = mBallList.front()->GetType();
    }
    else
    {
        aPrevColor = GetRandomPendingBallColor(aNumColors);
    }

    if (aPrevColor >= aNumColors)
    {
        aPrevColor = GetRandomPendingBallColor(aNumColors);
    }

    int aMaxSingle = mCurveDesc->mMaxSingle;
    if (mApp->mUglyChainMode)
    {
        // Pick uniformly from colors other than the neighbor.
        if (aNumColors <= 1)
        {
            aNewColor = 0;
        }
        else
        {
            aNewColor = Sexy::AppRand() % (aNumColors - 1);
            if (aNewColor >= aPrevColor)
                aNewColor++;
        }
    }
    else if (Sexy::AppRand() % 100 <= mCurveDesc->mBallRepeat)
    {
        aNewColor = aPrevColor;
    }
    else if (aMaxSingle < 10 && GetNumPendingSingles(1) == 1 && (aMaxSingle == 0 || GetNumPendingSingles(10) > aMaxSingle))
    {
        aNewColor = aPrevColor;
    }
    else
    {
        do
        {
            aNewColor = GetRandomPendingBallColor(aNumColors);
        } while (aNewColor == aPrevColor);
    }

    aBall->SetType(aNewColor);
    mPendingBalls.push_back(aBall);
}

void CurveMgr::AddBall()
{
    if (mPendingBalls.empty())
    {
        if (mCurveDesc->mNumBalls != 0 || mStopAddingBalls)
            return;

        AddPendingBall();
    }

    Ball *aBall = mPendingBalls.front();
    mWayPointMgr->SetWayPoint(aBall, 1.0f);

    if (!mBallList.empty())
    {
        Ball *aFrontBall = mBallList.front();
        if (aBall->GetWayPoint() > aFrontBall->GetWayPoint() || aFrontBall->CollidesWith(aBall))
        {
            return;
        }
    }

    mBoard->UpdateBallColorMap(aBall, true);

    aBall->InsertInList(mBallList, mBallList.begin());
    aBall->UpdateCollisionInfo(5 + mAdvanceSpeed);
    aBall->SetNeedCheckCollision(true);
    aBall->SetRotation(mWayPointMgr->GetRotationForPoint(aBall->GetWayPoint()));
    aBall->SetBackwardsCount(0);
    aBall->SetSuckCount(0);
    aBall->SetGapBonus(0, 0);
    aBall->SetComboCount(0, 0);

    if (mApp->mMaxPowerMode && mApp->mMaxPowerPercent > 0)
    {
        if ((Sexy::AppRand() % 100) < mApp->mMaxPowerPercent)
        {
            int available[PowerType_Max];
            int count = 0;
            for (int i = 0; i < (int)PowerType_Max; i++)
            {
                if (!mApp->IsPowerUpDisabled(i))
                    available[count++] = i;
            }

            if (count > 0)
                aBall->SetPowerType((PowerType)available[Sexy::AppRand() % count], false);
        }
    }

    mPendingBalls.pop_front();
}

void CurveMgr::UpdateBallRotation()
{
    for (BallList::iterator anItr = mBallList.begin(); anItr != mBallList.end(); ++anItr)
    {
        Ball *aBall = *anItr;
        aBall->UpdateRotation();
    }

    for (BulletList::iterator anItr = mBulletList.begin(); anItr != mBulletList.end(); ++anItr)
    {
        Bullet *aBullet = *anItr;
        aBullet->UpdateRotation();
    }
}

void CurveMgr::AdvanceBalls()
{
    if (mBallList.empty())
        return;

    float aMaxSpeed = mCurveDesc->mSpeed;
    if (mCurveDesc->mAccelerationRate != 0.0f)
    {
        mCurveDesc->mCurAcceleration += mCurveDesc->mAccelerationRate;
        aMaxSpeed += mCurveDesc->mCurAcceleration;

        if (aMaxSpeed > mCurveDesc->mMaxSpeed)
        {
            aMaxSpeed = mCurveDesc->mMaxSpeed;
        }
    }

    if (mSlowCount != 0)
    {
        aMaxSpeed /= 4.0f;
    }

    if (mFirstChainEnd >= mDangerPoint - mCurveDesc->mSlowDistance)
    {
        if (mFirstChainEnd < mDangerPoint)
        {
            float aDist = (mFirstChainEnd - (mDangerPoint - mCurveDesc->mSlowDistance)) / float(mCurveDesc->mSlowDistance);
            aMaxSpeed = (1 - aDist) * aMaxSpeed + aDist * aMaxSpeed / float(mCurveDesc->mSlowFactor);
        }
        else
        {
            aMaxSpeed /= mCurveDesc->mSlowFactor;
        }
    }

    if (mBoard->mIsEndless)
    {
        if (mBoard->mStateCount > 300)
        {
            int farthestBallPercent = GetFarthestBallPercent();
            int gauntletHurryDist = 30;
            if (farthestBallPercent < gauntletHurryDist)
            {
                aMaxSpeed = ((aMaxSpeed + (gauntletHurryDist - farthestBallPercent) * aMaxSpeed) * 3.0f) / (float)gauntletHurryDist;

                if (aMaxSpeed > mAdvanceSpeed)
                {
                    mAdvanceSpeed += 0.03f;
                }
            }
        }
    }

    aMaxSpeed *= mApp->GetChainSpeedMultiplier();

    if (mAdvanceSpeed > aMaxSpeed)
    {
        mAdvanceSpeed -= 0.1f;
    }

    if (mAdvanceSpeed < aMaxSpeed)
    {
        mAdvanceSpeed += 0.005f;

        if (mAdvanceSpeed >= aMaxSpeed)
        {
            mAdvanceSpeed = aMaxSpeed;
        }
    }

    Ball *aBall = mBallList.front();
    float aNextWayPoint = aBall->GetWayPoint();
    if (!mFirstBallMovedBackwards && !mStopTime)
    {
        mWayPointMgr->SetWayPoint(aBall, aNextWayPoint + mAdvanceSpeed);
    }

    BallList::iterator anItr = mBallList.begin();
    Ball *aFirstChainEnd = NULL;

    while (anItr != mBallList.end())
    {
        aBall = *anItr++;

        if (anItr == mBallList.end())
        {
            break;
        }

        Ball *aNextBall = *anItr;
        aNextWayPoint = aNextBall->GetWayPoint();
        float aWayPoint = aBall->GetWayPoint();

        if (aWayPoint > aNextWayPoint - aBall->GetRadius() - aNextBall->GetRadius())
        {
            mWayPointMgr->SetWayPoint(aNextBall, aWayPoint + aBall->GetRadius() + aNextBall->GetRadius());

            if (!aBall->GetCollidesWithNext())
            {
                aBall->SetCollidesWithNext(true);
                mBoard->PlayBallClick(Sexy::SOUND_BALLCLICK1);
            }

            aBall->SetNeedCheckCollision(false);
        }

        if (aFirstChainEnd == NULL)
        {
            if (!aBall->GetCollidesWithNext())
                aFirstChainEnd = aBall;
        }
    }

    if (aFirstChainEnd == NULL)
    {
        aFirstChainEnd = mBallList.back();
    }

    mFirstChainEnd = aFirstChainEnd->GetWayPoint();

    if (mFirstChainEnd >= mDangerPoint)
    {
        int aTick = Sexy::BoardGetTickCount();
        int end = GetEffectiveEndPoint();
        int aDenom = end - mDangerPoint;
        if (aDenom < 1)
            aDenom = 1;
        int aMaxTime = 100 + 4000 * (end - mFirstChainEnd) / aDenom;
        int aFrame = mBoard->GetStateCount();

        if (aFrame >= mPathLightEndFrame && aTick - mLastPathShowTick >= aMaxTime)
        {
            mApp->PlaySample(Sexy::SOUND_WARNING);
            mLastPathShowTick = aTick;
            mPathLightEndFrame = aFrame + DrawPathSparkles(mFirstChainEnd, 0, false);
        }
    }

    mInDanger = mBallList.back()->GetWayPoint() >= mDangerPoint;
}

void CurveMgr::AdvanceBackwardBalls()
{
    mFirstBallMovedBackwards = false;

    if (mBallList.empty())
        return;

    BallList::reverse_iterator anItr = mBallList.rbegin();
    bool aCollided = false;
    float aBackwardsSpeed = 0.0f;

    if (mBackwardCount != 0)
    {
        Ball *aBack = mBallList.back();
        // Post-clear knockback uses a longer pulse (count ~30) on this shared state.
        // Do not overwrite an active knockback with the 1-tick reverse drive.
        if (aBack->GetBackwardsCount() <= 1)
        {
            aBack->SetBackwardsSpeed(1.0f);
            aBack->SetBackwardsCount(1);
        }
    }

    for (;;)
    {
        Ball *aBall = *anItr;
        int aBackwardsCount = aBall->GetBackwardsCount();

        if (aBackwardsCount > 0)
        {
            aBackwardsSpeed = aBall->GetBackwardsSpeed();
            mWayPointMgr->SetWayPoint(aBall, aBall->GetWayPoint() - aBackwardsSpeed);
            aBall->SetBackwardsCount(aBackwardsCount - 1);
            aCollided = true;
        }

        anItr++;
        if (anItr == mBallList.rend())
        {
            break;
        }

        Ball *aNextBall = *anItr;

        if (aCollided)
        {
            if (aNextBall->GetCollidesWithNext())
            {
                float aPoint = aNextBall->GetWayPoint() - aBackwardsSpeed;
                mWayPointMgr->SetWayPoint(aNextBall, aPoint);
            }
            else
            {
                const float aWayOffNext = aBall->GetWayPoint() - (float)aBall->GetRadius() - (float)aNextBall->GetRadius();

                if (aNextBall->GetWayPoint() > aWayOffNext)
                {
                    aNextBall->SetCollidesWithNext(true);
                    aCollided = true;
                    mBoard->PlayBallClick(Sexy::SOUND_BALLCLICK1);
                    aBackwardsSpeed = aNextBall->GetWayPoint() - aWayOffNext;
                    aNextBall->SetWayPoint(aWayOffNext);
                }
                else
                {
                    aCollided = false;
                }
            }
        }
    }

    if (aCollided)
    {
        mFirstBallMovedBackwards = true;
        if (mStopTime < 20)
        {
            mStopTime = 20;
        }
    }
}

void CurveMgr::UpdateSuckingBalls()
{
    BallList::iterator anItr = mBallList.begin();
    while (anItr != mBallList.end())
    {
        Ball *aBall = *anItr;
        int aSuckCount = aBall->GetSuckCount();

        if (aSuckCount <= 0)
        {
            ++anItr;
            continue;
        }

        Ball *aNextBall = NULL;
        float aSuck = aSuckCount / 8;
        while (anItr != mBallList.end())
        {

            aNextBall = *anItr++;
            aNextBall->SetSuckCount(0);
            mWayPointMgr->SetWayPoint(aNextBall, aNextBall->GetWayPoint() - aSuck);

            Bullet *aBullet = aNextBall->GetBullet();
            if (aBullet != NULL)
            {
                Ball *aPushBall = aBullet->GetPushBall();
                if (aPushBall != NULL)
                {
                    mWayPointMgr->FindFreeWayPoint(aPushBall, aBullet, false);
                }

                aBullet->UpdateHitPos();
            }

            if (!aNextBall->GetCollidesWithNext())
                break;
        }

        aBall->SetSuckCount(aSuckCount + 1);
        Ball *aPrevBall = aBall->GetPrevBall(false);

        if (aPrevBall == NULL)
        {
            aBall->SetSuckCount(0);
            continue;
        }

        float aNewWayPoint = (aBall->GetWayPoint() - aBall->GetRadius()) - aPrevBall->GetRadius();
        if (aPrevBall->GetWayPoint() > aNewWayPoint)
        {
            mWayPointMgr->SetWayPoint(aPrevBall, aNewWayPoint);
            mBoard->PlayBallClick(SOUND_BALLCLICK1);
            aPrevBall->SetCollidesWithNext(true);
            aBall->SetSuckCount(0);
            if (!CheckSet(aBall))
            {
                aBall->SetComboCount(0, 0);
            }

            // Knockback after a clear/suck collision. Shared with reverse-power state —
            // always apply/refresh so reverse cannot swallow the knockback pulse.
            {
                float aKnockSpeed = aBall->GetComboCount() * 1.5f;
                if (aKnockSpeed < 0.5f)
                    aKnockSpeed = 0.5f;

                if (aNextBall->GetBackwardsCount() < 30 ||
                    aNextBall->GetBackwardsSpeed() < aKnockSpeed)
                {
                    aNextBall->SetBackwardsCount(30);
                    aNextBall->SetBackwardsSpeed(aKnockSpeed);
                }
            }

            ClearPendingSucks(aNextBall);
        }
    }
}

void CurveMgr::UpdateSets()
{
    mHaveSets = false;
    BallList::iterator anItr = mBallList.begin();
    while (anItr != mBallList.end())
    {
        Ball *aBall = *anItr;
        int aClearCount = aBall->GetClearCount();
        if (aClearCount > 0)
        {
            mHaveSets = true;
        }

        if (aClearCount < 40)
        {
            if (aClearCount > 0)
            {
                aBall->SetClearCount(aClearCount + 1);
            }
            anItr++;
        }
        else
        {
            Ball *aNextBall = aBall->GetNextBall();
            Ball *aPrevBall = aBall->GetPrevBall();

            if (!mApp->mCombolessMode &&
                aNextBall != NULL && aNextBall->GetClearCount() == 0 && aPrevBall != NULL &&
                aNextBall->GetType() == aPrevBall->GetType())
            {
                aNextBall->SetSuckCount(10);
                aNextBall->SetComboCount(aBall->GetComboCount() + 1, aBall->GetComboScore());
            }

            if (anItr == mBallList.begin())
            {
                mAdvanceSpeed = 0.0f;
                if (mStopTime < 40)
                {
                    mStopTime = 40;
                }
            }

            DeleteBall(aBall);
            mBallList.erase(anItr++);
        }
    }
}

void CurveMgr::UpdatePowerUps()
{
    if (mBallList.empty())
        return;

    // Max power already stamps every spawned ball; skip random mid-chain rolls.
    if (mApp->mMaxPowerMode)
        return;

    for (int i = 0; i < (int)PowerType_Max; i++)
    {
        if (mApp->IsPowerUpDisabled(i))
            continue;

        int aFreq = mCurveDesc->mPowerUpFreq[i];

        if (aFreq > 0 && (Sexy::AppRand() % aFreq) == 0 && aFreq < mBoard->GetStateCount() - mLastPowerUpFrame[i])
        {
            AddPowerUp((PowerType)i);
            mLastPowerUpFrame[i] = mBoard->GetStateCount();
        }
    }
}

void CurveMgr::RemoveBallsAtFront()
{
    BallList::iterator anItr = mBallList.begin();
    while (anItr != mBallList.end())
    {
        Ball *aBall = *anItr;

        if (aBall->GetWayPoint() >= 1.0f)
        {
            break;
        }

        anItr++;
        DeleteBullet(aBall->GetBullet());
        aBall->RemoveFromList();

        if (aBall->GetClearCount() == 0)
        {
            mBoard->UpdateBallColorMap(aBall, false);
        }

        if (aBall->GetClearCount() != 0 || mStopAddingBalls)
        {
            DeleteBall(aBall);
        }
        else
        {
            mPendingBalls.push_front(aBall);
        }
    }
}

void CurveMgr::AdvanceMergingBullet(BulletList::iterator &theBulletItr)
{
    Bullet *aBul = *theBulletItr;
    aBul->CheckSetHitBallToPrevBall();
    Ball *aHitBall = aBul->GetHitBall();
    mWayPointMgr->SetWayPoint(aBul, aHitBall->GetWayPoint());
    mWayPointMgr->FindFreeWayPoint(aHitBall, aBul, aBul->GetHitInFront());
    aBul->SetDestPos(aBul->GetX(), aBul->GetY());
    aBul->Update();

    Ball *aPushBall = aBul->GetPushBall();

    if (aPushBall != NULL)
    {
        float num = 1.0f - aBul->GetHitPercent();
        float f = -aBul->GetRadius() * num / 2.0f;
        float aPoint = aPushBall->GetWayPoint();
        float aPercent = (aPushBall->GetRadius() + aBul->GetRadius()) * (aBul->GetHitPercent() * aBul->GetHitPercent());

        mWayPointMgr->FindFreeWayPoint(aBul, aBul->GetPushBall(), true, (int)f);

        if (aPushBall->GetWayPoint() - aBul->GetWayPoint() > aPercent)
        {
            float anEndPoint = aBul->GetWayPoint() + aPercent;
            if (anEndPoint > aPoint)
            {
                mWayPointMgr->SetWayPoint(aPushBall, anEndPoint);
            }
            else
            {
                mWayPointMgr->SetWayPoint(aPushBall, aPoint);
            }
        }

        aPushBall->SetNeedCheckCollision(true);
    }

    if (aBul->GetHitPercent() >= 1.0f)
    {
        BallList::iterator aBallItr = aHitBall->GetListItr();
        if (aBul->GetHitInFront())
        {
            aBallItr++;
        }

        Ball *aNewBall = new Ball();
        aNewBall->SetRotation(aBul->GetRotation());
        aNewBall->SetType(aBul->GetType());
        aNewBall->SetPowerType(aBul->GetPowerType(), false);
        mWayPointMgr->SetWayPoint(aNewBall, aBul->GetWayPoint());
        aNewBall->SetFrame(0);
        aNewBall->InsertInList(mBallList, aBallItr);
        mBoard->UpdateBallColorMap(aNewBall, true);

        int aMinGapDist = aBul->GetMinGapDist();
        int aNumGaps = aBul->GetNumGaps();

        delete aBul;
        theBulletItr = mBulletList.erase(theBulletItr);
        mTotalBalls++;

        Ball *aPrevBall = aNewBall->GetPrevBall();
        Ball *aNextBall = aNewBall->GetNextBall();
        aNewBall->UpdateCollisionInfo(5);
        aNewBall->SetNeedCheckCollision(true);

        if (aPrevBall && aNewBall->GetCollidesWithPrev())
        {
            aPrevBall->SetNeedCheckCollision(true);
        }

        if (aMinGapDist > 0 && !mApp->mGapFreeMode)
        {
            aMinGapDist -= GetDefaultBallRadius() * 4;
            if (aMinGapDist < 0)
            {
                aMinGapDist = 0;
            }

            int aBonusRate = (mBoard->mIsEndless ? 250 : 500);
            int aGapBonus = ((MAX_GAP_SIZE - aMinGapDist) * aBonusRate / MAX_GAP_SIZE);
            aGapBonus = (aGapBonus / 10) * 10;
            if (aGapBonus < 10)
            {
                aGapBonus = 10;
            }
            if (aNumGaps > 1)
            {
                aGapBonus *= aNumGaps;
            }
            aNewBall->SetGapBonus(aGapBonus, aNumGaps);
        }

        mBoard->mNumClearsInARow++;

        Ball *aRowNextEnd = NULL;
        Ball *aRowPrevEnd = NULL;
        int aRowCount = GetNumInARow(aNewBall, aNewBall->GetType(), &aRowNextEnd, &aRowPrevEnd);

        bool didClear = CheckSet(aNewBall);
        bool matchOfThreeOrMore = didClear;

        if (mApp->mBomberMode && aRowCount > 2)
        {
            int aTicks = Sexy::BoardGetTickCount();
            if (aTicks - mBoard->mLastExplosionTick > 250)
            {
                mBoard->mLastExplosionTick = aTicks;
                mApp->PlaySample(Sexy::SOUND_EXPLODE);
            }

            for (int i = 0; i < mBoard->mNumCurves; i++)
                mBoard->mCurveMgr[i]->ActivateBomb(aNewBall);

            didClear = true;
        }

        // Colors ban only applies to normal 3+ matches, not bomber-only 3-ball blasts.
        if (matchOfThreeOrMore && mApp->IsColorBanned(aNewBall->GetType()))
        {
            mBoard->SetLosing();
            return;
        }

        if (!didClear)
        {
            mBoard->mNumClearsInARow--;

            // Pull same-color groups across a gap together (not a post-clear combo).
            // Comboless disables this suck as well as chain-reaction combos.
            if (!mApp->mCombolessMode &&
                aPrevBall != NULL &&
                !aPrevBall->GetCollidesWithNext() &&
                aPrevBall->GetType() == aNewBall->GetType() &&
                aPrevBall->GetBullet() == NULL &&
                aPrevBall->GetClearCount() == 0)
            {
                aNewBall->SetSuckPending(true);
                aNewBall->SetSuckCount(1);
            }
            else if (!mApp->mCombolessMode &&
                     aNextBall != NULL &&
                     !aNewBall->GetCollidesWithNext() &&
                     aNextBall->GetType() == aNewBall->GetType() &&
                     aNextBall->GetBullet() == NULL &&
                     aNextBall->GetClearCount() == 0)
            {
                aNewBall->SetSuckPending(true);
                if (aNextBall->GetSuckCount() <= 0)
                {
                    aNextBall->SetSuckCount(1);
                }
            }
            else
            {
                mBoard->ResetInARowBonus();
                aNewBall->SetGapBonus(0, 0);
            }
        }
    }
    else
    {
        theBulletItr++;
    }
}

void CurveMgr::AdvanceBullets()
{
    BulletList::iterator anItr = mBulletList.begin();

    while (anItr != mBulletList.end())
    {
        AdvanceMergingBullet(anItr);
    }
}

void CurveMgr::StartClearCount(Ball *theBall)
{
    if (theBall->GetClearCount() > 0)
        return;

    mBoard->UpdateBallColorMap(theBall, false);
    mBoard->mLevelStats.mNumBallsCleared++;
    mBoard->mNumCleared++;

    mBoard->mClearedXSum += theBall->GetX();
    mBoard->mClearedYSum += theBall->GetY();
    mLastClearedBallPoint = theBall->GetWayPoint();

    if (theBall->GetSuckPending())
    {
        theBall->SetSuckPending(false);
    }

    theBall->StartClearCount(mWayPointMgr->InTunnel(theBall->GetWayPoint()));

    if (theBall->GetPowerTypeWussy() != PowerType_None)
    {
        mBoard->ActivatePower(theBall);
        mHadPowerUp = true;
    }
}

void CurveMgr::ActivateBomb(Ball *theBall)
{
    int aColor = gBallColors[theBall->GetType()];
    int aBallX = theBall->GetX();
    int aBallY = theBall->GetY();
    mBoard->mParticleMgr->AddExplosion(aBallX, aBallY, 0, aColor, 5);
    int v19 = Sexy::IMAGE_EXPLOSION->mWidth / 3;

    for (int i = v19, a6 = 7; i < 100; i += v19, a6 += 4)
    {
        float v21 = 0.0f;
        do
        {
            mBoard->mParticleMgr->AddExplosion(
                aBallX + (Sexy::AppRand() % 21 - 10) + (sinf(v21) * i),
                aBallY + (Sexy::AppRand() % 21 - 10) + (cosf(v21) * i),
                0,
                aColor,
                a6);
            v21 += ((float)v19 / (float)i);
        } while (v21 < SEXY_PI * 2);
    }

    mBoard->mParticleMgr->AddExplosion(
        theBall->GetX(),
        theBall->GetY(),
        0,
        aColor,
        0);

    for (BallList::iterator anItr = mBallList.begin(); anItr != mBallList.end(); anItr++)
    {
        Ball *aBall = *anItr;

        if (aBall->GetClearCount() == 0 && aBall->CollidesWithPhysically(theBall, 45))
        {
            if (!mApp->mCombolessMode)
                aBall->SetComboCount(mBoard->mCurComboCount, mBoard->mCurComboScore);
            mBoard->mNeedComboCount.push_back(aBall);
            StartClearCount(aBall);
            mBoard->mParticleMgr->AddExplosion(
                theBall->GetX(),
                theBall->GetY(),
                0,
                aColor,
                0);
        }
    }
}

void CurveMgr::ClearPendingSucks(Ball *theEndBall)
{
    if (theEndBall == NULL)
        return;

    Ball *aBall = theEndBall;
    bool aCollided = true;

    while (aBall != NULL)
    {
        if (aBall->GetSuckPending())
        {
            aBall->SetSuckPending(false);
            mBoard->ResetInARowBonus();
            aBall->SetGapBonus(0, 0);
        }

        aBall = aBall->GetPrevBall();
        if (aBall == NULL)
        {
            return;
        }

        if (!aBall->GetCollidesWithNext())
        {
            aCollided = false;
        }

        if (!aCollided && aBall->GetSuckCount() != 0)
        {
            return;
        }
    }
}

void CurveMgr::RollBallsIn()
{
    float aBaseSpeed = mCurveDesc->mSpeed;
    float aSpeed = aBaseSpeed * mApp->GetChainSpeedMultiplier();
    int aStartDistance = 50;
    if (!mBoard->mIsEndless)
    {
        aStartDistance = mCurveDesc->mStartDistance;
    }

    float aWayPoint = (float)(aStartDistance * mWayPointMgr->GetNumPoints() / 100);
    aWayPoint -= (float)mFirstChainEnd / mWayPointMgr->GetNumPoints();

    if (mFirstChainEnd <= 0 || aWayPoint > 0.0f)
    {
        // Compute the roll-in boost from the unscaled base speed so Sonic doesn't
        // stretch the spawn/roll-in sound; then apply that same boost above cruise.
        float aBoostedBase =
            aBaseSpeed +
            ((sqrtf(((aBaseSpeed + 20.0f) * (aBaseSpeed + 20.0f)) + ((aWayPoint * -20.0f) * -4.0f)) -
              (aBaseSpeed + 20.0f)) *
                 0.5f +
             18.0f) *
                0.1f;
        mAdvanceSpeed = aSpeed + (aBoostedBase - aBaseSpeed);
    }
    else
    {
        mAdvanceSpeed = aSpeed;
    }
}

void CurveMgr::ApplyColorShift(const int theMap[MAX_BALL_COLORS])
{
    if (theMap == NULL)
        return;

    for (BallList::iterator anItr = mBallList.begin(); anItr != mBallList.end(); ++anItr)
    {
        Ball *aBall = *anItr;
        if (aBall == NULL || aBall->GetClearCount() != 0)
            continue;

        int src = aBall->GetType();
        if (src < 0 || src >= MAX_BALL_COLORS)
            continue;

        int dest = theMap[src];
        if (dest < 0 || dest >= MAX_BALL_COLORS || dest == src)
            continue;

        mBoard->UpdateBallColorMap(aBall, false);
        aBall->BeginColorShift(dest);
        mBoard->UpdateBallColorMap(aBall, true);
    }

    for (BallList::iterator anItr = mPendingBalls.begin(); anItr != mPendingBalls.end(); ++anItr)
    {
        Ball *aBall = *anItr;
        if (aBall == NULL)
            continue;

        int src = aBall->GetType();
        if (src < 0 || src >= MAX_BALL_COLORS)
            continue;

        int dest = theMap[src];
        if (dest < 0 || dest >= MAX_BALL_COLORS || dest == src)
            continue;

        // Pending balls aren't on-screen yet — snap instantly.
        aBall->SetType(dest);
    }
}

void CurveMgr::ApplyInvisible(int theFrames, int thePercent)
{
    if (theFrames <= 0 || thePercent <= 0)
        return;
    if (thePercent > 100)
        thePercent = 100;

    Ball *candidates[512];
    int count = 0;
    for (BallList::iterator anItr = mBallList.begin(); anItr != mBallList.end(); ++anItr)
    {
        Ball *aBall = *anItr;
        if (aBall == NULL || aBall->GetClearCount() != 0)
            continue;
        if (count < 512)
            candidates[count++] = aBall;
    }

    if (count == 0)
        return;

    int n = (count * thePercent + 50) / 100;
    if (n < 1)
        n = 1;
    if (n > count)
        n = count;

    for (int i = 0; i < n; i++)
    {
        int j = i + (int)(Sexy::AppRand() % (count - i));
        Ball *tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
        candidates[i]->SetInvisible(theFrames);
    }
}