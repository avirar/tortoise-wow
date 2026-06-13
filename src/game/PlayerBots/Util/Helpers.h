#ifndef PLAYERBOT_HELPERS_H
#define PLAYERBOT_HELPERS_H

#include <string>
#include <vector>

char* strstri(char const* haystack, char const* needle);
std::string& ltrim(std::string& s);
std::string& rtrim(std::string& s);
std::string& trim(std::string& s);
void split(std::vector<std::string>& dest, std::string const str, char const* delim);
std::vector<std::string>& split(std::string const s, char delim, std::vector<std::string>& elems);
std::vector<std::string> split(std::string const s, char delim);

#endif
