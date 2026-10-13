#include "Sound.h"
#include<DxLib.h>
#include<vector>
#include<cmath>
#include<cstdint>
#include<cstring>
#include<algorithm>
#include<string>
#include<cstdlib>

namespace
{
	constexpr int kSampleRate = 22050;
	constexpr float kTwoPi = 6.2831853f;

	//同時再生できる数
	constexpr int kSEBufferNum = 4;


	enum class Wave
	{
		Square,
		Triangle,
		Saw,
		Sine,
		Noise,
	};

	struct Tone
	{
		float start = 0.0f;
		float dur = 0.1f;
		float f0 = 440.0f;
		float f1 = 440.0f;
		Wave wave = Wave::Square;
		float vol = 0.3f;
		float duty = 0.5f;
		float attack = 0.004f;
		float decay = 0.0f;
		float vibrato = 0.0f;
		float release = 0.01f;	//消えるまでの時間
		float lowpass = 0.0f;	//こもらせる周波数(0で無し)
	};

	/// <summary>
	/// 波形を足し合わせて音を作る
	/// </summary>
	class Synth
	{
	public:
		explicit Synth(float seconds) :
			buf_(static_cast<size_t>(seconds * kSampleRate) + 1, 0.0f)
		{
		}

		float Length() const { return static_cast<float>(buf_.size()) / kSampleRate; }

		void Add(const Tone& t)
		{
			const size_t start = static_cast<size_t>(t.start * kSampleRate);
			const size_t num = static_cast<size_t>(t.dur * kSampleRate);
			const bool isSweep = t.f0 != t.f1;
			const float ratio = isSweep ? t.f1 / t.f0 : 1.0f;

			float phase = 0.0f;
			float noise = NextNoise();
			const float lpRate = t.lowpass > 0.0f ? 1.0f - std::exp(-kTwoPi * t.lowpass / kSampleRate) : 1.0f;
			float lp = 0.0f;
			const float release = std::max(0.002f, t.release);
			for (size_t i = 0; i < num; i++)
			{
				const size_t index = start + i;
				if (index >= buf_.size()) break;

				const float time = static_cast<float>(i) / kSampleRate;
				const float rate = time / t.dur;

				float freq = isSweep ? t.f0 * std::pow(ratio, rate) : t.f0;
				if (t.vibrato > 0.0f) freq *= 1.0f + t.vibrato * std::sin(kTwoPi * 6.0f * time);

				phase += freq / kSampleRate;
				bool isWrap = false;
				if (phase >= 1.0f)
				{
					phase -= std::floor(phase);
					isWrap = true;
				}

				float sample = 0.0f;
				switch (t.wave)
				{
				case Wave::Square:	sample = phase < t.duty ? 1.0f : -1.0f; break;
				case Wave::Triangle:	sample = 1.0f - 4.0f * std::fabs(phase - 0.5f); break;
				case Wave::Saw:		sample = 2.0f * phase - 1.0f; break;
				case Wave::Sine:		sample = std::sin(kTwoPi * phase); break;
				case Wave::Noise:
					if (isWrap) noise = NextNoise();
					sample = noise;
					break;
				}

				lp += (sample - lp) * lpRate;
				sample = lp;

				float env = t.attack > 0.0f ? std::min(1.0f, time / t.attack) : 1.0f;
				if (t.decay > 0.0f) env *= std::exp(-t.decay * time);
				env *= std::min(1.0f, (t.dur - time) / release);

				buf_[index] += sample * t.vol * env;
			}
		}

		/// <summary>
		/// サウンドハンドルの作成
		/// </summary>
		int MakeHandle(float gain, int bufferNum)
		{
			const int soft = MakeSoftSound1Ch16Bit22KHz(static_cast<LONGLONG>(buf_.size()));
			for (size_t i = 0; i < buf_.size(); i++)
			{
				const int value = static_cast<int>(std::tanh(buf_[i] * gain) * 32000.0f);
				WriteSoftSoundData(soft, static_cast<LONGLONG>(i), value, value);
			}
			const int handle = LoadSoundMemFromSoftSound(soft, bufferNum);
			DeleteSoftSound(soft);
			return handle;
		}

	private:
		float NextNoise()
		{
			seed_ = seed_ * 1664525u + 1013904223u;
			return static_cast<float>((seed_ >> 8) & 0xFFFF) / 32767.5f - 1.0f;
		}

		std::vector<float> buf_;
		uint32_t seed_ = 12345u;
	};

	float MidiToFreq(int note)
	{
		return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
	}

	//0は休符、-1は前の音を伸ばす
	void AddNotes(Synth& s, const std::vector<int>& notes, float step, Wave wave, float vol,
		float gate, float duty = 0.5f, float decay = 0.0f, float offset = 0.0f, float vibrato = 0.0f)
	{
		for (size_t i = 0; i < notes.size(); i++)
		{
			const int note = notes[i];
			if (note <= 0) continue;

			size_t len = 1;
			while (i + len < notes.size() && notes[i + len] == -1) len++;

			Tone t;
			t.start = offset + i * step;
			t.dur = len * step * gate;
			t.f0 = t.f1 = MidiToFreq(note);
			t.wave = wave;
			t.vol = vol;
			t.duty = duty;
			t.decay = decay;
			t.vibrato = vibrato;
			t.attack = 0.006f;
			s.Add(t);
		}
	}

	void AddKick(Synth& s, float at, float vol)
	{
		s.Add({ at, 0.18f, 160.0f, 40.0f, Wave::Sine, vol, 0.5f, 0.002f, 14.0f });
	}

	//------------------------------//
	// 楽譜
	//------------------------------//

