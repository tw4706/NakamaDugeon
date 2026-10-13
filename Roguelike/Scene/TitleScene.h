#pragma once
#include "Scene.h"

/// <summary>
/// タイトル画面「まものつかいの ぼうけん」(はじめる/せってい/おわる)
/// </summary>
class TitleScene : public Scene
{
public:
	TitleScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	void DrawBackground();
	void DrawLogo();

	float time_ = 0.0f;
	int cursor_ = 0;
	bool isDecided_ = false;
};
