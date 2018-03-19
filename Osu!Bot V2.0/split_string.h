#pragma once
#include <vector>

inline std::vector<std::string> split_string(const std::string &str, const std::string &delim = " ", size_t pos = 0) {
	std::vector<std::string> out;
	if (pos >= str.size()) return out;

	size_t currentPos = 0;
	while (str.find(delim, pos + 1) != std::string::npos) {
		out.push_back(str.substr(currentPos, str.find(delim, pos + 1) - currentPos));
		pos = str.find(delim, pos + 1) + 1;
		currentPos = pos;
	}
	out.push_back(str.substr(pos));

	return out;
}