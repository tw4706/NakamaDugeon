#include "Application.h"
#include "SceneManager.h"
#include "TitleScene.h"
#include "MonsterData.h"
#include "EquipData.h"
#include "SaveData.h"
#include "UpdateLog.h"
#include "Game.h"
#include "GameTime.h"
#include "Input.h"
#include "Sound.h"
#include "Font.h"
#include "Sprite.h"
#include "AuraRenderer.h"
#include "PixelArt.h"
#include "Effects.h"
#include "EffekseerForDXLib.h"
#include<DxLib.h>
#include<memory>

Application::Application()
{
}

Application::~Application()
{
}

Application& Application::GetInstance()
{
	static Application instance;
	return instance;
}

bool Application::Init()
{
	//データとセーブは描画の準備より先に読む(画面モードの設定のため)
	if (!MonsterTable::Load("data/Monster.csv") || !EquipTable::Load("data/Equipment.csv"))
	{
		MessageBox(nullptr, L"data フォルダの CSV が よめません", L"エラー", MB_OK);
		return false;
	}
	UpdateLog::Load("data/Updates.csv");
	SaveManager::GetInstance().LoadAll();
	const Settings& settings = SaveManager::GetInstance().GetSettings();

	ChangeWindowMode(settings.isFullScreen ? FALSE : TRUE);
	SetMainWindowText(L"まものつかいの ぼうけん");
	SetGraphMode(Game::kScreenWidth, Game::kScreenHeight, Game::kColorBitNum);
	SetOutApplicationLogValidFlag(FALSE);
	SetAlwaysRunFlag(TRUE);

	//Effekseerを使うためDirectX11にする
	SetUseDirect3DVersion(DX_DIRECT3D_11);

	if (DxLib_Init() == -1)
	{
		return false;
	}

	SetDrawScreen(DX_SCREEN_BACK);

	//------------------------------//
	// エフェクトの初期化
	//------------------------------//
	if (Effkseer_Init(8000) == -1)
	{
		DxLib_End();
		return false;
	}
	SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
	Effekseer_SetGraphicsDeviceLostCallbackFunctions();
	Effekseer_Set2DSetting(Game::kScreenWidth, Game::kScreenHeight);

	gameScreen_ = MakeScreen(Game::kScreenWidth, Game::kScreenHeight, FALSE);

	Font::Init();
	if (!Sprite::Init())
	{
		MessageBox(nullptr, L"data/gfx の ドット絵が よめません", L"エラー", MB_OK);
		DxLib_End();
		return false;
	}
	Sound::GetInstance().Init();
	Sound::GetInstance().SetVolume(settings.BgmLevel(), settings.SeLevel());

	//しんえんの オーラと キャラの ドット絵(つかえなければ かんたんな ひょうじ)
	AuraRenderer::Init();
	PixelArt::Init();

	return true;
}

void Application::Run()
{
	SceneManager sceneManager;
	sceneManager.ChangeScene(std::make_shared<TitleScene>(sceneManager));

	while (ProcessMessage() != -1)
	{
		LONGLONG start = GetNowHiPerformanceCount();

		GameTime::SetDeltaTime(Game::kFrameTime);
		Input::GetInstance().Update();

		sceneManager.Update();
		Effects::GetInstance().Update(Game::kFrameTime);
		SaveManager::GetInstance().AddPlayTime(Game::kFrameTime);
		UpdateEffekseer2D();

		//ゲーム画面に描く
		SetDrawScreen(gameScreen_);
		ClearDrawScreen();
		sceneManager.Draw();
		DrawEffekseer2D();

		//揺れを付けて裏画面に描く
		SetDrawScreen(DX_SCREEN_BACK);
		ClearDrawScreen();
		const Vec2 shake = Effects::GetInstance().GetShakeOffset();
		DrawGraph(static_cast<int>(shake.x), static_cast<int>(shake.y), gameScreen_, FALSE);
		Effects::GetInstance().DrawFlash();
		sceneManager.DrawFade();

		if (CheckHitKey(KEY_INPUT_ESCAPE) || Game::isExitRequested)
		{
			break;
		}


		ScreenFlip();

		//60FPS固定
		while (GetNowHiPerformanceCount() - start < 16667)
		{
		}
	}
}

void Application::Terminate()
{
	Sound::GetInstance().Terminate();
	Font::Terminate();
	Sprite::Terminate();
	AuraRenderer::Terminate();
	PixelArt::Terminate();
	DeleteGraph(gameScreen_);
	Effkseer_End();
	DxLib_End();
}