	/// <summary>
	/// 楽譜の1音(和音も可)
	/// </summary>
	struct Event
	{
		std::vector<int> notes;		//空なら休符
		float beats;
	};

	/// <summary>
	/// 楽譜の文字列を読む
	/// 例: "c5:1 e5+g5:.5 r:.5 | b-4:2" (音名+オクターブ:拍、#はシャープ、-はフラット、rは休符、|は区切り)
	/// </summary>
	std::vector<Event> Parse(const char* score)
	{
		static const int kPitch[7] = { 9, 11, 0, 2, 4, 5, 7 };
		std::vector<Event> out;
		const char* p = score;
		while (*p)
		{
			while (*p == ' ' || *p == '|') p++;
			if (!*p) break;

			Event e;
			while (*p && *p != ':')
			{
				if (*p == '+' || *p == 'r') { p++; continue; }
				int pitch = kPitch[*p - 'a'];
				p++;
				if (*p == '#') { pitch++; p++; }
				else if (*p == '-') { pitch--; p++; }
				int octave = 0;
				while (*p >= '0' && *p <= '9') { octave = octave * 10 + (*p - '0'); p++; }
				e.notes.push_back((octave + 1) * 12 + pitch);
			}
			if (*p == ':') p++;
			char* end = nullptr;
			e.beats = std::strtof(p, &end);
			p = end;
			out.push_back(e);
		}
		return out;
	}

	/// <summary>
	/// 楽器の音色
	/// </summary>
	struct Inst
	{
		Wave wave;
		float vol;
		float duty;
		float attack;
		float release;
		float decay;
		float vibrato;
		float lowpass;
		float gate;		//音の長さの割合
	};

	const Inst kBrass = { Wave::Saw, 0.075f, 0.5f, 0.035f, 0.07f, 0.0f, 0.004f, 2000.0f, 0.92f };
	const Inst kBrassEdge = { Wave::Square, 0.04f, 0.25f, 0.02f, 0.06f, 0.0f, 0.004f, 3200.0f, 0.92f };
	const Inst kStrings = { Wave::Saw, 0.03f, 0.5f, 0.16f, 0.22f, 0.0f, 0.005f, 1500.0f, 1.0f };
	const Inst kOboe = { Wave::Square, 0.085f, 0.15f, 0.03f, 0.06f, 0.0f, 0.007f, 1900.0f, 0.9f };
	const Inst kFlute = { Wave::Sine, 0.11f, 0.5f, 0.05f, 0.08f, 0.0f, 0.006f, 0.0f, 0.92f };
	const Inst kHarp = { Wave::Triangle, 0.09f, 0.5f, 0.002f, 0.06f, 4.0f, 0.0f, 0.0f, 1.0f };
	const Inst kPizz = { Wave::Triangle, 0.08f, 0.5f, 0.002f, 0.03f, 10.0f, 0.0f, 0.0f, 0.9f };
	const Inst kBass = { Wave::Triangle, 0.32f, 0.5f, 0.006f, 0.03f, 0.0f, 0.0f, 0.0f, 0.85f };
	const Inst kBassEdge = { Wave::Saw, 0.08f, 0.5f, 0.006f, 0.03f, 0.0f, 0.0f, 380.0f, 0.85f };
	const Inst kOrgan = { Wave::Sine, 0.045f, 0.5f, 0.1f, 0.18f, 0.0f, 0.0f, 0.0f, 1.0f };
	const Inst kTremolo = { Wave::Square, 0.03f, 0.125f, 0.004f, 0.02f, 0.0f, 0.0f, 1400.0f, 0.8f };

	void Play(Synth& s, const std::vector<Event>& events, float beat, const Inst& in, int transpose, float startBeat = 0.0f)
	{
		float at = startBeat;
		for (const Event& e : events)
		{
			for (int n : e.notes)
			{
				Tone t;
				t.start = at * beat;
				t.dur = e.beats * beat * in.gate;
				t.f0 = t.f1 = MidiToFreq(n + transpose);
				t.wave = in.wave;
				t.vol = in.vol;
				t.duty = in.duty;
				t.attack = in.attack;
				t.decay = in.decay;
				t.vibrato = in.vibrato;
				t.release = in.release;
				t.lowpass = in.lowpass;
				s.Add(t);
			}
			at += e.beats;
		}
	}

	/// <summary>
	/// 和音を分散和音にする
	/// </summary>
	std::vector<Event> Arpeggio(const std::vector<Event>& chords, float step, const std::vector<int>& order, int shift)
	{
		std::vector<Event> out;
		for (const Event& e : chords)
		{
			const int count = static_cast<int>(e.beats / step + 0.5f);
			for (int i = 0; i < count; i++)
			{
				if (e.notes.empty()) { out.push_back({ {}, step }); continue; }
				const int index = order[i % order.size()] % static_cast<int>(e.notes.size());
				out.push_back({ { e.notes[index] + shift }, step });
			}
		}
		return out;
	}

	/// <summary>
	/// 根音を刻む(offsetsは根音からの半音差)
	/// </summary>
	std::vector<Event> Pump(const std::vector<Event>& roots, float step, const std::vector<int>& offsets)
	{
		std::vector<Event> out;
		for (const Event& e : roots)
		{
			const int count = static_cast<int>(e.beats / step + 0.5f);
			for (int i = 0; i < count; i++)
			{
				if (e.notes.empty()) { out.push_back({ {}, step }); continue; }
				out.push_back({ { e.notes[0] + offsets[i % offsets.size()] }, step });
			}
		}
		return out;
	}

	//------------------------------//
	// 打楽器(オーケストラ)
	//------------------------------//

