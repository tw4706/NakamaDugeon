#pragma once
#include "MathUtil.h"
#include<queue>
#include<cstdlib>

template<class F>
Cell DungeonMap::NextStep(const Cell& from, const Cell& to, F blocked, int maxDist) const
{
	if (from == to) return from;

	//目的地から幅優先で距離を測る
	static int dist[kHeight][kWidth];
	for (auto& row : dist) for (int& d : row) d = -1;

	std::queue<Cell> open;
	dist[to.y][to.x] = 0;
	open.push(to);
	const Cell dirs[8] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };

	while (!open.empty())
	{
		const Cell c = open.front();
		open.pop();
		if (c == from) break;
		if (dist[c.y][c.x] >= maxDist) continue;

		for (const Cell& d : dirs)
		{
			const Cell n = c + d;
			if (!IsWalkable(n) || dist[n.y][n.x] >= 0) continue;

			//斜めは角を抜けられない
			if (d.x != 0 && d.y != 0 && (!IsWalkable({ c.x + d.x, c.y }) || !IsWalkable({ c.x, c.y + d.y }))) continue;

			//ほかのユニットは避ける(出発点と目的地は除く)
			if (n != from && blocked(n)) continue;

			dist[n.y][n.x] = dist[c.y][c.x] + 1;
			open.push(n);
		}
	}

	//距離が一番小さい隣へ
	Cell best = from;
	int bestDist = 1 << 20;
	for (const Cell& d : dirs)
	{
		const Cell n = from + d;
		if (!IsInside(n) || dist[n.y][n.x] < 0) continue;
		if (d.x != 0 && d.y != 0 && (!IsWalkable({ from.x + d.x, from.y }) || !IsWalkable({ from.x, from.y + d.y }))) continue;
		if (n != to && blocked(n)) continue;
		if (dist[n.y][n.x] < bestDist)
		{
			bestDist = dist[n.y][n.x];
			best = n;
		}
	}
	return best;
}

template<class F>
Cell DungeonMap::RandomFloor(F blocked, int roomIndex) const
{
	for (int tries = 0; tries < 200; tries++)
	{
		const int ri = roomIndex >= 0 ? roomIndex : MathUtil::RandI(0, static_cast<int>(rooms_.size()) - 1);
		const Room& r = rooms_[ri];
		const Cell c = { MathUtil::RandI(r.x, r.x + r.w - 1), MathUtil::RandI(r.y, r.y + r.h - 1) };
		if (Get(c) == Tile::Floor && !blocked(c)) return c;
	}
	return start_;
}
