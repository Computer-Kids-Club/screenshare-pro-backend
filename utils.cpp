#include "utils.h"

std::string get_file_contents(const char *filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in) {
        return (std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }
    throw(errno);
}