	void AddTimpani(Synth& s, float at, int note, float vol)
	{
		const float f = MidiToFreq(note);
		s.Add({ at, 0.9f, f * 1.03f, f, Wave::Sine, vol, 0.5f, 0.002f, 4.5f });
		s.Add({ at, 0.5f, f * 1.5f, f * 1.5f, Wave::Sine, vol * 0.25f, 0.5f, 0.002f, 8.0f });
		Tone hit = { at, 0.04f, 3000.0f, 3000.0f, Wave::Noise, vol * 0.2f, 0.5f, 0.001f, 60.0f };
		hit.lowpass = 1200.0f;
		s.Add(hit);
	}

	void AddTimpaniRoll(Synth& s, float at, float dur, int note, float vol)
	{
		for (float t = 0.0f; t < dur; t += 0.065f)
		{
			const float rate = 0.35f + 0.65f * (t / dur);
			AddTimpani(s, at + t, note, vol * rate * 0.6f);
		}
	}

	void AddCymbal(Synth& s, float at, float vol)
	{
		s.Add({ at, 2.0f, 22050.0f, 22050.0f, Wave::Noise, vol, 0.5f, 0.004f, 2.2f });
	}

	void AddSideDrum(Synth& s, float at, float vol)
	{
		Tone t = { at, 0.12f, 9000.0f, 9000.0f, Wave::Noise, vol, 0.5f, 0.001f, 28.0f };
		t.lowpass = 5000.0f;
		s.Add(t);
	}

	//------------------------------//
	// BGM(ドラクエ風のオーケストラ)
	//------------------------------//

	/// <summary>
	/// タイトル: 序曲風の行進曲(変ホ長調)
	/// </summary>
	int MakeTitleBgm()
	{
		constexpr float kBpm = 108.0f;
		constexpr int kBars = 16;
		constexpr int kKey = 3;
		const float beat = 60.0f / kBpm;
		Synth s(kBars * 4 * beat);

		const auto melody = Parse(
			"g4:.333 g4:.333 g4:.334 c5:2 e5:1 | d5:.333 d5:.333 d5:.334 g5:3 |"
			"c5:1.5 g4:.5 c5:1 e5:1 | g5:2 f5:1 e5:1 | f5:1.5 e5:.5 d5:1 c5:1 | d5:3 r:1 |"
			"e5:1.5 d5:.5 e5:1 g5:1 | a5:2 g5:1 f5:1 | e5:1.5 d5:.5 c5:1 d5:1 | c5:3 r:1 |"
			"a5:1.5 g5:.5 f5:1 a5:1 | g5:2 e5:2 | f5:1.5 e5:.5 d5:1 f5:1 | e5:2 c5:1 a4:1 |"
			"a4:1 b4:1 c5:1 d5:1 | g5:3 r:1");
		const auto chords = Parse(
			"c4+e4+g4:4 | b3+d4+g4:4 | c4+e4+g4:4 | c4+e4+g4:4 | a3+c4+f4:4 | b3+d4+g4:4 | c4+e4+g4:4 | a3+c4+f4:4 |"
			"c4+e4+g4:2 b3+d4+g4:2 | c4+e4+g4:4 | a3+c4+f4:4 | c4+e4+g4:4 | a3+d4+f4:4 | a3+c4+e4:4 | a3+c4+f4:2 b3+d4+g4:2 | b3+f4+g4:4");
		const auto bass = Parse(
			"c2:4 | g2:4 | c3:1 g2:1 c3:1 g2:1 | c3:1 g2:1 c3:1 g2:1 | f2:1 c3:1 f2:1 c3:1 | g2:1 d3:1 g2:1 d3:1 |"
			"c3:1 g2:1 c3:1 g2:1 | f2:1 c3:1 f2:1 c3:1 | c3:1 g2:1 g2:1 d3:1 | c3:1 g2:1 c3:1 g2:1 | f2:1 c3:1 f2:1 c3:1 |"
			"c3:1 g2:1 c3:1 g2:1 | d3:1 a2:1 d3:1 a2:1 | a2:1 e3:1 a2:1 e3:1 | f2:1 c3:1 g2:1 d3:1 | g2:1 d3:1 g2:1 b2:1");

		Play(s, melody, beat, kBrass, kKey);
		Play(s, melody, beat, kBrassEdge, kKey);
		Play(s, chords, beat, kStrings, kKey);
		Play(s, chords, beat, kStrings, kKey + 12);
		Play(s, bass, beat, kBass, kKey);
		Play(s, bass, beat, kBassEdge, kKey);

		//ティンパニとスネア
		AddTimpaniRoll(s, 0.0f, beat * 4.0f, 36 + kKey, 0.5f);
		AddTimpaniRoll(s, beat * 4.0f, beat * 3.0f, 43 + kKey, 0.5f);
		AddCymbal(s, beat * 8.0f, 0.12f);
		for (int bar = 2; bar < kBars; bar++)
		{
			const float b = bar * 4.0f;
			if (bar % 2 == 0) AddTimpani(s, b * beat, 36 + kKey, 0.4f);
			static const float kSnare[] = { 0.0f, 1.5f, 1.75f, 2.0f, 3.0f };
			for (float o : kSnare) AddSideDrum(s, (b + o) * beat, 0.05f);
		}
		return s.MakeHandle(0.9f, 1);
	}

