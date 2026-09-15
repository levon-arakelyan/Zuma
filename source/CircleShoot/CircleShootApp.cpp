#include "Zuma_Prefix.pch"

#include <SexyAppFramework/MTRand.h>
#include <SexyAppFramework/MusicInterface.h>
#include <SexyAppFramework/ResourceManager.h>
#include <SexyAppFramework/MemoryImage.h>
#include <SexyAppFramework/Dialog.h>
#include <SexyAppFramework/Font.h>
#include <SexyAppFramework/ImageFont.h>
#include <SexyAppFramework/WidgetManager.h>
#include <SexyAppFramework/Checkbox.h>
#include <SexyAppFramework/DialogButton.h>

#include "Board.h"
#include "DataSync.h"
#include "CircleCommon.h"
#include "CircleShootApp.h"
#include "LoadingScreen.h"
#include "MainMenu.h"
#include "CreateUserDialog.h"
#include "OptionsDialog.h"
#include "ModesDialog.h"
#include "ColorsBanDialog.h"
#include "UnpoweredDialog.h"
#include "SonicDialog.h"
#include "MovingHoleDialog.h"
#include "ChainCountDialog.h"
#include "ColorShiftDialog.h"
#include "ColorShiftTargetDialog.h"
#include "InvisibleDialog.h"
#include "KillerBallDialog.h"
#include "ShootSpeedDialog.h"
#include "ConfirmContinueDialog.h"
#include "StatsDialog.h"
#include "UserDialog.h"
#include "AdventureScreen.h"
#include "PracticeScreen.h"
#include "MoreGamesScreen.h"
#include "CreditsScreen.h"
#include "HelpScreen.h"
#include "LevelParser.h"
#include "ProfileMgr.h"
#include "HighScoreMgr.h"
#include "WidgetMover.h"
#include "WorkerThread.h"
#include "Res.h"

using namespace Sexy;

CircleShootApp::CircleShootApp()
{
    mTitle = "Zuma Deluxe " + mProductVersion;
    mRegKey = "PopCap\\Zuma";

    mAutoEnable3D = true;

    mLevelParser = new LevelParser();
    mProfileMgr = new ProfileMgr();
    mHighScoreMgr = new HighScoreMgr();
    mWidgetMover = new WidgetMover();
    mWorkerThread = new WorkerThread();

    mWidth = CIRCLE_WINDOW_WIDTH;
    mHeight = CIRCLE_WINDOW_HEIGHT;

    mAdventureScreen = NULL;
    mBoard = NULL;
    mMainMenu = NULL;
    mPracticeScreen = NULL;
    mProfile = NULL;
    mHelpScreen = NULL;
    mMoreGamesScreen = NULL;
    mLoadingScreen = NULL;
    mCreditsScreen = NULL;

    mSongId = 0;
    mLastSong = -1;
    mUnk28 = 0;
    mMusicVolume = 0.6;
    mSfxVolume = 0.6;
    mDoPlayCount = false;
    mPlayCount = 0;
    mUnk29 = 0;
    mMaxExecutions = 0;
    mMaxPlays = 0;
    mMaxTime = 0;
    mColorsBanMode = false;
    mColorsBanRandom = false;
    mColorsBanRandomCount = 1;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mBannedColors[i] = false;
        mActiveBannedColors[i] = false;
    }
    mUnpoweredMode = false;
    for (int i = 0; i < PowerType_Max; i++)
        mDisabledPowerUps[i] = false;
    mNoSwapMode = false;
    mSonicMode = false;
    mMachineGunMode = false;
    mUglyChainMode = false;
    mMovingHoleMode = false;
    mChainSpeedMultiplier = 1.0f;
    mMovingHoleSpeed = 20; // ~old 0.5s crawl rate; 100 = Ultra fast (~0.1s)
    mCombolessMode = false;
    mGapFreeMode = false;
    mChainCountMode = false;
    mChainBonusThreshold = 5; // vanilla chain bonus starts at 5 clears
    mChainBonusDisabled = false;
    mBankruptMode = false;
    mColorShiftMode = false;
    mColorShiftHz = 3.0f;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        mColorShiftMap[i] = (i + 1) % MAX_BALL_COLORS;
        mColorShiftEnabled[i] = true;
    }
    mColorShiftRandom = true;
    mInvisibleMode = false;
    mInvisibleDurationSec = 3.0f;
    mInvisibleIntervalSec = 5.0f;
    mInvisiblePercent = 50;
    mBaseMinimumMode = false;
    mAutoAdvanceMode = false;
    mKillerBallMode = false;
    mKillerBallIntervalSec = 10.0f;
    mKillerBallFlightSec = 10.0f;
    mLightSpeedMode = false;
    mShootSpeedMode = false;
    mShootSpeedMultiplier = 1.0f;
    mShootSpeedInstant = false;
}

CircleShootApp::~CircleShootApp()
{
    // Sexy::SpriteMgrDeleteGlobals()
    // Sexy::BallDeleteGlobals()
    // Sexy::GunDeleteGlobals()

    delete mLevelParser;
    delete mProfileMgr;
    delete mHighScoreMgr;
    delete mWidgetMover;
    delete mWorkerThread;

    // mResourceManager->DeleteResources("");
}

void CircleShootApp::Init()
{
    SexyAppBase::Init();

    mMaxExecutions = GetInteger("MaxExecutions", 0);
    mMaxPlays = GetInteger("MaxPlays", 0);
    mMaxTime = GetInteger("MaxTime", 60);

#ifdef _WIN32
    mLastSongSwitchTime = GetTickCount() - 100000;
    gMainThreadId = (int)GetCurrentThreadId();
#else
#ifdef _POSIX
    // todo
    gMainThreadId = (int)pthread_self();
#endif
#error Unimplemented on this platform.
#endif

    gThreadRand.SRand(Sexy::Rand());
    gAppRand.SRand(Sexy::Rand());

    this->mProfileMgr->Load();
    this->mHighScoreMgr->Load();

    std::string profile;

    if (this->RegistryReadString("CurUser", &profile))
    {
        mProfile = mProfileMgr->GetProfile(profile);
    }
    else
    {
        mProfile = mProfileMgr->GetAnyProfile();
    }

    if (!mLevelParser->ParseLevelFile("levels\\levels.xml"))
    {
        Popup(mLevelParser->GetErrorText());
        exit(0);
    }

    if (mLevelParser->mLevels.size() == 0)
    {
        Popup("No levels defined in levels.xml");
        exit(0);
    }

    // mResourceManager->SetAllowMissingProgramImages(true); // hack?

    LoadResourceManifest();
    if (!mResourceManager->LoadResources("Init"))
    {
        ShowResourceError(true);
    }

    ImageFont *fontMain10 = reinterpret_cast<ImageFont *>(mResourceManager->GetFont("FONT_MAIN10"));
    if (fontMain10)
    {
        ImageFont *fontDialog = reinterpret_cast<ImageFont *>(fontMain10->Duplicate());
        fontDialog->AddTag("Shadow1");
        mResourceManager->ReplaceFont("FONT_DIALOG", fontDialog);

        ImageFont *fontMain10Outline = reinterpret_cast<ImageFont *>(fontMain10->Duplicate());
        fontMain10Outline->RemoveTag("Outline");
        mResourceManager->ReplaceFont("FONT_MAIN10OUTLINE", fontDialog);

        ImageFont *fontMain10Outline2 = reinterpret_cast<ImageFont *>(fontMain10->Duplicate());
        fontMain10Outline2->RemoveTag("Outline2");
        mResourceManager->ReplaceFont("FONT_MAIN10OUTLINE2", fontDialog);

        ImageFont *fontMain10Outline3 = reinterpret_cast<ImageFont *>(fontMain10->Duplicate());
        fontMain10Outline3->RemoveTag("Outline3");
        mResourceManager->ReplaceFont("FONT_MAIN10OUTLINE3", fontDialog);
    }

    if (!ExtractInitResources(mResourceManager))
    {
        ShowResourceError(true);
    }

    SetCursorImage(0, Sexy::IMAGE_CURSOR_POINTER);
    SetCursorImage(1, Sexy::IMAGE_CURSOR_HAND);
    SetCursorImage(2, Sexy::IMAGE_CURSOR_DRAGGING);
    SetCursorImage(3, Sexy::IMAGE_CURSOR_TEXT);

    mMusicInterface->LoadMusic(0, "music\\zuma.mo3");
    mMusicInterface->LoadMusic(1, "music\\zuma.mo3");
    PlaySong(24, false, 0.01);
    ShowLoadingScreen();
}

