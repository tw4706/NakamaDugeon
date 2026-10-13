#include "DungeonMap.h"
#include "MathUtil.h"
#include<algorithm>
#include<cmath>

using namespace MathUtil;

namespace
{
	constexpr int kRoomTry = 60;
	constexpr int kMaxRoom = 9;
	constexpr int kRoomMin = 4;
	constexpr int kRoomMax = 10;
}

void DungeonMap::Generate()
{
	for (auto& row : tiles_) row.fill(Tile::Wall);
	for (auto& row : visible_) row.fill(false);
	for (auto& row : explored_) row.fill(false);
	rooms_.clear();

	//重ならないように部屋を置く
	for (int i = 0; i < kRoomTry && static_cast<int>(rooms_.size()) < kMaxRoom; i++)
	{
		Room r;
		r.w = RandI(kRoomMin, kRoomMax);
		r.h = RandI(kRoomMin, kRoomMax - 3);
		r.x = RandI(1, kWidth - r.w - 2);
		r.y = RandI(1, kHeight - r.h - 2);

		bool isOverlap = false;
		for (const Room& o : rooms_)
		{
			if (r.x - 2 < o.x + o.w && r.x + r.w + 2 > o.x && r.y - 2 < o.y + o.h && r.y + r.h + 2 > o.y)
			{
				isOverlap = true;
				break;
			}
		}
		if (isOverlap) continue;

		for (int y = r.y; y < r.y + r.h; y++)
		{
			for (int x = r.x; x < r.x + r.w; x++) Carve(x, y);
		}
		rooms_.push_back(r);
	}

	//左から順に並べて通路でつなぐ
	std::sort(rooms_.begin(), rooms_.end(), [](const Room& a, const Room& b) { return a.x < b.x; });
	for (size_t i = 1; i < rooms_.size(); i++)
	{
		const Cell a = rooms_[i - 1].Center();
		const Cell b = rooms_[i].Center();
		if (RandI(0, 1) == 0)
		{
			for (int x = std::min(a.x, b.x); x <= std::max(a.x, b.x); x++) Carve(x, a.y);
			for (int y = std::min(a.y, b.y); y <= std::max(a.y, b.y); y++) Carve(b.x, y);
		}
		else
		{
			for (int y = std::min(a.y, b.y); y <= std::max(a.y, b.y); y++) Carve(a.x, y);
			for (int x = std::min(a.x, b.x); x <= std::max(a.x, b.x); x++) Carve(x, b.y);
		}
	}

	//たまに遠い部屋同士もつなぐ
	if (rooms_.size() >= 4)
	{
		const Cell a = rooms_[0].Center();
		const Cell b = rooms_[rooms_.size() / 2].Center();
		for (int x = std::min(a.x, b.x); x <= std::max(a.x, b.x); x++) Carve(x, a.y);
		for (int y = std::min(a.y, b.y); y <= std::max(a.y, b.y); y++) Carve(b.x, y);
	}

	start_ = rooms_.front().Center();
	stairs_ = rooms_.back().Center();
	tiles_[stairs_.y][stairs_.x] = Tile::Stairs;
}

void DungeonMap::Carve(int x, int y)
{
	if (x <= 0 || y <= 0 || x >= kWidth - 1 || y >= kHeight - 1) return;
	tiles_[y][x] = Tile::Floor;
}

bool DungeonMap::IsInside(const Cell& c) const
{
	return c.x >= 0 && c.y >= 0 && c.x < kWidth && c.y < kHeight;
}

Tile DungeonMap::Get(const Cell& c) const
{
	if (!IsInside(c)) return Tile::Wall;
	return tiles_[c.y][c.x];
}

bool DungeonMap::IsWalkable(const Cell& c) const
{
	return Get(c) != Tile::Wall;
}

bool DungeonMap::HasLineOfSight(const Cell& a, const Cell& b) const
{
	//ブレゼンハムで壁を探す
	int x0 = a.x, y0 = a.y;
	const int dx = std::abs(b.x - a.x), dy = -std::abs(b.y - a.y);
	const int sx = a.x < b.x ? 1 : -1, sy = a.y < b.y ? 1 : -1;
	int err = dx + dy;
	while (true)
	{
		if (x0 == b.x && y0 == b.y) return true;
		if (!(x0 == a.x && y0 == a.y) && Get({ x0, y0 }) == Tile::Wall) return false;
		const int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

void DungeonMap::UpdateVision(const Cell& eye, int radius)
{
	for (auto& row : visible_) row.fill(false);

	//部屋の中なら部屋全体が見える
	for (const Room& r : rooms_)
	{
		if (!r.Contains(eye)) continue;
		for (int y = r.y - 1; y <= r.y + r.h; y++)
		{
			for (int x = r.x - 1; x <= r.x + r.w; x++)
			{
				if (!IsInside({ x, y })) continue;
				visible_[y][x] = true;
				explored_[y][x] = true;
			}
		}
	}

	for (int y = eye.y - radius; y <= eye.y + radius; y++)
	{
		for (int x = eye.x - radius; x <= eye.x + radius; x++)
		{
			const Cell c = { x, y };
			if (!IsInside(c)) continue;
			if ((x - eye.x) * (x - eye.x) + (y - eye.y) * (y - eye.y) > radius * radius) continue;
			if (!HasLineOfSight(eye, c)) continue;
			visible_[y][x] = true;
			explored_[y][x] = true;
		}
	}
}

bool DungeonMap::IsVisible(const Cell& c) const
{
	return IsInside(c) && visible_[c.y][c.x];
}

bool DungeonMap::IsExplored(const Cell& c) const
{
	return IsInside(c) && explored_[c.y][c.x];
}

void DungeonMap::Reveal(const Cell& center, int radius)
{
	for (int dy = -radius; dy <= radius; dy++)
	{
		for (int dx = -radius; dx <= radius; dx++)
		{
			const Cell c = { center.x + dx, center.y + dy };
			if (IsInside(c)) explored_[c.y][c.x] = true;
		}
	}
}
