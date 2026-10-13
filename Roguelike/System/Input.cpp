#include "Input.h"
#include<DxLib.h>
#include<cmath>

namespace
{
	//スティックの遊び
	constexpr float kStickDeadZone = 0.25f;

	//スティックを ボタンとして あつかう たおし具合
	constexpr float kStickButton = 0.5f;

	struct KeyMap
	{
		int up;
		int down;
		int left;
		int right;
		int decide1;
		int decide2;
		int cancel;
		int turn1;
		int turn2;
		int pageL;
		int pageR;
	};

	//キーボードの割り当て(-1は無し)
	constexpr KeyMap kKeyMaps[Game::kPlayerNum] =
	{
		{ KEY_INPUT_W, KEY_INPUT_S, KEY_INPUT_A, KEY_INPUT_D, KEY_INPUT_SPACE, KEY_INPUT_Z, KEY_INPUT_X, KEY_INPUT_C, KEY_INPUT_LSHIFT, KEY_INPUT_Q, KEY_INPUT_E },
		{ KEY_INPUT_UP, KEY_INPUT_DOWN, KEY_INPUT_LEFT, KEY_INPUT_RIGHT, KEY_INPUT_RETURN, KEY_INPUT_NUMPAD0, KEY_INPUT_RSHIFT, KEY_INPUT_RCONTROL, -1, KEY_INPUT_PGUP, KEY_INPUT_PGDN },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	};

	bool IsKeyOn(const char* keys, int code)
	{
		return code >= 0 && keys[code] != 0;
	}
}

Input& Input::GetInstance()
{
	static Input instance;
	return instance;
}

Input::Input()
{
}

void Input::Update()
{
	char keys[256] = {};
	GetHitKeyStateAll(keys);
	for (char k : keys)
	{
		if (k != 0) { isUsingPad_ = false; break; }
	}

	last_ = now_;

	for (int i = 0; i < Game::kPlayerNum; i++)
	{
		const int padType = DX_INPUT_PAD1 + i;
		const int pad = GetJoypadInputState(padType);
		const KeyMap& map = kKeyMaps[i];

		auto& b = now_[i];
		b[static_cast<int>(Button::Up)] = IsKeyOn(keys, map.up) || (pad & PAD_INPUT_UP);
		b[static_cast<int>(Button::Down)] = IsKeyOn(keys, map.down) || (pad & PAD_INPUT_DOWN);
		b[static_cast<int>(Button::Left)] = IsKeyOn(keys, map.left) || (pad & PAD_INPUT_LEFT);
		b[static_cast<int>(Button::Right)] = IsKeyOn(keys, map.right) || (pad & PAD_INPUT_RIGHT);
		b[static_cast<int>(Button::Decide)] = IsKeyOn(keys, map.decide1) || IsKeyOn(keys, map.decide2) || (pad & PAD_INPUT_1);
		b[static_cast<int>(Button::Cancel)] = IsKeyOn(keys, map.cancel) || (pad & PAD_INPUT_2);
		b[static_cast<int>(Button::Turn)] = IsKeyOn(keys, map.turn1) || IsKeyOn(keys, map.turn2) || (pad & PAD_INPUT_4);
		b[static_cast<int>(Button::Menu)] = IsKeyOn(keys, map.cancel) || (pad & PAD_INPUT_8);
		b[static_cast<int>(Button::PageL)] = IsKeyOn(keys, map.pageL) || (pad & PAD_INPUT_5);
		b[static_cast<int>(Button::PageR)] = IsKeyOn(keys, map.pageR) || (pad & PAD_INPUT_6);
		if (pad != 0) isUsingPad_ = true;

		//アナログスティック
		int ax = 0;
		int ay = 0;
		GetJoypadAnalogInput(&ax, &ay, padType);
		Vec2 stick(ax / 1000.0f, ay / 1000.0f);

		//スティックを たおした向きも 上下左右ボタンとして あつかう
		if (std::fabs(stick.y) > kStickButton && std::fabs(stick.y) >= std::fabs(stick.x))
		{
			b[static_cast<int>(stick.y < 0.0f ? Button::Up : Button::Down)] = true;
		}
		if (std::fabs(stick.x) > kStickButton && std::fabs(stick.x) > std::fabs(stick.y))
		{
			b[static_cast<int>(stick.x < 0.0f ? Button::Left : Button::Right)] = true;
		}

		if (stick.Length() > kStickDeadZone)
		{
			move_[i] = stick.Length() > 1.0f ? stick.Normalized() : stick;
			isUsingPad_ = true;
			continue;
		}

		//十字キー・キーボード
		Vec2 dir;
		if (b[static_cast<int>(Button::Up)]) dir.y -= 1.0f;
		if (b[static_cast<int>(Button::Down)]) dir.y += 1.0f;
		if (b[static_cast<int>(Button::Left)]) dir.x -= 1.0f;
		if (b[static_cast<int>(Button::Right)]) dir.x += 1.0f;
		move_[i] = dir.Normalized();
	}}

bool Input::IsPressed(int player, Button button) const
{
	return now_[player][static_cast<int>(button)];
}

bool Input::IsTriggered(int player, Button button) const
{
	const int index = static_cast<int>(button);
	return now_[player][index] && !last_[player][index];
}

bool Input::IsAnyTriggered(Button button) const
{
	for (int i = 0; i < Game::kPlayerNum; i++)
	{
		if (IsTriggered(i, button)) return true;
	}
	return false;
}

Vec2 Input::GetMove(int player) const
{
	return move_[player];
}

void Input::Vibrate(int power, int milliseconds)
{
	for (int i = 0; i < Game::kPlayerNum; i++)
	{
		StartJoypadVibration(DX_INPUT_PAD1 + i, power, milliseconds);
	}
}