void CircleShootApp::Shutdown()
{
    if (!this->mShutdown)
    {
        CleanupWidgets();

        if (this->mMaxTime <= 0)
        {
            if (this->mMaxPlays > 0)
            {
                // todo
            }
        }
        else
        {
            // todo
        }

        SexyAppBase::Shutdown();
    }
}

void CircleShootApp::UpdateFrames()
{
    SexyAppBase::UpdateFrames();
    mWidgetMover->Update();
}

void CircleShootApp::ButtonDepress(int theId)
{
    CheckYesNoButton(theId);
}

Dialog *CircleShootApp::NewDialog(int theDialogId,
                                  bool isModal,
                                  const SexyString &theDialogHeader,
                                  const SexyString &theDialogLines,
                                  const SexyString &theDialogFooter,
                                  int theButtonMode)
{
    CircleDialog *aDialog = new CircleDialog(Sexy::IMAGE_DIALOG_BACK,
                                             Sexy::IMAGE_DIALOG_BUTTON,
                                             theDialogId,
                                             isModal,
                                             theDialogHeader,
                                             theDialogLines,
                                             theDialogFooter,
                                             theButtonMode,
                                             false);

    Sexy::SetupDialog(aDialog, 348);
    return aDialog;
}

bool CircleShootApp::KillDialog(int theDialogId)
{
    Widget *aDialog = GetDialog(theDialogId);
    if (aDialog)
    {
        mWidgetMover->RemoveWidget(aDialog);
    }

    if (!SexyAppBase::KillDialog(theDialogId))
    {
        return false;
    }

    if (mDialogMap.empty())
    {
        if (mBoard != NULL)
            mWidgetManager->SetFocus(mBoard);
        else if (mMainMenu != NULL)
            mWidgetManager->SetFocus(mMainMenu);
    }

    return true;
}

void CircleShootApp::GotFocus()
{
    if (mBoard)
    {
        mBoard->Pause(false);
    }
}

void CircleShootApp::LostFocus()
{
    if (mBoard)
    {
        mBoard->Pause(true);
    }
}

void CircleShootApp::MakeBoard()
{
    mDidNextTempleDialog = false;
    CleanupWidgets();

    mBoard = new Board(this);

    mBoard->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mBoard);
    mWidgetManager->SetFocus(mBoard);
    mDoPlayCount = false;
}

void CircleShootApp::CleanupWidgets()
{
    if (mAdventureScreen)
    {
        mWidgetMover->SafeDeleteWidget(mAdventureScreen);
        mAdventureScreen = NULL;
    }

    if (mBoard)
    {
        if (mDoPlayCount)
        {
            mPlayCount++;
            mDoPlayCount = false;
        }

        mBoard->WaitForLoadingThread();
        if (mBoard->NeedSaveGame())
        {
            mBoard->SaveGame(GetSaveGameName(mBoard->IsPracticeMode(), mProfile->mId));
        }

        mWidgetMover->SafeDeleteWidget(mBoard);
        mBoard = NULL;
    }

    if (mLoadingScreen)
    {
        mWidgetMover->SafeDeleteWidget(mLoadingScreen);
        mLoadingScreen = NULL;
    }

    if (mMainMenu)
    {
        mWidgetMover->SafeDeleteWidget(mMainMenu);
        mMainMenu = NULL;
    }

    if (mPracticeScreen)
    {
        mWidgetMover->SafeDeleteWidget(mPracticeScreen);
        mPracticeScreen = NULL;
    }

    if (mMoreGamesScreen)
    {
        mWidgetMover->SafeDeleteWidget(mMoreGamesScreen);
        mMoreGamesScreen = NULL;
    }

    if (mHelpScreen)
    {
        mWidgetMover->SafeDeleteWidget(mHelpScreen);
        mHelpScreen = NULL;
    }

    if (mCreditsScreen)
    {
        mWidgetMover->SafeDeleteWidget(mCreditsScreen);
        mCreditsScreen = NULL;
    }
}

void CircleShootApp::ShowLoadingScreen()
{
    CleanupWidgets();

    mLoadingScreen = new LoadingScreen();

    mLoadingScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mLoadingScreen);
    mWidgetManager->SetFocus(mLoadingScreen);
}

void CircleShootApp::ShowHelpScreen()
{
    mHelpScreen = new HelpScreen();
    mHelpScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mHelpScreen);
    mWidgetManager->SetFocus(mHelpScreen);
    if (mBoard)
    {
        mBoard->Pause(true, true);
    }
    ClearUpdateBacklog();
}

void CircleShootApp::StartAdventureGame(int theStage)
{
    if (mDidNextTempleDialog && theStage == mProfile->mMaxStage && CheckSaveGame(false))
    {
        StartSavedGame(true);
    }
    else
    {
        MakeBoard();
        mBoard->SetStartLevel(mLevelParser->GetLevelNumByStage(theStage));
        mBoard->Reset(true);
        if (mProfile->mShowHelpScreen)
        {
            ShowHelpScreen();
        }

        PlaySong(0, 1, 0.01);
    }
}

void CircleShootApp::StartPracticeGame(const std::string &theLevelName, int theStartLevel, bool endless)
{
    MakeBoard();
    mBoard->SetPracticeBoard(theLevelName);
    mBoard->SetStartLevel(theStartLevel);
    mBoard->SetIsEndless(endless);
    mBoard->Reset(true);
    if (mProfile->mShowHelpScreen)
    {
        ShowHelpScreen();
    }

    PlaySong(0, true, 0.01);
}

bool CircleShootApp::CheckSaveGame(bool showConfirm)
{
    std::string aSaveGameName = GetSaveGameName(mIsPractice, mProfile->mId);
    if (!ReadBufferFromFile(aSaveGameName, &mSaveGameBuffer))
        return false;

    DataReader aReader;
    aReader.OpenMemory(mSaveGameBuffer.GetDataPtr(), mSaveGameBuffer.GetDataLen(), false);
    if (aReader.ReadLong() != gSaveGameVersion)
    {
        EraseFile(aSaveGameName);
        mSaveGameBuffer.Clear();
        return false;
    }

    std::string aVerboseLevelString;
    std::string aLevelDisplayName;
    bool aIsPractice = aReader.ReadBool();
    aReader.ReadString(aVerboseLevelString);
    aReader.ReadString(aLevelDisplayName);
    int aScore = aReader.ReadLong();
    if (showConfirm)
    {
        if (aScore > 0)
        {
            DoConfirmContinueDialog(aVerboseLevelString, aLevelDisplayName, aScore);
        }
        else
        {
            EraseFile(aSaveGameName);
            mSaveGameBuffer.Clear();
        }
    }
    else
    {
        EraseFile(aSaveGameName);
    }

    return true;
}

