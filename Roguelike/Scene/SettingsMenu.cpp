#include "SettingsMenu.h"
#include "SaveData.h"
#include "Input.h"
#include "Sound.h"
#include "DrawUtil.h"
#include "ButtonGuide.h"
#include "Font.h"
#include<DxLib.h>
#include<algorithm>
#include<cmath>

namespace
{
	const wchar_t* kStyleNames[3] = { L"じどう", L"キーボード", L"Xbox コントローラー" };
	const wchar_t* kSpeedNames[3] = { L"ゆっくり", L"ふつう", L"はやい" };
	constexpr float kRowH = 43.0f;

	//かぎの マーク
	void DrawLock(float cx, float cy, unsigned int color)
	{
		DrawCircleAA(cx, cy - 5.0f, 6.5f, 16, color, FALSE, 3.0f);
		DrawBoxAA(cx - 9.0f, cy - 3.0f, cx + 9.0f, cy + 10.0f, color, TRUE);
		DrawCircleAA(cx, cy + 3.0f, 2.2f, 8, GetColor(40, 35, 55), TRUE);
		DrawLineAA(cx, cy + 3.0f, cx, cy + 7.0f, GetColor(40, 35, 55), 2.0f);
	}
}

SettingsMenu::SettingsMenu(bool hasDelete) :
	hasDelete_(hasDelete)
{
}

int SettingsMenu::RowNum() const
{
	return hasDelete_ ? kItemNum : kItemNum - 1;
}

int SettingsMenu::ItemAt(int row) const
{
	if (!hasDelete_ && row >= kDelete) return row + 1;
	return row;
}

void SettingsMenu::Notice(const std::wstring& text)
{
	notice_ = text;
	noticeTime_ = 2.5f;
}

SettingsMenu::Result SettingsMenu::Update()
{
	time_ += Game::kFrameTime;
	noticeTime_ = std::max(0.0f, noticeTime_ - Game::kFrameTime);

	Input& input = Input::GetInstance();
	Sound& sound = Sound::GetInstance();
	SaveManager& sm = SaveManager::GetInstance();
	Settings& st = sm.GetSettings();
	const bool decide = input.IsAnyTriggered(Button::Decide);
	const bool cancel = input.IsAnyTriggered(Button::Cancel);

	if (isControls_)
	{
		if (decide || cancel) { isControls_ = false; sound.PlaySE(SE::Cancel); }
		return Result::None;
	}

	const int num = RowNum();
	if (input.IsAnyTriggered(Button::Up)) { cursor_ = (cursor_ + num - 1) % num; sound.PlaySE(SE::Cursor); }
	if (input.IsAnyTriggered(Button::Down)) { cursor_ = (cursor_ + 1) % num; sound.PlaySE(SE::Cursor); }
	if (cancel)
	{
		sm.SaveSettings();
		sound.PlaySE(SE::Cancel);
		return Result::Close;
	}

	const int dir = (input.IsAnyTriggered(Button::Right) ? 1 : 0) - (input.IsAnyTriggered(Button::Left) ? 1 : 0);
	const int item = ItemAt(cursor_);
	const int step = dir != 0 ? dir : (decide ? 1 : 0);
	switch (item)
	{
	case kBgm:
		if (dir != 0) { st.bgmVolume = std::clamp(st.bgmVolume + dir, 0, 10); sound.SetVolume(st.BgmLevel(), st.SeLevel()); }
		break;
	case kSe:
		if (dir != 0) { st.seVolume = std::clamp(st.seVolume + dir, 0, 10); sound.SetVolume(st.BgmLevel(), st.SeLevel()); sound.PlaySE(SE::Coin); }
		break;
	case kMute:
		if (step != 0)
		{
			st.isMute = !st.isMute;
			sound.SetVolume(st.BgmLevel(), st.SeLevel());
			if (!st.isMute) sound.PlaySE(SE::Cursor);
		}
		break;
	case kButton:
		if (step != 0) { st.buttonStyle = (st.buttonStyle + 3 + step) % 3; sound.PlaySE(SE::Cursor); }
		break;
	case kSpeed:
		if (step != 0)
		{
			//「はやい」は クリアするまで えらべない
			const int maxSpeed = sm.IsFastMoveUnlocked() ? 2 : 1;
			const int now = std::min(st.moveSpeed, maxSpeed);
			const int next = now + step;
			if (next > maxSpeed && maxSpeed < 2)
			{
				sound.PlaySE(SE::Buzzer);
				Notice(L"「はやい」は ダンジョンを クリアすると えらべる");
			}
			else if (next >= 0 && next <= maxSpeed)
			{
				st.moveSpeed = next;
				sound.PlaySE(SE::Cursor);
			}
		}
		break;
	case kShake:
		if (step != 0) { st.isShake = !st.isShake; sound.PlaySE(SE::Cursor); }
		break;
	case kVibration:
		if (step != 0)
		{
			st.isVibration = !st.isVibration;
			sound.PlaySE(SE::Cursor);
			if (st.isVibration) input.Vibrate(600, 200);
		}
		break;
	case kScreen:
		if (step != 0)
		{
			st.isFullScreen = !st.isFullScreen;
			sound.PlaySE(SE::Cursor);
			Notice(L"つぎに きどうしたときから かわります");
		}
		break;
	case kControls:
		if (decide) { isControls_ = true; sound.PlaySE(SE::Decide); }
		break;
	case kDelete:
		if (decide) { sound.PlaySE(SE::Decide); return Result::Delete; }
		break;
	case kBack:
		if (decide)
		{
			sm.SaveSettings();
			sound.PlaySE(SE::Cancel);
			return Result::Close;
		}
		break;
	default:
		break;
	}
	return Result::None;
}

