#pragma once
#include"MathUtil.h"
#include"Game.h"
#include<array>

/// <summary>
/// ボタンの種類
/// </summary>
enum class Button
{
	Up,
	Down,
	Left,
	Right,
	Decide,
	Cancel,
	Turn,		//その場で向きを変える
	Menu,		//メニューを ひらく(キーボードは もどると おなじ)
	PageL,		//まえの ページ(LB / Q)
	PageR,		//つぎの ページ(RB / E)
	Num,
};

/// <summary>
/// 4人分の入力
/// P1:キーボード(WASD+SPACE/Z)・パッド1
/// P2:キーボード(矢印+ENTER)・パッド2
/// P3:パッド3 / P4:パッド4
/// </summary>
class Input
{
public:
	static Input& GetInstance();

private:
	Input();
	Input(const Input&) = delete;
	Input& operator=(const Input&) = delete;

public:
	void Update();

	bool IsPressed(int player, Button button) const;
	bool IsTriggered(int player, Button button) const;

	/// <summary>
	/// 誰かが押した瞬間か
	/// </summary>
	bool IsAnyTriggered(Button button) const;

	/// <summary>
	/// 移動入力(長さ1以下)
	/// </summary>
	Vec2 GetMove(int player) const;

	/// <summary>
	/// さいごに さわったのが コントローラーか
	/// </summary>
	bool IsUsingPad() const { return isUsingPad_; }

	/// <summary>
	/// コントローラーを ふるわせる
	/// </summary>
	/// <param name="power">0〜1000</param>
	void Vibrate(int power, int milliseconds);

private:
	static constexpr int kButtonNum = static_cast<int>(Button::Num);
	using ButtonArray = std::array<bool, kButtonNum>;

	std::array<ButtonArray, Game::kPlayerNum> now_ = {};
	std::array<ButtonArray, Game::kPlayerNum> last_ = {};
	std::array<Vec2, Game::kPlayerNum> move_ = {};
	bool isUsingPad_ = false;
};