void CircleShootApp::StartSavedGame(bool showConfirm)
{
    MakeBoard();
    mBoard->SetStartLevel(0);
    mBoard->LoadGame(mSaveGameBuffer);
    mSaveGameBuffer.Clear();
    if (!showConfirm)
    {
        if (mProfile->mShowHelpScreen)
        {
            ShowHelpScreen();
        }
        else
        {
            DoGetReadyDialog();
        }
    }
    PlaySong(0, true, 0.01);
}

void CircleShootApp::SaveProfile()
{
    if (mProfile)
    {
        mProfile->SaveDetails();
    }
}

void CircleShootApp::LoadingThreadProc()
{
    const char *resourceGroups[5];
    resourceGroups[0] = "Register";
    resourceGroups[1] = "LoadingThread";
    resourceGroups[2] = "MainMenu";
    resourceGroups[3] = "AdventureScreen";
    resourceGroups[4] = "GauntletScreen";

    mUnk28 = 0;
    mNumLoadingThreadTasks = 0;
    int i;

    for (i = 0; i < 5; i++)
    {
        mNumLoadingThreadTasks +=
            mResourceManager->GetNumResources(resourceGroups[i]);
    }
    ++mNumLoadingThreadTasks;

    i = 0;
    for (;;)
    {
        mResourceManager->StartLoadResources(resourceGroups[i]);

        while (mResourceManager->LoadNextResource())
            ++mCompletedLoadingThreadTasks;

        if (mResourceManager->HadError() ||
            i == 0 && !Sexy::ExtractResourcesByName(mResourceManager,
                                                    resourceGroups[0]))
        {
            break;
        }

        ++mUnk28;
        ++i;

        if (i == 5)
        {
            Sexy::SharedImageRef checkBoxLine = mResourceManager->GetImage("IMAGE_DIALOG_CHECKBOXLINE");
            if (checkBoxLine.mSharedImage != NULL)
            {
                Sexy::MemoryImage *image = new MemoryImage(this);
                image->Create(20, checkBoxLine->mHeight);

                Sexy::Graphics g(image);

                for (int j = 0; j != 20; ++j)
                    g.DrawImage(checkBoxLine, j, 0);

                mResourceManager->ReplaceImage("IMAGE_DIALOG_CHECKBOXLINE", image);
            }

            Sexy::SharedImageRef ballShadow = mResourceManager->GetImage("IMAGE_BALL_SHADOW");
            if (ballShadow.mSharedImage != NULL)
            {
                Sexy::MemoryImage *image = new MemoryImage(this);
                image->Create(ballShadow->mWidth, ballShadow->mHeight);

                Sexy::Graphics g(image);
                g.SetColorizeImages(true);
                g.SetColor(Sexy::Color(0, 0, 0, 96));
                g.DrawImage(ballShadow, 0, 0);

                mResourceManager->ReplaceImage("IMAGE_BALL_SHADOW", image);
            }

            Sexy::Font *main8Font = mResourceManager->GetFont("FONT_MAIN8");
            if (main8Font != NULL)
            {
                Sexy::ImageFont *imFont = reinterpret_cast<Sexy::ImageFont *>(main8Font);
                imFont->AddTag("Outline");

                mResourceManager->ReplaceFont("FONT_MAIN8OUTLINE", imFont);
            }

            int v10 = 1;
            while (Sexy::ExtractResourcesByName(mResourceManager, resourceGroups[v10]))
            {
                if (++v10 >= 5)
                {
                    ++mCompletedLoadingThreadTasks;
                    return;
                }
            }

            break;
        }
    }

    ShowResourceError();
    mLoadingFailed = true;
}

void CircleShootApp::LoadingThreadCompleted()
{
    if (ShouldCheckForUpdate())
    {
        DoConfirmCheckForUpdatesDialog();
    }
}

void CircleShootApp::FinishStatsDialog(bool confirm)
{
    KillDialog(DialogType_Stats);
    if (mBoard == NULL)
        return;

    if (!mBoard->IsGameOver())
    {
        mBoard->Pause(false, true);
    }
    else if (mBoard->IsPracticeMode())
    {
        ShowPracticeScreen(false);
    }
    else if (!mBoard->IsWinning())
    {
        ShowAdventureScreen(false, false);
    }
    else if (mBoard->GetCurrentStage() < 12)
    {
        ShowAdventureScreen(false, true);
    }
    else
    {
        ShowCreditsScreen(true);
    }
}

void CircleShootApp::FinishConfirmQuitDialog(bool confirm)
{
    KillDialog(DialogType_ConfirmQuit);

    if (confirm)
    {
        Shutdown();
    }
}

void CircleShootApp::DoStatsDialog(bool slide, bool doCounter)
{
    if (mBoard == NULL)
        return;

    mBoard->Pause(true, true);

    StatsDialog *aDialog = new StatsDialog(mBoard, doCounter);
    SetupDialog(aDialog, 460);
    AddDialog(DialogType_Stats, aDialog);

    if (slide)
    {
        mWidgetMover->MoveWidget(
            aDialog,
            aDialog->mX,
            aDialog->mHeight + 480,
            aDialog->mX,
            aDialog->mY,
            false);
    }
}

void CircleShootApp::DoNextTempleDialog()
{
    mDidNextTempleDialog = true;
    DoDialog(DialogType_NextTemple, true, "Enter Next Temple", "You are now going to enter the next temple.\nGet Ready!", "", Dialog::BUTTONS_OK_CANCEL);
}

void CircleShootApp::FinishNextTempleDialog(bool save)
{
    KillDialog(DialogType_NextTemple);

    if (save && CheckSaveGame(false))
    {
        StartSavedGame(true);
    }
    else if (mAdventureScreen)
    {
        mWidgetManager->SetFocus(mAdventureScreen);
    }
}

void CircleShootApp::DoUserDialog()
{
    KillDialog(DialogType_User);
    UserDialog *aDialog = new UserDialog();
    SetupDialog(aDialog, 400);
    AddDialog(DialogType_User, aDialog);
}

void CircleShootApp::DoCreateUserDialog()
{
    KillDialog(DialogType_CreateUser);
    CreateUserDialog *aDialog = new CreateUserDialog(false);
    SetupDialog(aDialog, 400);
    AddDialog(DialogType_CreateUser, aDialog);
}

void CircleShootApp::DoRenameUserDialog(const std::string &theName)
{
    KillDialog(DialogType_RenameUser);
    CreateUserDialog *aDialog = new CreateUserDialog(true);
    aDialog->SetName(theName);
    SetupDialog(aDialog, 400);
    AddDialog(DialogType_RenameUser, aDialog);
}

void CircleShootApp::DoConfirmDeleteUserDialog(const std::string &theName)
{
    KillDialog(DialogType_ConfirmDeleteUser);

    std::string aText = Sexy::StrFormat("This will permanently remove '%s' from the player roster!", theName.c_str());
    DoDialog(DialogType_ConfirmDeleteUser, true, "Are You Sure?", aText, "", Dialog::BUTTONS_YES_NO);
}

void CircleShootApp::FinishUserDialog(bool confirm)
{
    UserDialog *aDialog = (UserDialog *)GetDialog(DialogType_User);
    if (!aDialog)
        return;

    if (confirm)
    {
        std::string aName = aDialog->GetSelName();
        UserProfile *aProfile = mProfileMgr->GetProfile(aName);

        if (aProfile)
        {
            mProfile = aProfile;
            mWidgetManager->MarkAllDirty();
            if (mMainMenu)
            {
                mMainMenu->SyncProfile();
            }
        }
    }

    KillDialog(DialogType_User);
}

