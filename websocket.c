#include "websocket.h"

#include <netinet/in.h>
#include <cstring>
#include <string>

#include "base64.h"
#include "sha1.h"

int ws_compute_handshake(const char *key, char *out, size_t *out_sz) {
    unsigned char *buffer, sha1_output[20];
    char magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    SHA1Context ctx;
    base64_encodestate b64_ctx;
    int pos, i;

    // websocket handshake
    size_t key_sz = key ? strlen(key) : 0, buffer_sz = key_sz + sizeof(magic) - 1;
    buffer = (unsigned char *)calloc(buffer_sz, 1);

    // concatenate key and guid in buffer
    memcpy(buffer, key, key_sz);
    memcpy(buffer + key_sz, magic, sizeof(magic) - 1);

    // compute sha-1
    SHA1Reset(&ctx);
    SHA1Input(&ctx, buffer, buffer_sz);
    SHA1Result(&ctx);
    for (i = 0; i < 5; ++i) {  // put in correct byte order before memcpy.
        ctx.Message_Digest[i] = ntohl(ctx.Message_Digest[i]);
    }
    memcpy(sha1_output, (unsigned char *)ctx.Message_Digest, 20);

    // encode `sha1_output' in base 64, into `out'.
    base64_init_encodestate(&b64_ctx);
    pos = base64_encode_block((const char *)sha1_output, 20, out, &b64_ctx);
    base64_encode_blockend(out + pos, &b64_ctx);

    // compute length, without \n
    *out_sz = strlen(out);
    if (out[*out_sz - 1] == '\n')
        (*out_sz)--;

    free(buffer);

    return 0;
}

int generate_ws_frame(char *buf, std::string msg) {
    const char *cmsg = msg.c_str();
    int cmsg_len = msg.length();

    int payload_offset = 2;

    buf[0] = 0b10000001;
    if (cmsg_len > 125) {
        payload_offset += 2;
        buf[1] = (char)(126 & 0b01111111);
        buf[3] = (char)(cmsg_len >> 8);
        buf[4] = (char)(cmsg_len);
    } else {
        buf[1] = (char)(cmsg_len & 0b01111111);
    }

    for (int cmsg_i = 0; cmsg_i < cmsg_len; cmsg_i++) {
        buf[payload_offset + cmsg_i] = cmsg[cmsg_i];
    }

    buf[payload_offset + cmsg_len] = '\0';

    return payload_offset + cmsg_len;
}
