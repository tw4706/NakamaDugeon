#pragma once
#include "Scene.h"

/// <summary>
/// どのセーブデータで遊ぶかを選ぶ
/// </summary>
class SaveSelectScene : public Scene
{
public:
	SaveSelectScene(SceneManager& manager);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	float time_ = 0.0f;
	int cursor_ = 0;
	bool isDecided_ = false;
};