void CircleShootApp::FinishCreateUserDialog(bool confirm)
{
    CreateUserDialog *aDialog = (CreateUserDialog *)GetDialog(DialogType_CreateUser);
    if (!aDialog)
        return;

    std::string aName = aDialog->GetName();
    if (confirm && aName.empty())
    {
        DoDialog(DialogType_NameEntry, true, "Enter Your Name",
                 "Enter your name to create a new user profile for storing high score data and games in progress.",
                 "OK", Dialog::BUTTONS_FOOTER);
        return;
    }

    if (mProfile)
    {
        if (!confirm)
        {
            KillDialog(DialogType_CreateUser);
            return;
        }
    }
    else if (!confirm || aName.empty())
    {
        DoDialog(DialogType_NameEntry, true, "Enter Your Name",
                 "Enter your name to create a new user profile for storing high score data and games in progress.",
                 "OK", Dialog::BUTTONS_FOOTER);
        return;
    }

    UserProfile *aProfile = mProfileMgr->AddProfile(aName);
    if (aProfile)
    {
        mProfileMgr->Save();
        mProfile = aProfile;
        KillDialog(DialogType_User);
        KillDialog(DialogType_CreateUser);
        mWidgetManager->MarkAllDirty();
        if (mMainMenu)
        {
            mMainMenu->SyncProfile();
        }
    }
    else
    {
        // yup, it's 10 here
        DoDialog(DialogType_NameEntry, true, "Name Conflict",
                 "The name you entered is already being used.  Please enter a unique player name.",
                 "OK", Dialog::BUTTONS_FOOTER);
    }
}

void CircleShootApp::FinishRenameUserDialog(bool confirm)
{
    if (!confirm)
    {
        KillDialog(DialogType_RenameUser);
        return;
    }

    UserDialog *aUserDialog = (UserDialog *)GetDialog(DialogType_User);
    CreateUserDialog *aCreateDialog = (CreateUserDialog *)GetDialog(DialogType_RenameUser);
    if (!aUserDialog || !aCreateDialog)
        return;

    std::string aSelName = aUserDialog->GetSelName();
    std::string aNewName = aCreateDialog->GetName();

    if (!aNewName.empty())
    {
        int cmp = stricmp(aSelName.c_str(), aNewName.c_str());
        if (mProfileMgr->RenameProfile(aSelName, aNewName))
        {
            mProfileMgr->Save();

            if (!cmp)
            {
                mProfile = mProfileMgr->GetProfile(aNewName);
            }

            aUserDialog->FinishRenameUser(aNewName);
            mWidgetManager->MarkAllDirty();
            KillDialog(DialogType_RenameUser);
        }
        else
        {
            DoDialog(DialogType_NameConflict, true, "Name Conflict",
                     "The name you entered is already being used.  Please enter a unique player name.",
                     "OK", Dialog::BUTTONS_FOOTER);
        }
    }
}

void CircleShootApp::FinishConfirmDeleteUserDialog(bool confirm)
{
    KillDialog(DialogType_ConfirmDeleteUser);
    if (!confirm)
        return;

    UserDialog *aDialog = (UserDialog *)GetDialog(DialogType_User);
    if (!aDialog)
        return;

    std::string aCurProfileName = mProfile ? mProfile->mName : "";
    std::string aSelName = aDialog->GetSelName();

    if (aSelName == aCurProfileName)
    {
        mProfile = NULL;
    }

    mProfileMgr->DeleteProfile(aSelName);
    aDialog->FinishDeleteUser();

    if (!mProfile)
    {
        std::string aNewName = aDialog->GetSelName();
        mProfile = mProfileMgr->GetProfile(aNewName);
        if (!mProfile)
        {
            mProfile = mProfileMgr->GetAnyProfile();
        }
    }

    mProfileMgr->Save();

    if (!mProfile)
    {
        DoCreateUserDialog();
    }

    mWidgetManager->MarkAllDirty();

    if (mMainMenu)
    {
        mMainMenu->SyncProfile();
    }
}

void CircleShootApp::DoRegisterDialog()
{
}

void CircleShootApp::DoCheckForUpdatesDialog()
{
}

void CircleShootApp::DoConfirmMainMenuDialog()
{
    if (mBoard && mBoard->NeedSaveGame())
    {
        Dialog *aDialog = DoDialog(DialogType_ConfirmMainMenu, true, "Leave Game?", "Your game session will be saved upon leaving. Do you want to continue?", "", Dialog::BUTTONS_OK_CANCEL);
        aDialog->mYesButton->mLabel = "Leave";

        OptionsDialog *aDialogOptions = (OptionsDialog *)GetDialog(DialogType_Options);
        if (aDialogOptions)
        {
            aDialog->Resize(aDialog->mX, aDialogOptions->mY + aDialogOptions->mHeight - aDialog->mHeight, aDialog->mWidth, aDialog->mHeight);
        }
    }
    else
    {
        ShowMainMenu();
    }
}

void CircleShootApp::FinishUpdateDialogs(int theDialogId, bool confirm)
{
}

void CircleShootApp::DoConfirmQuitDialog()
{
    DoDialog(DialogType_ConfirmQuit, true, "Quit Zuma?", "Are you sure you want to\nquit the game?", "", Dialog::BUTTONS_YES_NO);
}

void CircleShootApp::SwitchSong(int id)
{
    if (this->mLastSong == id)
        return;

    // Entering danger (song 36): switch immediately.
    // Leaving danger / other switches: keep the normal 5s debounce.
    if (id == 36 || (GetTickCount() - this->mLastSongSwitchTime) >= 5000)
        PlaySong(id, true, 0.01);
}

void CircleShootApp::DoOptionsDialog()
{
    if (mBoard != NULL)
    {
        mBoard->Pause(true, true);
    }

    Dialog *dialog = new OptionsDialog(mBoard != NULL);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_Options, dialog);
}

void CircleShootApp::DoModesDialog()
{
    Dialog *dialog = new ModesDialog();
    dialog->SetHeaderFont(Sexy::FONT_TITLE);
    dialog->SetLinesFont(Sexy::FONT_DIALOG);
    dialog->SetColor(0, Sexy::Color(203, 201, 187));
    dialog->SetColor(1, Sexy::Color(0xD5E520));
    dialog->Resize(0, 0, CIRCLE_WINDOW_WIDTH, CIRCLE_WINDOW_HEIGHT);
    SetupButton(dialog->mYesButton, 3);
    SetupButton(dialog->mNoButton, 3);
    AddDialog(DialogType_Modes, dialog);
}

void CircleShootApp::DoColorsBanDialog()
{
    if (GetDialog(DialogType_ColorsBan) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    bool initialBanned[MAX_BALL_COLORS];
    bool initialRandom = mColorsBanRandom;
    int initialCount = mColorsBanRandomCount;
    if (modes != NULL)
    {
        modes->GetBannedColors(initialBanned);
        initialRandom = modes->GetColorsBanRandom();
        initialCount = modes->GetColorsBanRandomCount();
    }
    else
    {
        for (int i = 0; i < MAX_BALL_COLORS; i++)
            initialBanned[i] = mBannedColors[i];
    }

    Dialog *dialog = new ColorsBanDialog(initialBanned, initialRandom, initialCount);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_ColorsBan, dialog);
}

void CircleShootApp::DoUnpoweredDialog()
{
    if (GetDialog(DialogType_Unpowered) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    bool initialDisabled[PowerType_Max];
    if (modes != NULL)
        modes->GetDisabledPowerUps(initialDisabled);
    else
    {
        for (int i = 0; i < PowerType_Max; i++)
            initialDisabled[i] = mDisabledPowerUps[i];
    }

    Dialog *dialog = new UnpoweredDialog(initialDisabled);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_Unpowered, dialog);
}

void CircleShootApp::DoSonicDialog()
{
    if (GetDialog(DialogType_Sonic) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    float initialMult = (modes != NULL) ? modes->GetChainSpeedMultiplier() : mChainSpeedMultiplier;

    Dialog *dialog = new SonicDialog(initialMult);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_Sonic, dialog);
}

