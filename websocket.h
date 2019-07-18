
#include <stdio.h>
#include <algorithm>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>
#include <unordered_map>
#include <vector>

int ws_compute_handshake(const char *key, char *out, size_t *out_sz);

int generate_ws_frame(char *buf, std::string msg);
