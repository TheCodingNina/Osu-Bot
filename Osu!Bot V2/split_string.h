#pragma once
#include <vector>

inline std::vector<std::wstring> split_string(const std::wstring &str, const std::wstring &delim = L" ", size_t pos = 0) {
	std::vector<std::wstring> out;
	if (pos >= str.size()) return out;

	size_t currentPos = 0;
	while (str.find(delim, pos + 1) != std::wstring::npos) {
		out.push_back(str.substr(currentPos, str.find(delim, pos + 1) - currentPos));
		pos = str.find(delim, pos + 1) + 1;
		currentPos = pos;
	}
	out.push_back(str.substr(pos));

	return out;
}