void CircleShootApp::DoMovingHoleDialog()
{
    if (GetDialog(DialogType_MovingHole) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    int initialSpeed = (modes != NULL) ? modes->GetMovingHoleSpeed() : mMovingHoleSpeed;

    Dialog *dialog = new MovingHoleDialog(initialSpeed);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_MovingHole, dialog);
}

void CircleShootApp::DoChainCountDialog()
{
    if (GetDialog(DialogType_ChainCount) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    int initialThreshold = (modes != NULL) ? modes->GetChainBonusThreshold() : mChainBonusThreshold;
    if (initialThreshold < 1 || initialThreshold > 20)
        initialThreshold = 5;
    bool disableBonus = (modes != NULL) ? modes->GetChainBonusDisabled() : mChainBonusDisabled;

    Dialog *dialog = new ChainCountDialog(initialThreshold, disableBonus);
    SetupDialog(dialog, 420);
    AddDialog(DialogType_ChainCount, dialog);
}

void CircleShootApp::DoColorShiftDialog()
{
    if (GetDialog(DialogType_ColorShift) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    float initialHz = (modes != NULL) ? modes->GetColorShiftHz() : mColorShiftHz;
    bool randomMap = (modes != NULL) ? modes->GetColorShiftRandom() : mColorShiftRandom;
    int initialMap[MAX_BALL_COLORS];
    bool initialEnabled[MAX_BALL_COLORS];
    if (modes != NULL)
    {
        modes->GetColorShiftMap(initialMap);
        modes->GetColorShiftEnabled(initialEnabled);
    }
    else
    {
        for (int i = 0; i < MAX_BALL_COLORS; i++)
        {
            initialMap[i] = mColorShiftMap[i];
            initialEnabled[i] = mColorShiftEnabled[i];
        }
    }

    Dialog *dialog = new ColorShiftDialog(initialHz, initialMap, initialEnabled, randomMap);
    SetupDialog(dialog, 420);
    AddDialog(DialogType_ColorShift, dialog);
}

void CircleShootApp::DoColorShiftTargetDialog(int theSrcColor, int initialDest)
{
    if (GetDialog(DialogType_ColorShiftTarget) != NULL)
        return;

    Dialog *dialog = new ColorShiftTargetDialog(theSrcColor, initialDest);
    SetupDialog(dialog, 380);
    AddDialog(DialogType_ColorShiftTarget, dialog);
}

void CircleShootApp::DoInvisibleDialog()
{
    if (GetDialog(DialogType_Invisible) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    float duration = (modes != NULL) ? modes->GetInvisibleDurationSec() : mInvisibleDurationSec;
    float interval = (modes != NULL) ? modes->GetInvisibleIntervalSec() : mInvisibleIntervalSec;
    int percent = (modes != NULL) ? modes->GetInvisiblePercent() : mInvisiblePercent;

    Dialog *dialog = new InvisibleDialog(duration, interval, percent);
    SetupDialog(dialog, 420);
    AddDialog(DialogType_Invisible, dialog);
}

void CircleShootApp::DoKillerBallDialog()
{
    if (GetDialog(DialogType_KillerBall) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    float interval = (modes != NULL) ? modes->GetKillerBallIntervalSec() : mKillerBallIntervalSec;
    float flight = (modes != NULL) ? modes->GetKillerBallFlightSec() : mKillerBallFlightSec;

    Dialog *dialog = new KillerBallDialog(interval, flight);
    SetupDialog(dialog, 400);
    AddDialog(DialogType_KillerBall, dialog);
}

void CircleShootApp::DoShootSpeedDialog()
{
    if (GetDialog(DialogType_ShootSpeed) != NULL)
        return;

    ModesDialog *modes = (ModesDialog *)GetDialog(DialogType_Modes);
    float multiplier = (modes != NULL) ? modes->GetShootSpeedMultiplier() : mShootSpeedMultiplier;
    bool instant = (modes != NULL) ? modes->GetShootSpeedInstant() : (mShootSpeedInstant || mLightSpeedMode);

    Dialog *dialog = new ShootSpeedDialog(multiplier, instant);
    SetupDialog(dialog, 420);
    AddDialog(DialogType_ShootSpeed, dialog);
}

void CircleShootApp::DoModeHelpDialog(const std::string &theTitle, const std::string &theDescription)
{
    KillDialog(DialogType_ModeHelp);
    Dialog *dialog = DoDialog(DialogType_ModeHelp, true, theTitle, theDescription, "OK", Dialog::BUTTONS_FOOTER);
    SetupDialog(dialog, 380);
}

void CircleShootApp::DoConfirmContinueDialog(const std::string &theVerboseLevelString, const std::string &theDisplayName, int theScore)
{
    std::string aScore = Sexy::StrFormat("%s (%s)\r\nScore: %d\r\n", theVerboseLevelString.c_str(), theDisplayName.c_str(), theScore);
    std::string aText = "Your game was saved when you quit.  If you do not continue now, the game will be lost.\r\n"
                        "\r\n"
                        "Do you want to continue your last game?"
                        "\r\n"
                        "\r\n" +
                        aScore;

    Dialog *aDialog = new ConfirmContinueDialog(aText);
    SetupDialog(aDialog, 348);
    AddDialog(DialogType_ConfirmContinue, aDialog);
}

void CircleShootApp::DoGetReadyDialog()
{
    if (mDialogMap.empty())
    {
        DoDialog(DialogType_GetReady, true, "GO", "", "Get Ready!", Dialog::BUTTONS_FOOTER);
        if (mBoard)
        {
            mBoard->Pause(true, true);
        }

        mBoard->SetShowBallsDuringPause(true);
        mBoard->SetFullPauseFade();
    }
}

void CircleShootApp::PlaySong(int id, bool fade, double fadeSpeed)
{
    if (this->mLastSong != id)
    {
        int prevSong = mSongId;

        if (fade)
        {
            mSongId = (mSongId + 1) % 2;
            mMusicInterface->FadeOut(prevSong, true, fadeSpeed);
            mMusicInterface->FadeIn(mSongId, id, fadeSpeed * 0.5);
        }
        else
        {
            mMusicInterface->PlayMusic(mSongId, id);
        }

        mLastSong = id;
        mLastSongSwitchTime = GetTickCount();
    }
}

void CircleShootApp::FinishGetReadyDialog()
{
    KillDialog(DialogType_GetReady);
    if (mBoard)
    {
        mBoard->Pause(false, true);
    }
}

void CircleShootApp::FinishConfirmContinueDialog(bool startGame)
{
    KillDialog(DialogType_ConfirmContinue);

    std::string aSaveGameName = GetSaveGameName(mIsPractice, mProfile->mId);
    EraseFile(aSaveGameName);

    if (startGame)
    {
        StartSavedGame(true);
    }
    else if (mIsPractice)
    {
        ShowPracticeScreen(false);
    }
    else
    {
        ShowAdventureScreen(false, false);
    }
}

void CircleShootApp::FinishConfirmContinueDialogCancel()
{
    KillDialog(DialogType_ConfirmContinue);
    mSaveGameBuffer.Clear();
}

void CircleShootApp::FinishOptionsDialog(bool saveSettings)
{
    Sexy::OptionsDialog *dialog = (OptionsDialog *)GetDialog(DialogType_Options);
    if (dialog != NULL)
    {
        if (saveSettings)
        {
            bool fullscreen = dialog->mFullScreenCheckbox->IsChecked();
            bool acceleration = dialog->m3DAccelCheckbox->IsChecked();
            SwitchScreenMode(fullscreen != true, acceleration);
            bool cursorsEnabled = dialog->mCustomCursorsCheckbox->IsChecked();
            EnableCustomCursors(cursorsEnabled);
            ClearUpdateBacklog();
        }

        KillDialog(0);
        if (mBoard)
        {
            mBoard->Pause(false, true);
        }
    }
}

void CircleShootApp::FinishModesDialog(bool apply)
{
    ModesDialog *dialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (dialog == NULL)
        return;

    // Copy session state before any dialog teardown.
    bool colorsBan = false;
    bool unpowered = false;
    bool noSwap = false;
    bool sonic = false;
    bool uglyChain = false;
    bool movingHole = false;
    bool comboless = false;
    bool gapFree = false;
    bool chainCount = false;
    bool bankrupt = false;
    bool colorShift = false;
    bool invisible = false;
    bool baseMinimum = false;
    bool autoAdvance = false;
    bool killerBall = false;
    bool shootSpeed = false;
    bool bannedColors[MAX_BALL_COLORS];
    bool disabledPowerUps[PowerType_Max];
    float chainSpeed = 1.0f;
    int movingHoleSpeed = 20;
    int chainBonusThreshold = 5;
    bool chainBonusDisabled = false;
    float colorShiftHz = 3.0f;
    int colorShiftMap[MAX_BALL_COLORS];
    bool colorShiftRandom = true;
    bool colorShiftEnabled[MAX_BALL_COLORS];
    float invisibleDurationSec = 3.0f;
    float invisibleIntervalSec = 5.0f;
    int invisiblePercent = 50;
    float killerBallIntervalSec = 10.0f;
    float killerBallFlightSec = 10.0f;
    float shootSpeedMultiplier = 1.0f;
    bool shootSpeedInstant = false;
    bool colorsBanRandom = false;
    int colorsBanRandomCount = 1;
    for (int i = 0; i < MAX_BALL_COLORS; i++)
    {
        colorShiftMap[i] = (i + 1) % MAX_BALL_COLORS;
        colorShiftEnabled[i] = true;
    }

    if (apply)
    {
        colorsBan = dialog->IsColorsBanSelected();
        dialog->GetBannedColors(bannedColors);
        colorsBanRandom = dialog->GetColorsBanRandom();
        colorsBanRandomCount = dialog->GetColorsBanRandomCount();
        unpowered = dialog->IsUnpoweredSelected();
        dialog->GetDisabledPowerUps(disabledPowerUps);
        noSwap = dialog->IsNoSwapSelected();
        sonic = dialog->IsSonicSelected();
        uglyChain = dialog->IsUglyChainSelected();
        movingHole = dialog->IsMovingHoleSelected();
        comboless = dialog->IsCombolessSelected();
        gapFree = dialog->IsGapFreeSelected();
        chainCount = dialog->IsChainCountSelected();
        bankrupt = dialog->IsBankruptSelected();
        colorShift = dialog->IsColorShiftSelected();
        invisible = dialog->IsInvisibleSelected();
        baseMinimum = dialog->IsBaseMinimumSelected();
        autoAdvance = dialog->IsAutoAdvanceSelected();
        killerBall = dialog->IsKillerBallSelected();
        shootSpeed = dialog->IsShootSpeedSelected();
        chainSpeed = dialog->GetChainSpeedMultiplier();
        movingHoleSpeed = dialog->GetMovingHoleSpeed();
        chainBonusThreshold = dialog->GetChainBonusThreshold();
        chainBonusDisabled = dialog->GetChainBonusDisabled();
        colorShiftHz = dialog->GetColorShiftHz();
        dialog->GetColorShiftMap(colorShiftMap);
        dialog->GetColorShiftEnabled(colorShiftEnabled);
        colorShiftRandom = dialog->GetColorShiftRandom();
        invisibleDurationSec = dialog->GetInvisibleDurationSec();
        invisibleIntervalSec = dialog->GetInvisibleIntervalSec();
        invisiblePercent = dialog->GetInvisiblePercent();
        killerBallIntervalSec = dialog->GetKillerBallIntervalSec();
        killerBallFlightSec = dialog->GetKillerBallFlightSec();
        shootSpeedMultiplier = dialog->GetShootSpeedMultiplier();
        shootSpeedInstant = dialog->GetShootSpeedInstant();
    }

    dialog->PrepareClose();

    KillDialog(DialogType_ColorsBan);
    KillDialog(DialogType_Unpowered);
    KillDialog(DialogType_Sonic);
    KillDialog(DialogType_MovingHole);
    KillDialog(DialogType_ChainCount);
    KillDialog(DialogType_ColorShiftTarget);
    KillDialog(DialogType_ColorShift);
    KillDialog(DialogType_Invisible);
    KillDialog(DialogType_KillerBall);
    KillDialog(DialogType_ShootSpeed);
    KillDialog(DialogType_ModeHelp);
    KillDialog(DialogType_Modes);

    if (apply)
    {
        mColorsBanMode = colorsBan;
        for (int i = 0; i < MAX_BALL_COLORS; i++)
            mBannedColors[i] = bannedColors[i];
        mColorsBanRandom = colorsBanRandom;
        mColorsBanRandomCount = colorsBanRandomCount;
        RollRandomBannedColors();

        mUnpoweredMode = unpowered;
        for (int i = 0; i < PowerType_Max; i++)
            mDisabledPowerUps[i] = disabledPowerUps[i];

        mNoSwapMode = noSwap;
        mSonicMode = sonic;
        mMachineGunMode = false;
        mUglyChainMode = uglyChain;
        mMovingHoleMode = movingHole;
        mCombolessMode = comboless;
        mGapFreeMode = gapFree;
        mChainCountMode = chainCount;
        mBankruptMode = bankrupt;
        mColorShiftMode = colorShift;
        mInvisibleMode = invisible;
        mBaseMinimumMode = baseMinimum;
        mAutoAdvanceMode = autoAdvance;
        mChainSpeedMultiplier = chainSpeed;
        mMovingHoleSpeed = movingHoleSpeed;
        mChainBonusThreshold = chainBonusThreshold;
        mChainBonusDisabled = chainBonusDisabled;
        mColorShiftHz = colorShiftHz;
        for (int i = 0; i < MAX_BALL_COLORS; i++)
        {
            mColorShiftMap[i] = colorShiftMap[i];
            mColorShiftEnabled[i] = colorShiftEnabled[i];
        }
        mColorShiftRandom = colorShiftRandom;
        mInvisibleDurationSec = invisibleDurationSec;
        mInvisibleIntervalSec = invisibleIntervalSec;
        mInvisiblePercent = invisiblePercent;
        mKillerBallMode = killerBall;
        mKillerBallIntervalSec = killerBallIntervalSec;
        mKillerBallFlightSec = killerBallFlightSec;
        mShootSpeedMode = shootSpeed;
        mShootSpeedMultiplier = shootSpeedMultiplier;
        if (mShootSpeedMultiplier < 0.1f)
            mShootSpeedMultiplier = 0.1f;
        if (mShootSpeedMultiplier > 5.0f)
            mShootSpeedMultiplier = 5.0f;
        mShootSpeedInstant = shootSpeed && shootSpeedInstant;
        mLightSpeedMode = mShootSpeedMode && mShootSpeedInstant;
    }
}

void CircleShootApp::FinishColorsBanDialog(bool apply)
{
    ColorsBanDialog *colorsDialog = (ColorsBanDialog *)GetDialog(DialogType_ColorsBan);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (colorsDialog == NULL)
        return;

    if (apply)
    {
        bool banned[MAX_BALL_COLORS];
        colorsDialog->GetBannedColors(banned);

        if (modesDialog != NULL)
        {
            modesDialog->SetBannedColors(banned);
            modesDialog->SetColorsBanRandom(colorsDialog->GetRandom());
            modesDialog->SetColorsBanRandomCount(colorsDialog->GetRandomCount());
            modesDialog->SetColorsBanSelected(true);
        }
    }

    KillDialog(DialogType_ColorsBan);
}

void CircleShootApp::FinishUnpoweredDialog(bool apply)
{
    UnpoweredDialog *powerDialog = (UnpoweredDialog *)GetDialog(DialogType_Unpowered);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (powerDialog == NULL)
        return;

    if (apply)
    {
        bool disabled[PowerType_Max];
        powerDialog->GetDisabledPowerUps(disabled);

        if (modesDialog != NULL)
        {
            modesDialog->SetDisabledPowerUps(disabled);
            modesDialog->SetUnpoweredSelected(true);
        }
    }

    KillDialog(DialogType_Unpowered);
}

void CircleShootApp::FinishSonicDialog(bool apply)
{
    SonicDialog *sonicDialog = (SonicDialog *)GetDialog(DialogType_Sonic);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (sonicDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetChainSpeedMultiplier(sonicDialog->GetChainSpeedMultiplier());
            modesDialog->SetSonicSelected(true);
        }
    }

    KillDialog(DialogType_Sonic);
}

void CircleShootApp::FinishMovingHoleDialog(bool apply)
{
    MovingHoleDialog *holeDialog = (MovingHoleDialog *)GetDialog(DialogType_MovingHole);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (holeDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetMovingHoleSpeed(holeDialog->GetSpeed());
            modesDialog->SetMovingHoleSelected(true);
        }
    }

    KillDialog(DialogType_MovingHole);
}

void CircleShootApp::FinishChainCountDialog(bool apply)
{
    ChainCountDialog *chainDialog = (ChainCountDialog *)GetDialog(DialogType_ChainCount);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (chainDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetChainBonusThreshold(chainDialog->GetThreshold());
            modesDialog->SetChainBonusDisabled(chainDialog->GetDisableBonus());
            modesDialog->SetChainCountSelected(true);
        }
    }

    KillDialog(DialogType_ChainCount);
}

void CircleShootApp::FinishColorShiftDialog(bool apply)
{
    KillDialog(DialogType_ColorShiftTarget);

    ColorShiftDialog *shiftDialog = (ColorShiftDialog *)GetDialog(DialogType_ColorShift);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (shiftDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            int map[MAX_BALL_COLORS];
            bool enabled[MAX_BALL_COLORS];
            shiftDialog->GetColorMap(map);
            shiftDialog->GetEnabledColors(enabled);
            modesDialog->SetColorShiftHz(shiftDialog->GetHz());
            modesDialog->SetColorShiftMap(map);
            modesDialog->SetColorShiftEnabled(enabled);
            modesDialog->SetColorShiftRandom(shiftDialog->GetRandomMap());
            modesDialog->SetColorShiftSelected(true);
        }
    }

    KillDialog(DialogType_ColorShift);
}

