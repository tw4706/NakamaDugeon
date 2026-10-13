#include "UpdateLog.h"
#include<fstream>
#include<Windows.h>

namespace
{
	std::vector<UpdateEntry> entries;

	std::wstring ToWide(const std::string& s)
	{
		const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
		std::wstring w(len > 0 ? len - 1 : 0, L'\0');
		if (len > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), len);
		return w;
	}
}

bool UpdateLog::Load(const char* path)
{
	entries.clear();
	std::ifstream file(path);
	if (!file) return false;

	std::string line;
	std::getline(file, line);	//見出し
	while (std::getline(file, line))
	{
		if (!line.empty() && line.back() == '\r') line.pop_back();
		const size_t comma = line.find(',');
		if (comma == std::string::npos) continue;

		//1列目がバージョン、残りが本文(本文にカンマがあってもよい)
		const std::wstring version = ToWide(line.substr(0, comma));
		const std::wstring text = ToWide(line.substr(comma + 1));
		if (version.empty() || text.empty()) continue;
		if (entries.empty() || entries.back().version != version) entries.push_back({ version, {} });
		entries.back().lines.push_back(text);
	}
	return !entries.empty();
}

const std::vector<UpdateEntry>& UpdateLog::All()
{
	return entries;
}

std::wstring UpdateLog::Latest()
{
	return entries.empty() ? std::wstring() : entries.back().version;
}

bool UpdateLog::IsPatch(const std::wstring& version)
{
	int dots = 0;
	for (wchar_t c : version) if (c == L'.') dots++;
	return dots >= 2;
}

int UpdateLog::VersionCode(const std::wstring& version)
{
	int parts[3] = {};
	int index = 0;
	for (wchar_t c : version)
	{
		if (c == L'.') { if (++index >= 3) break; continue; }
		if (c >= L'0' && c <= L'9') parts[index] = parts[index] * 10 + (c - L'0');
	}
	return parts[0] * 10000 + parts[1] * 100 + parts[2];
}
