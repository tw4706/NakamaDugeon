#pragma once
#include "Scene.h"
#include "SettingsMenu.h"
#include<string>

/// <summary>
/// せってい(音量・画面・操作・記録の削除)
/// </summary>
class SettingsScene : public Scene
{
public:
	/// <summary>
	/// 終わった後に戻る場所
	/// </summary>
	enum class Return
	{
		Title,
		Camp,
	};

	SettingsScene(SceneManager& manager, Return returnTo);

	void Init() override;
	void Update() override;
	void Draw() override;

private:
	enum class Mode
	{
		Main,
		DeleteSelect,
		DeleteConfirm1,
		DeleteConfirm2,
	};

	void Leave();

	Return returnTo_;
	Mode mode_ = Mode::Main;
	float time_ = 0.0f;
	SettingsMenu menu_{ true };
	int deleteSlot_ = 0;
	int confirmCursor_ = 1;
	bool isLeaving_ = false;
};