	/// <summary>
	/// キャンプ: のどかな村のワルツ(ヘ長調)
	/// </summary>
	int MakeVillageBgm()
	{
		constexpr float kBpm = 168.0f;
		constexpr int kBars = 16;
		constexpr int kKey = 5;
		const float beat = 60.0f / kBpm;
		Synth s(kBars * 3 * beat);

		const auto melody = Parse(
			"e5:2 g5:1 | c6:2 b5:1 | a5:1 g5:1 f5:1 | e5:3 | d5:2 e5:1 | f5:2 a5:1 | g5:1 f5:1 e5:1 | d5:3 |"
			"e5:2 g5:1 | c6:2 e6:1 | d6:1 c6:1 a5:1 | g5:3 | f5:1 e5:1 d5:1 | g5:1.5 e5:.5 c5:1 | d5:2 b4:1 | c5:3");

		//和音(根音, 上の和音)
		static const char* kRoots[kBars] = { "c3", "c3", "f2", "c3", "g2", "d3", "g2", "g2", "c3", "a2", "f2", "c3", "d3", "c3", "g2", "c3" };
		static const char* kUpper[kBars] =
		{
			"e4+g4+c5", "e4+g4+c5", "f4+a4+c5", "e4+g4+c5", "d4+g4+b4", "d4+f4+a4", "d4+g4+b4", "d4+f4+b4",
			"e4+g4+c5", "e4+a4+c5", "f4+a4+c5", "e4+g4+c5", "d4+f4+a4", "e4+g4+c5", "d4+f4+b4", "e4+g4+c5",
		};
		std::string bass;
		std::string pah;
		std::string pad;
		for (int i = 0; i < kBars; i++)
		{
			bass += std::string(kRoots[i]) + ":1 r:2 ";
			pah += std::string("r:1 ") + kUpper[i] + ":1 " + kUpper[i] + ":1 ";
			pad += std::string(kUpper[i]) + ":3 ";
		}

		Play(s, melody, beat, kOboe, kKey);
		Play(s, melody, beat, kFlute, kKey + 12);
		Play(s, Parse(bass.c_str()), beat, kBass, kKey);
		Play(s, Parse(pah.c_str()), beat, kPizz, kKey);
		Play(s, Parse(pad.c_str()), beat, kStrings, kKey - 12);

		//後半はハープを足す
		const auto arp = Arpeggio(Parse(pad.c_str()), 0.5f, { 0, 1, 2, 1, 2, 1 }, 12);
		std::vector<Event> late(arp.begin() + arp.size() / 2, arp.end());
		Play(s, late, beat, kHarp, kKey, kBars * 3 * 0.5f);
		return s.MakeHandle(0.9f, 1);
	}

	/// <summary>
	/// ダンジョン: 下がっていく低音の重い曲(ニ短調)
	/// </summary>
	int MakeDungeonBgm()
	{
		constexpr float kBpm = 76.0f;
		constexpr int kBars = 16;
		constexpr int kKey = 5;
		const float beat = 60.0f / kBpm;
		Synth s(kBars * 4 * beat);

		const auto melody = Parse(
			"e5:2 d5:1 b4:1 | c5:2 d5:1.5 c5:.5 | a4:2 b4:1 g#4:1 | b4:4 | f5:2 e5:1 d5:1 | e5:2 c5:1 a4:1 | d5:1.5 f5:.5 e5:1.5 d5:.5 | c5:2 b4:2 |"
			"a5:2 g#5:1 e5:1 | g5:2 f#5:1.5 e5:.5 | f5:2 e5:1 d5:1 | e5:4 | a5:2 g5:1 f5:1 | g5:2 e5:1 c5:1 | f5:1.5 a5:.5 g#5:1.5 b5:.5 | a5:2 g#5:2");
		const char* chordText =
			"a3+c4+e4:2 g#3+b3+e4:2 | a3+c4+e4:2 f#3+a3+d4:2 | a3+c4+f4:2 g#3+b3+e4:2 | g#3+b3+e4:4 |"
			"a3+d4+f4:2 g3+b3+d4:2 | g3+c4+e4:2 a3+c4+f4:2 | a3+d4+f4:2 g#3+b3+e4:2 | a3+c4+e4:2 g#3+b3+e4:2 |";
		const char* bassText = "a2:2 g#2:2 | g2:2 f#2:2 | f2:2 e2:2 | e2:4 | d2:2 g2:2 | c3:2 f2:2 | d2:2 e2:2 | a2:2 e2:2 |";
		const auto chords = Parse((std::string(chordText) + chordText).c_str());
		const auto bass = Parse((std::string(bassText) + bassText).c_str());

		//前半は笛、後半は弦で強く
		const std::vector<Event> first(melody.begin(), melody.begin() + melody.size() / 2);
		const std::vector<Event> second(melody.begin() + melody.size() / 2, melody.end());
		Play(s, first, beat, kFlute, kKey - 12);
		Play(s, first, beat, kOboe, kKey - 12);
		Inst lead = kStrings;
		lead.vol = 0.07f;
		lead.attack = 0.08f;
		Play(s, second, beat, lead, kKey - 12, kBars * 2.0f);
		Play(s, second, beat, kOboe, kKey - 12, kBars * 2.0f);

		Play(s, chords, beat, kOrgan, kKey);
		Play(s, chords, beat, kOrgan, kKey + 12);
		Play(s, bass, beat, kBass, kKey);
		Play(s, bass, beat, kBassEdge, kKey);
		Play(s, Arpeggio(chords, 0.5f, { 0, 1, 2, 1 }, 12), beat, kHarp, kKey);

		//心臓の音のようなティンパニ
		for (int bar = 0; bar < kBars; bar++)
		{
			AddTimpani(s, bar * 4.0f * beat, 33 + kKey, 0.28f);
			AddTimpani(s, (bar * 4.0f + 0.5f) * beat, 33 + kKey, 0.16f);
		}
		return s.MakeHandle(0.9f, 1);
	}

