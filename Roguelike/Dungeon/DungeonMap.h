#pragma once
#include<vector>
#include<array>

/// <summary>
/// マスの座標
/// </summary>
struct Cell
{
	int x = 0;
	int y = 0;

	bool operator==(const Cell& c) const { return x == c.x && y == c.y; }
	bool operator!=(const Cell& c) const { return !(*this == c); }
	Cell operator+(const Cell& c) const { return { x + c.x, y + c.y }; }
};

enum class Tile
{
	Wall,
	Floor,
	Stairs,
};

struct Room
{
	int x;
	int y;
	int w;
	int h;

	Cell Center() const { return { x + w / 2, y + h / 2 }; }
	bool Contains(const Cell& c) const { return c.x >= x && c.x < x + w && c.y >= y && c.y < y + h; }
};

/// <summary>
/// 部屋と通路のダンジョン
/// </summary>
class DungeonMap
{
public:
	static constexpr int kWidth = 56;
	static constexpr int kHeight = 36;

	/// <summary>
	/// ランダムに作る
	/// </summary>
	void Generate();

	Tile Get(const Cell& c) const;
	bool IsWalkable(const Cell& c) const;
	bool IsInside(const Cell& c) const;

	/// <summary>
	/// 視界の更新(見えたマスは記憶する)
	/// </summary>
	void UpdateVision(const Cell& eye, int radius);

	bool IsVisible(const Cell& c) const;
	bool IsExplored(const Cell& c) const;

	/// <summary>
	/// まわりを 見たことに する(かいだんの けはい)
	/// </summary>
	void Reveal(const Cell& center, int radius);

	/// <summary>
	/// 2点間が見通せるか
	/// </summary>
	bool HasLineOfSight(const Cell& a, const Cell& b) const;

	/// <summary>
	/// 目的地へ向かう最初の1歩(通れない時は自分の位置)
	/// </summary>
	/// <param name="blocked">通れないマスの判定</param>
	template<class F>
	Cell NextStep(const Cell& from, const Cell& to, F blocked, int maxDist = 30) const;

	/// <summary>
	/// 部屋の中のランダムな床(blockedは避ける)
	/// </summary>
	template<class F>
	Cell RandomFloor(F blocked, int roomIndex = -1) const;

	const std::vector<Room>& Rooms() const { return rooms_; }
	Cell StartCell() const { return start_; }
	Cell StairsCell() const { return stairs_; }

private:
	void Carve(int x, int y);

	std::array<std::array<Tile, kWidth>, kHeight> tiles_ = {};
	std::array<std::array<bool, kWidth>, kHeight> visible_ = {};
	std::array<std::array<bool, kWidth>, kHeight> explored_ = {};
	std::vector<Room> rooms_;
	Cell start_;
	Cell stairs_;
};

#include "DungeonMap.inl"