void SettingsMenu::Draw() const
{
	const Settings& st = SaveManager::GetInstance().GetSettings();
	const unsigned int edge = GetColor(20, 15, 30);
	const int num = RowNum();
	const bool isFastLocked = !SaveManager::GetInstance().IsFastMoveUnlocked();
	const float top = 350.0f - (num * kRowH) * 0.5f;
	DrawUtil::Window(250.0f, top - 70.0f, 1030.0f, top + num * kRowH + 20.0f);
	Font::DrawCenter(640.0f, top - 30.0f, L"せってい", GetColor(255, 230, 140), FontSize::Medium, edge);

	const wchar_t* labels[kItemNum] =
	{
		L"BGM の おおきさ", L"こうかおん の おおきさ", L"しょうおん（おとを けす）", L"ボタンの ひょうじ", L"いどうの はやさ",
		L"がめんの ゆれ", L"コントローラーの しんどう", L"がめん", L"そうさの かくにん", L"きろくを けす", L"もどる",
	};
	for (int row = 0; row < num; row++)
	{
		const int item = ItemAt(row);
		const float y = top + 20.0f + row * kRowH;
		const bool isSel = row == cursor_;
		if (isSel) DrawUtil::Cursor(285.0f, y, time_);
		const unsigned int col = item == kDelete ? GetColor(255, 150, 150) : (isSel ? GetColor(255, 240, 150) : GetColor(210, 200, 230));
		Font::DrawFit(320.0f, y - 13.0f, labels[item], col, FontSize::Small, edge, 360.0f);

		std::wstring value;
		switch (item)
		{
		case kBgm:
		case kSe:
		{
			const int v = item == kBgm ? st.bgmVolume : st.seVolume;
			for (int k = 0; k < 10; k++)
			{
				const float bx = 730.0f + k * 24.0f;
				const unsigned int on = st.isMute ? GetColor(120, 110, 90) : GetColor(255, 210, 90);
				DrawBoxAA(bx, y - 11.0f, bx + 18.0f, y + 11.0f, k < v ? on : GetColor(60, 55, 80), TRUE);
			}
			break;
		}
		case kMute: value = st.isMute ? L"オン" : L"オフ"; break;
		case kButton: value = kStyleNames[std::clamp(st.buttonStyle, 0, 2)]; break;
		case kSpeed: value = kSpeedNames[std::clamp(isFastLocked ? std::min(st.moveSpeed, 1) : st.moveSpeed, 0, 2)]; break;
		case kShake: value = st.isShake ? L"あり" : L"なし"; break;
		case kVibration: value = st.isVibration ? L"あり" : L"なし"; break;
		case kScreen: value = st.isFullScreen ? L"ぜんがめん" : L"ウィンドウ"; break;
		default: break;
		}
		//ふつうの みぎに かぎ(クリアまで はやいに できない)
		const bool isSpeedLock = item == kSpeed && isFastLocked && st.moveSpeed >= 1;
		if (isSpeedLock) DrawLock(985.0f, y, isSel ? GetColor(255, 220, 120) : GetColor(150, 140, 170));
		if (!value.empty())
		{
			Font::DrawCenterFit(850.0f, y, value, GetColor(255, 240, 150), FontSize::Small, edge, 240.0f);
			if (isSel)
			{
				const float bob = std::sin(time_ * 6.0f) * 3.0f;
				Font::DrawCenter(715.0f - bob, y, L"◀", GetColor(255, 220, 120), FontSize::Small, edge);
				if (!isSpeedLock) Font::DrawCenter(985.0f + bob, y, L"▶", GetColor(255, 220, 120), FontSize::Small, edge);
			}
		}
	}

	//じどうの ときは いま どちらか
	const float infoY = top + num * kRowH + 50.0f;
	if (noticeTime_ > 0.0f)
	{
		Font::DrawCenterAlpha(640.0f, infoY, 1.0f, static_cast<int>(255 * std::min(1.0f, noticeTime_)), notice_.c_str(), GetColor(255, 220, 160), FontSize::Small, edge);
	}
	else if (ItemAt(cursor_) == kButton && st.buttonStyle == 0)
	{
		Font::DrawCenter(640.0f, infoY, ButtonGuide::IsPad() ? L"いまは コントローラーの ひょうじ" : L"いまは キーボードの ひょうじ", GetColor(180, 200, 230), FontSize::Small, edge);
	}
	ButtonGuide::DrawCenter(640.0f, 694.0f, L"{UD}：えらぶ　{LR}：かえる　{A}：けってい　{B}：もどる", GetColor(170, 160, 190), FontSize::Small, GetColor(10, 10, 20));

	if (isControls_) DrawControls();
}