	/// <summary>
	/// ボス: はげしい戦いの曲(イ短調)
	/// </summary>
	int MakeBossBgm()
	{
		constexpr float kBpm = 160.0f;
		constexpr int kBars = 16;
		constexpr int kKey = 0;
		const float beat = 60.0f / kBpm;
		Synth s(kBars * 4 * beat);

		const auto melody = Parse(
			"a3+e4+a4:.5 r:1 a3+e4+a4:.5 r:.5 a3+e4+a4:.5 b-3+f4+b-4:1 | g#3+b3+e4+g#4:3 r:1 |"
			"a4:1.5 c5:.5 e5:1 a5:1 | f5:1.5 e5:.5 c5:2 | d5:1.5 f5:.5 a5:1 d6:1 | c6:.5 b5:.5 g#5:1 e5:2 |"
			"a5:1 e5:.5 a5:.5 c6:1 b5:1 | a5:1 f5:1 g5:1 d5:1 | e5:1.5 f5:.5 g#5:1 b5:1 | a5:3 r:1 |"
			"c6:1.5 a5:.5 f5:1 c6:1 | g5:1.5 e5:.5 c5:1 g5:1 | f5:1 a5:1 d6:1 c6:1 | b5:1.5 g#5:.5 e5:2 |"
			"a5:1 c6:1 d6:1 f6:1 | e6:2 r:2");
		const auto chords = Parse(
			"a3+c4+e4:4 | g#3+b3+e4:4 | a3+c4+e4:4 | a3+c4+f4:4 | a3+d4+f4:4 | g#3+b3+e4:4 | a3+c4+e4:4 | a3+c4+f4:2 b3+d4+g4:2 |"
			"g#3+b3+e4:4 | a3+c4+e4:4 | a3+c4+f4:4 | g3+c4+e4:4 | a3+d4+f4:4 | g#3+b3+e4:4 | a3+c4+f4:2 a3+d4+f4:2 | g#3+b3+e4:4");
		const auto roots = Parse("a2:4 | e2:4 | a2:4 | f2:4 | d2:4 | e2:4 | a2:4 | f2:2 g2:2 | e2:4 | a2:4 | f2:4 | c3:4 | d2:4 | e2:4 | f2:2 d2:2 | e2:4");

		Inst brass = kBrass;
		brass.vol = 0.085f;
		Play(s, melody, beat, brass, kKey);
		Play(s, melody, beat, kBrassEdge, kKey);
		Play(s, chords, beat, kStrings, kKey);
		Play(s, Arpeggio(chords, 0.25f, { 0, 1, 2, 1 }, 12), beat, kTremolo, kKey);
		const auto pump = Pump(roots, 0.5f, { 0, 0, 12, 0 });
		Play(s, pump, beat, kBass, kKey);
		Play(s, pump, beat, kBassEdge, kKey);

		AddCymbal(s, beat * 8.0f, 0.12f);
		for (int bar = 0; bar < kBars; bar++)
		{
			const float b = bar * 4.0f;
			AddTimpani(s, b * beat, 45 + kKey, 0.42f);
			AddTimpani(s, (b + 2.0f) * beat, 40 + kKey, 0.32f);
			AddSideDrum(s, (b + 1.0f) * beat, 0.06f);
			AddSideDrum(s, (b + 3.0f) * beat, 0.06f);
			AddSideDrum(s, (b + 3.5f) * beat, 0.04f);
		}
		AddTimpaniRoll(s, (kBars - 1) * 4.0f * beat + beat * 2.0f, beat * 2.0f, 40 + kKey, 0.5f);
		return s.MakeHandle(0.9f, 1);
	}

	/// <summary>
	/// クリア・帰還: よろこびの行進曲(ハ長調)
	/// </summary>
	int MakeVictoryBgm()
	{
		constexpr float kBpm = 116.0f;
		constexpr int kBars = 8;
		constexpr int kKey = 0;
		const float beat = 60.0f / kBpm;
		Synth s(kBars * 4 * beat);

		const auto melody = Parse(
			"c5:1 e5:1 g5:1 c6:1 | b5:1.5 a5:.5 g5:2 | a5:1 c6:1 e6:1.5 d6:.5 | c6:2 a5:1 f5:1 |"
			"g5:1.5 e5:.5 c5:1 e5:1 | f5:1.5 a5:.5 c6:1 a5:1 | g5:1 b5:1 d6:1 f6:1 | e6:3 r:1");
		const auto chords = Parse(
			"c4+e4+g4:4 | b3+d4+g4:4 | a3+c4+e4:4 | a3+c4+f4:4 | g3+c4+e4:4 | a3+c4+f4:4 | b3+f4+g4:4 | c4+e4+g4:4");
		const auto bass = Parse(
			"c3:1 g2:1 c3:1 g2:1 | b2:1 g2:1 b2:1 d3:1 | a2:1 e3:1 a2:1 e3:1 | f2:1 c3:1 f2:1 c3:1 |"
			"g2:1 c3:1 e3:1 c3:1 | f2:1 a2:1 c3:1 a2:1 | g2:1 b2:1 d3:1 g2:1 | c3:1 g2:1 c2:2");

		Play(s, melody, beat, kBrass, kKey);
		Play(s, melody, beat, kBrassEdge, kKey);
		Play(s, melody, beat, kFlute, kKey + 12);
		Play(s, chords, beat, kStrings, kKey);
		Play(s, chords, beat, kStrings, kKey + 12);
		Play(s, bass, beat, kBass, kKey);
		Play(s, bass, beat, kBassEdge, kKey);

		AddCymbal(s, 0.0f, 0.14f);
		for (int bar = 0; bar < kBars; bar++)
		{
			const float b = bar * 4.0f;
			AddTimpani(s, b * beat, 36 + kKey, 0.4f);
			AddTimpani(s, (b + 2.0f) * beat, 43 + kKey, 0.3f);
			AddSideDrum(s, (b + 1.0f) * beat, 0.05f);
			AddSideDrum(s, (b + 3.0f) * beat, 0.05f);
		}
		AddTimpaniRoll(s, 7.0f * 4.0f * beat, beat * 3.0f, 36 + kKey, 0.45f);
		return s.MakeHandle(0.9f, 1);
	}