void CircleShootApp::FinishInvisibleDialog(bool apply)
{
    InvisibleDialog *invisibleDialog = (InvisibleDialog *)GetDialog(DialogType_Invisible);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (invisibleDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetInvisibleDurationSec(invisibleDialog->GetDurationSec());
            modesDialog->SetInvisibleIntervalSec(invisibleDialog->GetIntervalSec());
            modesDialog->SetInvisiblePercent(invisibleDialog->GetPercent());
            modesDialog->SetInvisibleSelected(true);
        }
    }

    KillDialog(DialogType_Invisible);
}

void CircleShootApp::FinishKillerBallDialog(bool apply)
{
    KillerBallDialog *killerDialog = (KillerBallDialog *)GetDialog(DialogType_KillerBall);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (killerDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetKillerBallIntervalSec(killerDialog->GetIntervalSec());
            modesDialog->SetKillerBallFlightSec(killerDialog->GetFlightSec());
            modesDialog->SetKillerBallSelected(true);
        }
    }

    KillDialog(DialogType_KillerBall);
}

void CircleShootApp::FinishShootSpeedDialog(bool apply)
{
    ShootSpeedDialog *speedDialog = (ShootSpeedDialog *)GetDialog(DialogType_ShootSpeed);
    ModesDialog *modesDialog = (ModesDialog *)GetDialog(DialogType_Modes);
    if (speedDialog == NULL)
        return;

    if (apply)
    {
        if (modesDialog != NULL)
        {
            modesDialog->SetShootSpeedMultiplier(speedDialog->GetMultiplier());
            modesDialog->SetShootSpeedInstant(speedDialog->GetInstant());
            modesDialog->SetShootSpeedSelected(true);
        }
    }

    KillDialog(DialogType_ShootSpeed);
}

