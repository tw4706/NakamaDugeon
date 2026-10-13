#include "SceneManager.h"
#include "Scene.h"
#include "Game.h"
#include "Effects.h"

SceneManager::SceneManager()
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::ChangeScene(std::shared_ptr<Scene> scene)
{
	scene_ = scene;
	Effects::GetInstance().Clear();
	scene_->Init();
}

void SceneManager::ChangeSceneWithFade(std::shared_ptr<Scene> scene)
{
	if (nextScene_) return;
	nextScene_ = scene;
	fade_.StartFadeOut();
}

void SceneManager::Update()
{
	fade_.Update(Game::kFrameTime);

	if (fade_.IsFadeOutEnd() && nextScene_)
	{
		auto next = nextScene_;
		nextScene_ = nullptr;
		ChangeScene(next);
		fade_.StartFadeIn();
	}

	if (nextScene_) return;

	//更新中に切り替わっても消えないように保持
	auto current = scene_;
	if (current) current->Update();
}

void SceneManager::Draw()
{
	if (scene_) scene_->Draw();
}

void SceneManager::DrawFade() const
{
	fade_.Draw();
}