	//------------------------------//
	// 効果音
	//------------------------------//

	int MakeSE(SE se)
	{
		switch (se)
		{
		case SE::Cursor:
		{
			Synth s(0.06f);
			s.Add({ 0.0f, 0.05f, 880.0f, 880.0f, Wave::Square, 0.22f, 0.25f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Decide:
		{
			Synth s(0.2f);
			s.Add({ 0.0f, 0.06f, 660.0f, 660.0f, Wave::Square, 0.25f, 0.25f });
			s.Add({ 0.06f, 0.12f, 990.0f, 990.0f, Wave::Square, 0.25f, 0.25f, 0.002f, 10.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Cancel:
		{
			Synth s(0.15f);
			s.Add({ 0.0f, 0.14f, 440.0f, 200.0f, Wave::Square, 0.25f, 0.5f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Count:
		{
			Synth s(0.25f);
			s.Add({ 0.0f, 0.22f, 523.0f, 523.0f, Wave::Square, 0.28f, 0.5f, 0.002f, 8.0f });
			s.Add({ 0.0f, 0.22f, 1046.0f, 1046.0f, Wave::Triangle, 0.15f, 0.5f, 0.002f, 8.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Start:
		{
			Synth s(0.6f);
			s.Add({ 0.0f, 0.55f, 1046.0f, 1046.0f, Wave::Square, 0.28f, 0.5f, 0.002f, 4.0f });
			s.Add({ 0.0f, 0.55f, 1568.0f, 1568.0f, Wave::Triangle, 0.18f, 0.5f, 0.002f, 4.0f });
			s.Add({ 0.0f, 0.55f, 523.0f, 523.0f, Wave::Triangle, 0.25f, 0.5f, 0.002f, 4.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Finish:
		{
			//ホイッスル
			Synth s(0.9f);
			s.Add({ 0.0f, 0.15f, 2100.0f, 2100.0f, Wave::Sine, 0.3f, 0.5f, 0.005f, 0.0f, 0.03f });
			s.Add({ 0.2f, 0.6f, 2100.0f, 2100.0f, Wave::Sine, 0.3f, 0.5f, 0.005f, 0.0f, 0.03f });
			s.Add({ 0.0f, 0.15f, 8000.0f, 8000.0f, Wave::Noise, 0.04f });
			s.Add({ 0.2f, 0.6f, 8000.0f, 8000.0f, Wave::Noise, 0.04f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Coin:
		{
			Synth s(0.35f);
			s.Add({ 0.0f, 0.06f, 988.0f, 988.0f, Wave::Square, 0.2f, 0.5f });
			s.Add({ 0.06f, 0.28f, 1319.0f, 1319.0f, Wave::Square, 0.2f, 0.5f, 0.002f, 9.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::BigCoin:
		{
			Synth s(0.6f);
			const float notes[5] = { 1047.0f, 1319.0f, 1568.0f, 2093.0f, 2637.0f };
			for (int i = 0; i < 5; i++)
			{
				s.Add({ i * 0.06f, 0.25f, notes[i], notes[i], Wave::Square, 0.16f, 0.25f, 0.002f, 8.0f });
			}
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Explosion:
		{
			Synth s(0.9f);
			s.Add({ 0.0f, 0.85f, 4000.0f, 150.0f, Wave::Noise, 0.6f, 0.5f, 0.002f, 4.5f });
			s.Add({ 0.0f, 0.45f, 120.0f, 35.0f, Wave::Sine, 0.6f, 0.5f, 0.002f, 5.0f });
			return s.MakeHandle(1.2f, kSEBufferNum);
		}
		case SE::Hit:
		{
			Synth s(0.2f);
			s.Add({ 0.0f, 0.12f, 5000.0f, 900.0f, Wave::Noise, 0.45f, 0.5f, 0.001f, 18.0f });
			s.Add({ 0.0f, 0.12f, 260.0f, 80.0f, Wave::Square, 0.3f, 0.5f, 0.001f, 15.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Pop:
		{
			Synth s(0.35f);
			s.Add({ 0.0f, 0.25f, 9000.0f, 1500.0f, Wave::Noise, 0.7f, 0.5f, 0.001f, 16.0f });
			s.Add({ 0.0f, 0.08f, 900.0f, 120.0f, Wave::Sine, 0.6f, 0.5f, 0.001f, 10.0f });
			return s.MakeHandle(1.3f, kSEBufferNum);
		}
		case SE::Pump:
		{
			Synth s(0.13f);
			s.Add({ 0.0f, 0.12f, 1800.0f, 3500.0f, Wave::Noise, 0.25f, 0.5f, 0.02f, 10.0f });
			s.Add({ 0.0f, 0.06f, 300.0f, 500.0f, Wave::Triangle, 0.12f, 0.5f, 0.002f, 20.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Dash:
		{
			Synth s(0.25f);
			s.Add({ 0.0f, 0.22f, 600.0f, 3000.0f, Wave::Noise, 0.3f, 0.5f, 0.03f, 8.0f });
			s.Add({ 0.0f, 0.15f, 220.0f, 440.0f, Wave::Saw, 0.12f, 0.5f, 0.002f, 12.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Splash:
		{
			Synth s(0.8f);
			s.Add({ 0.0f, 0.7f, 3000.0f, 400.0f, Wave::Noise, 0.45f, 0.5f, 0.005f, 5.0f });
			for (int i = 0; i < 5; i++)
			{
				const float f = 500.0f + i * 170.0f;
				s.Add({ 0.1f + i * 0.08f, 0.06f, f, f * 2.0f, Wave::Sine, 0.18f, 0.5f, 0.002f, 20.0f });
			}
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Bang:
		{
			Synth s(0.45f);
			s.Add({ 0.0f, 0.4f, 7000.0f, 300.0f, Wave::Noise, 0.65f, 0.5f, 0.001f, 9.0f });
			s.Add({ 0.0f, 0.2f, 180.0f, 50.0f, Wave::Square, 0.35f, 0.5f, 0.001f, 12.0f });
			return s.MakeHandle(1.3f, kSEBufferNum);
		}
		case SE::Tick:
		{
			Synth s(0.04f);
			s.Add({ 0.0f, 0.025f, 1500.0f, 1500.0f, Wave::Square, 0.2f, 0.25f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Ready:
		{
			Synth s(0.25f);
			s.Add({ 0.0f, 0.08f, 784.0f, 784.0f, Wave::Triangle, 0.35f });
			s.Add({ 0.08f, 0.14f, 1175.0f, 1175.0f, Wave::Triangle, 0.35f, 0.5f, 0.002f, 8.0f });
			s.Add({ 0.08f, 0.14f, 1175.0f, 1175.0f, Wave::Square, 0.08f, 0.25f, 0.002f, 8.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Fanfare:
		{
			const float step = 0.12f;
			Synth s(2.2f);
			const std::vector<int> lead = { 67, 72, 76, 79, -1, -1, 76, 79, -1, -1, -1, -1, -1, -1, -1, -1 };
			AddNotes(s, lead, step, Wave::Square, 0.18f, 0.95f, 0.25f, 0.0f, 0.0f, 0.006f);
			AddNotes(s, lead, step, Wave::Triangle, 0.15f, 0.95f);
			const std::vector<int> low = { 55, 60, 64, 67, -1, -1, 64, 67, -1, -1, -1, -1, -1, -1, -1, -1 };
			AddNotes(s, low, step, Wave::Triangle, 0.2f, 0.95f);
			AddNotes(s, { 0, 0, 0, 0, 0, 0, 0, 72, -1, -1, -1, -1, -1, -1, -1, -1 }, step, Wave::Square, 0.08f, 0.95f, 0.5f);
			AddNotes(s, { 0, 0, 0, 0, 0, 0, 0, 48, -1, -1, -1, -1, -1, -1, -1, -1 }, step, Wave::Triangle, 0.35f, 0.95f);
			AddKick(s, 0.0f, 0.5f);
			AddKick(s, step * 7.0f, 0.6f);
			s.Add({ step * 7.0f, 1.0f, 11000.0f, 6000.0f, Wave::Noise, 0.18f, 0.5f, 0.002f, 3.0f });
			return s.MakeHandle(1.0f, 1);
		}
		case SE::Drumroll:
		{
			Synth s(2.1f);
			for (float t = 0.0f; t < 2.0f; t += 0.035f)
			{
				const float vol = 0.12f + 0.35f * (t / 2.0f);
				s.Add({ t, 0.05f, 7000.0f, 4000.0f, Wave::Noise, vol, 0.5f, 0.001f, 40.0f });
			}
			return s.MakeHandle(1.0f, 1);
		}
		case SE::Crash:
		{
			Synth s(1.6f);
			s.Add({ 0.0f, 1.5f, 12000.0f, 6000.0f, Wave::Noise, 0.45f, 0.5f, 0.002f, 2.5f });
			AddKick(s, 0.0f, 0.8f);
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Stun:
		{
			Synth s(0.7f);
			for (int i = 0; i < 6; i++)
			{
				const float f = (i % 2 == 0) ? 1400.0f : 1000.0f;
				s.Add({ i * 0.1f, 0.09f, f, f, Wave::Square, 0.15f, 0.25f });
			}
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Jump:
		{
			Synth s(0.15f);
			s.Add({ 0.0f, 0.13f, 300.0f, 750.0f, Wave::Square, 0.2f, 0.25f, 0.002f, 6.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Point:
		{
			Synth s(0.4f);
			s.Add({ 0.0f, 0.08f, 1319.0f, 1319.0f, Wave::Square, 0.2f, 0.25f });
			s.Add({ 0.08f, 0.3f, 1760.0f, 1760.0f, Wave::Square, 0.2f, 0.25f, 0.002f, 6.0f });
			s.Add({ 0.08f, 0.3f, 880.0f, 880.0f, Wave::Triangle, 0.2f, 0.5f, 0.002f, 6.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Whoosh:
		{
			Synth s(0.4f);
			s.Add({ 0.0f, 0.35f, 400.0f, 2400.0f, Wave::Noise, 0.3f, 0.5f, 0.15f, 3.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Fall:
		{
			Synth s(0.7f);
			s.Add({ 0.0f, 0.65f, 1200.0f, 150.0f, Wave::Square, 0.18f, 0.5f, 0.01f, 1.5f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Buzzer:
		{
			Synth s(0.5f);
			s.Add({ 0.0f, 0.45f, 110.0f, 105.0f, Wave::Saw, 0.35f });
			s.Add({ 0.0f, 0.45f, 165.0f, 160.0f, Wave::Square, 0.15f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Signal:
		{
			Synth s(0.5f);
			s.Add({ 0.0f, 0.45f, 1760.0f, 1760.0f, Wave::Square, 0.25f, 0.5f, 0.001f, 6.0f });
			s.Add({ 0.0f, 0.45f, 2637.0f, 2637.0f, Wave::Triangle, 0.2f, 0.5f, 0.001f, 6.0f });
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::Shiny:
		{
			//きらきらと下りる音のあとに、ゆれる和音
			Synth s(1.6f);
			const float glints[6] = { 3136.0f, 2637.0f, 2349.0f, 3520.0f, 2794.0f, 4186.0f };
			for (int i = 0; i < 6; i++)
			{
				s.Add({ i * 0.055f, 0.3f, glints[i], glints[i], Wave::Sine, 0.2f, 0.5f, 0.001f, 9.0f });
				s.Add({ i * 0.055f, 0.12f, glints[i] * 2.0f, glints[i] * 2.0f, Wave::Triangle, 0.05f, 0.5f, 0.001f, 20.0f });
			}
			const float chord[3] = { 1047.0f, 1319.0f, 1568.0f };
			for (int i = 0; i < 3; i++)
			{
				Tone t = { 0.35f + i * 0.04f, 1.2f, chord[i], chord[i], Wave::Sine, 0.13f, 0.5f, 0.01f, 2.5f, 0.012f };
				s.Add(t);
				t.f0 = t.f1 = chord[i] * 2.0f;
				t.vol = 0.05f;
				s.Add(t);
			}
			return s.MakeHandle(1.0f, kSEBufferNum);
		}
		case SE::ShinyJackpot:
		{
			//さきばれ ふう(シュッと ためて、キュイン！と するどく なる)
			Synth s(0.9f);

			//シュッ(ためる ような ぎゃく まわしの おと)
			s.Add({ 0.0f, 0.1f, 0.0f, 0.0f, Wave::Noise, 0.12f, 0.5f, 0.09f, 0.0f, 0.0f, 0.005f, 3500.0f });
			s.Add({ 0.0f, 0.1f, 300.0f, 900.0f, Wave::Sine, 0.12f, 0.5f, 0.09f, 0.0f, 0.0f, 0.005f });

			//キュ(いっしゅんで かけあがる)
			s.Add({ 0.1f, 0.07f, 1300.0f, 4600.0f, Wave::Square, 0.18f, 0.3f, 0.002f, 0.0f, 0.0f, 0.005f, 6500.0f });
			s.Add({ 0.1f, 0.07f, 650.0f, 2300.0f, Wave::Saw, 0.12f, 0.5f, 0.002f, 0.0f, 0.0f, 0.005f, 4000.0f });

			//イン！(こまかく ふるえながら のびる)
			s.Add({ 0.17f, 0.55f, 4600.0f, 4300.0f, Wave::Square, 0.15f, 0.3f, 0.002f, 4.5f, 0.035f, 0.08f, 6500.0f });
			s.Add({ 0.17f, 0.55f, 2300.0f, 2150.0f, Wave::Saw, 0.1f, 0.5f, 0.002f, 4.5f, 0.035f, 0.08f, 4000.0f });
			s.Add({ 0.17f, 0.55f, 1150.0f, 1075.0f, Wave::Sine, 0.16f, 0.5f, 0.002f, 4.0f, 0.03f, 0.08f });

			//キン(きんぞくの ような ひびき)
			s.Add({ 0.17f, 0.5f, 3170.0f, 3170.0f, Wave::Sine, 0.08f, 0.5f, 0.001f, 6.0f });
			s.Add({ 0.17f, 0.4f, 8760.0f, 8760.0f, Wave::Sine, 0.04f, 0.5f, 0.001f, 9.0f });
			return s.MakeHandle(1.0f, 2);
		}
		default:
			return -1;
		}
	}
}

Sound& Sound::GetInstance()
{
	static Sound instance;
	return instance;
}

void Sound::Init()
{
	for (int i = 0; i < static_cast<int>(SE::Num); i++)
	{
		seHandles_[i] = MakeSE(static_cast<SE>(i));
		ChangeVolumeSoundMem(seVolume_, seHandles_[i]);
	}

	bgmHandles_[static_cast<int>(BGM::Title)] = MakeTitleBgm();
	bgmHandles_[static_cast<int>(BGM::Village)] = MakeVillageBgm();
	bgmHandles_[static_cast<int>(BGM::Dungeon)] = MakeDungeonBgm();
	bgmHandles_[static_cast<int>(BGM::Boss)] = MakeBossBgm();
	bgmHandles_[static_cast<int>(BGM::Victory)] = MakeVictoryBgm();
	for (int handle : bgmHandles_)
	{
		ChangeVolumeSoundMem(bgmVolume_, handle);
	}
}

void Sound::Terminate()
{
	InitSoundMem();
}

void Sound::PlaySE(SE se, float pitch)
{
	const int handle = seHandles_[static_cast<int>(se)];
	if (handle < 0) return;

	if (pitch == 1.0f) SetFrequencySoundMem(-1, handle);
	else SetFrequencySoundMem(static_cast<int>(kSampleRate * pitch), handle);

	PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

void Sound::PlayBGM(BGM bgm)
{
	if (bgm == currentBgm_) return;

	StopBGM();
	currentBgm_ = bgm;
	if (bgm == BGM::None) return;

	PlaySoundMem(bgmHandles_[static_cast<int>(bgm)], DX_PLAYTYPE_LOOP);
}

void Sound::StopBGM()
{
	if (currentBgm_ != BGM::None)
	{
		StopSoundMem(bgmHandles_[static_cast<int>(currentBgm_)]);
	}
	currentBgm_ = BGM::None;
}

void Sound::SetVolume(int bgm, int se)
{
	bgmVolume_ = kBgmVolume * bgm / 10;
	seVolume_ = kSEVolume * se / 10;
	for (int handle : seHandles_) ChangeVolumeSoundMem(seVolume_, handle);
	for (int handle : bgmHandles_) ChangeVolumeSoundMem(bgmVolume_, handle);
}