void SettingsMenu::DrawControls() const
{
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);
	DrawBox(0, 0, Game::kScreenWidth, Game::kScreenHeight, GetColor(0, 0, 0), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	const unsigned int edge = GetColor(20, 15, 30);
	const bool isPad = ButtonGuide::IsPad();
	DrawUtil::Window(230.0f, 110.0f, 1050.0f, 640.0f);
	Font::DrawCenter(640.0f, 150.0f, isPad ? L"そうさ（コントローラー）" : L"そうさ（キーボード）", GetColor(255, 230, 140), FontSize::Medium, edge);

	struct Line { const wchar_t* key; const wchar_t* what; };
	const Line lines[] =
	{
		{ L"{MOVE}", isPad ? L"いどう（じゅうじキーでも うごける）" : L"いどう（やじるしキーでも うごける）" },
		{ L"{UD} {LR}", L"メニューで えらぶ" },
		{ L"{A}", L"けってい・こうげき・かいだんを おりる" },
		{ L"{Y} ＋ {MOVE}", L"その ばで むきだけ かえる" },
		{ L"{B}", L"もどる" },
		{ L"{M}", L"メニューを ひらく" },
		{ L"", L"ななめは 2つの ほうこうを どうじに" },
	};
	for (int i = 0; i < static_cast<int>(std::size(lines)); i++)
	{
		const float y = 215.0f + i * 50.0f;
		ButtonGuide::Draw(300.0f, y, lines[i].key, GetColor(255, 240, 180), FontSize::Small, edge);
		Font::DrawFit(520.0f, y - 13.0f, lines[i].what, GetColor(230, 225, 245), FontSize::Small, edge, 500.0f);
	}
	if (!isPad) Font::DrawFit(300.0f, 565.0f, L"ESC：ゲームを おわる", GetColor(180, 170, 200), FontSize::Small, edge, 700.0f);
	ButtonGuide::DrawCenter(640.0f, 610.0f, L"{A} / {B}：もどる", GetColor(160, 150, 180), FontSize::Small, edge);
}
