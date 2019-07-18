#include "http.h"
#include <cstring>

std::unordered_map<std::string, std::string> parse_header(const char *msg, const char *msg_end) {
    const char *head = msg;
    const char *tail = msg;

    std::unordered_map<std::string, std::string> http_request;

    // Find request type
    while (tail != msg_end && *tail != ' ')
        ++tail;
    http_request["Type"] = std::string(head, tail);

    // Find path
    while (tail != msg_end && *tail == ' ')
        ++tail;
    head = tail;
    while (tail != msg_end && *tail != ' ')
        ++tail;
    http_request["Path"] = std::string(head, tail);

    // Find HTTP version
    while (tail != msg_end && *tail == ' ')
        ++tail;
    head = tail;
    while (tail != msg_end && *tail != '\r')
        ++tail;
    http_request["Version"] = std::string(head, tail);
    if (tail != msg_end)
        ++tail;  // skip '\r'
    // TODO: what about the trailing '\n'?

    // Map all headers from a key to a value
    head = tail;
    while (head < msg_end && *head != '\r') {
        tail = head;
        while (tail < msg_end && *tail != '\r')
            ++tail;
        const char *colon = (const char *)memchr(head, ':', tail - head);
        if (colon == NULL) {
            // TODO: malformed headers, what should happen?
            break;
        }
        const char *value = colon + 1;
        while (value < tail && *value == ' ')
            ++value;
        if (head < colon && value < tail && tail <= msg_end) {
            http_request[std::string(head, colon)] = std::string(value, tail);
        }
        head = tail + 1;
        while (head < msg_end && (*head == '\n' || *head == '\r' || *head == ' '))
            head++;
        //printf("%d\n", head - msg);
        // TODO: what about the trailing '\n'?
    }

    return http_request;
}