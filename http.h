
#include <stdio.h>
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, std::string> parse_header(const char *msg, const char *msg_end);