#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.empty()) {
        out_name = "input";
    }

    // Strip .b64.txt if present
    if (out_name.size() > 8 && out_name.substr(out_name.size() - 8) == ".b64.txt") {
        out_name = out_name.substr(0, out_name.size() - 8);
    } else {
        out_name += ".bin";
    }

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // EVP_DecodeUpdate needs enough space, max 3 bytes output per 4 bytes input
    std::size_t required_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(required_len);
    int total_len = 0;
    int out_len = 0;

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 padding");
    }
    total_len += out_len;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
