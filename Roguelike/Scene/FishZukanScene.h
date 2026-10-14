#pragma once
#include "Scene.h"

/// <summary>
/// つりずかん(しょこ)
/// </summary>
class FishZukanScene : public Scene
{
public:
	FishZukanScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	float time_ = 0.0f;
	int cursor_ = 0;
	bool isLeaving_ = false;
};
