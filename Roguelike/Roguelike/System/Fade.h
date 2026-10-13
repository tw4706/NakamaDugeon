#pragma once

/// <summary>
/// 円が閉じる・開く画面遷移
/// </summary>
class Fade
{
public:
	void StartFadeIn();
	void StartFadeOut();

	void Update(float dt);
	void Draw() const;

	bool IsFading() const { return state_ == State::FadeIn || state_ == State::FadeOut; }
	bool IsFadeOutEnd() const { return state_ == State::FadeOutEnd; }

private:
	enum class State
	{
		None,
		FadeIn,
		FadeOut,
		FadeOutEnd,
	};
	State state_ = State::None;

	//0で開いている、1で閉じている
	float rate_ = 0.0f;
};
