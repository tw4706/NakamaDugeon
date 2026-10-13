#pragma once
#include"Fade.h"
#include<memory>

class Scene;
class SceneManager
{
public:
	SceneManager();
	virtual ~SceneManager();

	SceneManager(const SceneManager&) = delete;
	SceneManager& operator=(const SceneManager&) = delete;

	/// <summary>
	/// すぐに切り替える
	/// </summary>
	void ChangeScene(std::shared_ptr<Scene> scene);

	/// <summary>
	/// フェードを挟んで切り替える
	/// </summary>
	void ChangeSceneWithFade(std::shared_ptr<Scene> scene);

	void Update();
	void Draw();
	void DrawFade() const;

private:
	std::shared_ptr<Scene> scene_;
	std::shared_ptr<Scene> nextScene_;
	Fade fade_;
};