void CircleShootApp::FinishColorShiftTargetDialog(bool apply)
{
    ColorShiftTargetDialog *targetDialog =
        (ColorShiftTargetDialog *)GetDialog(DialogType_ColorShiftTarget);
    ColorShiftDialog *shiftDialog = (ColorShiftDialog *)GetDialog(DialogType_ColorShift);
    if (targetDialog == NULL)
        return;

    int src = targetDialog->GetSrcColor();
    int dest = targetDialog->GetSelectedDest();
    KillDialog(DialogType_ColorShiftTarget);

    if (shiftDialog != NULL)
        shiftDialog->OnTargetPicked(src, dest, apply);
}

bool CircleShootApp::IsColorBanned(int theColor) const
{
    if (!mColorsBanMode || theColor < 0 || theColor >= MAX_BALL_COLORS)
        return false;

    if (mColorsBanRandom)
        return mActiveBannedColors[theColor];

    return mBannedColors[theColor];
}

void CircleShootApp::RollRandomBannedColors(int theNumColors)
{
    for (int i = 0; i < MAX_BALL_COLORS; i++)
        mActiveBannedColors[i] = false;

    if (!mColorsBanMode)
        return;

    if (!mColorsBanRandom)
    {
        for (int i = 0; i < MAX_BALL_COLORS; i++)
            mActiveBannedColors[i] = mBannedColors[i];
        return;
    }

    // Only ban colors that can actually appear on this level's chains.
    int poolSize = theNumColors;
    if (poolSize < 2)
        poolSize = 4;
    if (poolSize > MAX_BALL_COLORS)
        poolSize = MAX_BALL_COLORS;

    int count = mColorsBanRandomCount;
    if (count < 1)
        count = 1;
    if (count > 5)
        count = 5;
    if (count > poolSize - 1)
        count = poolSize - 1;
    if (count < 1)
        return;

    // Shuffle the in-level color pool and take the first `count`.
    int order[MAX_BALL_COLORS];
    for (int i = 0; i < poolSize; i++)
        order[i] = i;
    for (int i = poolSize - 1; i > 0; i--)
    {
        int j = Sexy::AppRand() % (i + 1);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }
    for (int i = 0; i < count; i++)
        mActiveBannedColors[order[i]] = true;
}

bool CircleShootApp::IsPowerUpDisabled(int thePowerType) const
{
    if (!mUnpoweredMode || thePowerType < 0 || thePowerType >= PowerType_Max)
        return false;

    return mDisabledPowerUps[thePowerType];
}

float CircleShootApp::GetChainSpeedMultiplier() const
{
    if (!mSonicMode)
        return 1.0f;

    return mChainSpeedMultiplier;
}

void CircleShootApp::FinishConfirmMainMenuDialog(bool mainMenu)
{
    KillDialog(DialogType_ConfirmMainMenu);
    if (mainMenu)
    {
        ShowMainMenu();
    }
}

