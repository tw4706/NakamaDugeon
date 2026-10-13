#pragma once
#include "Scene.h"

/// <summary>
/// モンスター図鑑
/// </summary>
class ZukanScene : public Scene
{
public:
	ZukanScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	float time_ = 0.0f;
	int cursor_ = 0;
	bool isShinyPage_ = false;
	bool isLeaving_ = false;
};
