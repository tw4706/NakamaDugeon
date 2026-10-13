#pragma once

class GameTime
{
public:

	/// <summary>
	/// 経過時間の設定
	/// </summary>
	static void SetDeltaTime(float deltaTime)
	{
		//ヒットストップ中は止める
		if (hitStopTime_ > 0.0f)
		{
			hitStopTime_ -= deltaTime;
			deltaTime_ = 0.0f;
			return;
		}

		deltaTime_ = deltaTime;
	}

	/// <summary>
	/// ヒットストップの開始
	/// </summary>
	/// <param name="time">止める時間(秒)</param>
	static void StartHitStop(float time)
	{
		if (time > hitStopTime_) hitStopTime_ = time;
	}

	/// <summary>
	/// 経過時間の取得
	/// </summary>
	static float GetDeltaTime() { return deltaTime_; }

private:
	static inline float deltaTime_ = 0.0f;
	static inline float hitStopTime_ = 0.0f;
};