bool CircleShootApp::CheckYesNoButton(int theButton)
{
    if ((theButton % 10000 - 2000) < 1000)
    {
        switch (theButton)
        {
        case 2000:
            FinishOptionsDialog(true);
            return true;
        case 2001:
            // DoCheckForUpdatesDialog();
            return true;
        case 2002:
        case 12002:
        case 22002:
            // FinishUpdateDialogs(theButton - 2000, true);
            return true;
        case 2007:
            FinishUserDialog(true);
            return true;
        case 2008:
            FinishCreateUserDialog(true);
            return true;
        case 2009:
            FinishRenameUserDialog(true);
            return true;
        case 2012:
            FinishConfirmDeleteUserDialog(true);
            return true;
        case 2013:
            FinishConfirmContinueDialog(true);
            return true;
        case 2014:
            FinishStatsDialog(true);
            return true;
        case 2015:
            FinishGetReadyDialog();
            return true;
        case 2016:
            FinishNextTempleDialog(true);
            return true;
        case 2017:
            // FinishRegisterDialog(true);
            return true;
        case 2020:
            FinishConfirmMainMenuDialog(true);
            return true;
        case 2021:
            FinishConfirmQuitDialog(true);
            return true;
        case 2022:
            // FinishNeedRegisterDialog(true);
            return true;
        case 2023:
            FinishModesDialog(true);
            return true;
        case 2024:
            FinishColorsBanDialog(true);
            return true;
        case 2025:
            FinishUnpoweredDialog(true);
            return true;
        case 2026:
            FinishSonicDialog(true);
            return true;
        case 2028:
            FinishMovingHoleDialog(true);
            return true;
        case 2030:
            FinishChainCountDialog(true);
            return true;
        case 2031:
            FinishColorShiftDialog(true);
            return true;
        case 2032:
            FinishColorShiftTargetDialog(true);
            return true;
        case 2033:
            FinishInvisibleDialog(true);
            return true;
        case 2034:
            FinishKillerBallDialog(true);
            return true;
        case 2035:
            FinishShootSpeedDialog(true);
            return true;
        default:
            KillDialog(theButton - 2000);
            return true;
        }
    }
    else if ((theButton % 10000 - 3000) < 1000)
    {
        switch (theButton)
        {
        case 3002:
        case 13002:
        case 23002:
            // FinishUpdateDialogs(theButton - 3000, true);
            return true;
        case 3007:
            FinishUserDialog(false);
            return true;
        case 3008:
            FinishCreateUserDialog(false);
            return true;
        case 3009:
            FinishRenameUserDialog(false);
            return true;
        case 3012:
            FinishConfirmDeleteUserDialog(false);
            return true;
        case 3013:
            FinishConfirmContinueDialog(false);
            return true;
        case 3014:
            FinishStatsDialog(false);
            return true;
        case 3016:
            FinishNextTempleDialog(false);
            return true;
        case 3017:
            // FinishRegisterDialog(false);
            return true;
        case 3020:
            FinishConfirmMainMenuDialog(false);
            return true;
        case 3021:
            FinishConfirmQuitDialog(false);
            return true;
        case 3022:
            // FinishNeedRegisterDialog(false);
            return true;
        case 3023:
            FinishModesDialog(false);
            return true;
        case 3024:
            FinishColorsBanDialog(false);
            return true;
        case 3025:
            FinishUnpoweredDialog(false);
            return true;
        case 3026:
            FinishSonicDialog(false);
            return true;
        case 3028:
            FinishMovingHoleDialog(false);
            return true;
        case 3030:
            FinishChainCountDialog(false);
            return true;
        case 3031:
            FinishColorShiftDialog(false);
            return true;
        case 3032:
            FinishColorShiftTargetDialog(false);
            return true;
        case 3033:
            FinishInvisibleDialog(false);
            return true;
        case 3034:
            FinishKillerBallDialog(false);
            return true;
        case 3035:
            FinishShootSpeedDialog(false);
            return true;
        default:
            KillDialog(theButton - 3000);
            return true;
        }
    }

    return false;
}

void CircleShootApp::ShowMainMenu()
{
    if (mPracticeScreen != NULL)
    {
        mWidgetMover->MoveWidget(mPracticeScreen, mPracticeScreen->mX, mPracticeScreen->mY, -mPracticeScreen->mWidth, mPracticeScreen->mY, true);
        mPracticeScreen = NULL;
    }

    if (mAdventureScreen != NULL)
    {
        mWidgetMover->MoveWidget(mAdventureScreen, mAdventureScreen->mX, mAdventureScreen->mY, -mAdventureScreen->mWidth, mAdventureScreen->mY, true);
        mAdventureScreen = NULL;
    }

    if (mMoreGamesScreen != NULL)
    {
        mWidgetMover->MoveWidget(mMoreGamesScreen, mMoreGamesScreen->mX, mMoreGamesScreen->mY, -mMoreGamesScreen->mWidth, mMoreGamesScreen->mY, true);
        mMoreGamesScreen = NULL;
    }

    FinishOptionsDialog(true);
    CleanupWidgets();

    mMainMenu = new MainMenu();
    mMainMenu->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mMainMenu);
    mWidgetManager->BringToBack(mMainMenu);
    mWidgetManager->SetFocus(mMainMenu);

    if (!mProfile)
    {
        DoCreateUserDialog();
    }

    PlaySong(28, true, 0.01);
    ClearUpdateBacklog();
}

void CircleShootApp::ShowAdventureScreen(bool fromMenu, bool revealTemple)
{
    mIsPractice = false;
    if (fromMenu && CheckSaveGame(true))
        return;

    MainMenu *aMainMenu = mMainMenu;
    mWidgetMover->DelayDeleteWidget(mMainMenu);
    mMainMenu = NULL;

    CleanupWidgets();

    mAdventureScreen = new AdventureScreen();
    mAdventureScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mAdventureScreen);
    mWidgetManager->SetFocus(mAdventureScreen);

    if (revealTemple)
    {
        mAdventureScreen->RevealTemple(75, mProfile->mMaxStage / 3 + 1);
        mAdventureScreen->SetStartNextTempleOnRevel(true);
    }

    if (aMainMenu)
    {
        mWidgetMover->MoveWidget(mAdventureScreen, -mAdventureScreen->mWidth, 0, 0, 0, false);
    }

    PlaySong(32, true, 0.01);
    ClearUpdateBacklog();
}

void CircleShootApp::ShowPracticeScreen(bool fromMenu)
{
    mIsPractice = true;
    if (fromMenu && CheckSaveGame(true))
        return;

    MainMenu *aMainMenu = mMainMenu;
    mWidgetMover->DelayDeleteWidget(mMainMenu);
    mMainMenu = NULL;

    CleanupWidgets();

    mPracticeScreen = new PracticeScreen();
    mPracticeScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mPracticeScreen);
    mWidgetManager->SetFocus(mPracticeScreen);

    if (aMainMenu)
    {
        mWidgetMover->MoveWidget(mPracticeScreen, -mPracticeScreen->mWidth, 0, 0, 0, false);
    }

    PlaySong(34, true, 0.01);
    ClearUpdateBacklog();
}

void CircleShootApp::ShowCreditsScreen(bool happyEnd)
{
    CleanupWidgets();
    mCreditsScreen = new CreditsScreen(happyEnd);
    mCreditsScreen->Resize(0, 0, mWidth, mHeight);
    mWidgetManager->AddWidget(mCreditsScreen);
    mWidgetManager->SetFocus(mCreditsScreen);
    PlaySong(0, true, 0.01);
}

void CircleShootApp::ShowMoreGamesScreen()
{
}

void CircleShootApp::EndHelpScreen()
{
    if (mHelpScreen)
    {
        mWidgetMover->SafeDeleteWidget(mHelpScreen);
        mHelpScreen = NULL;
    }

    if (mBoard)
    {
        mWidgetManager->SetFocus(mBoard);

        if (mBoard)
        {
            mBoard->Pause(false, true);
        }

        if (mBoard->IsSavedGame())
        {
            DoGetReadyDialog();
        }
    }
}

void CircleShootApp::ReturnToMainMenu()
{
    ShowMainMenu();
}