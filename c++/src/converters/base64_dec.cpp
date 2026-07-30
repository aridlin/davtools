#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.b64.txt";

    // Strip trailing .b64.txt or .b64 or .txt or append .dec
    if (out_name.size() > 8 && out_name.substr(out_name.size() - 8) == ".b64.txt") {
        out_name = out_name.substr(0, out_name.size() - 8);
    } else if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".b64") {
        out_name = out_name.substr(0, out_name.size() - 4);
    } else if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".txt") {
        out_name = out_name.substr(0, out_name.size() - 4);
    } else {
        out_name += ".dec";
    }

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    std::vector<unsigned char> out_buf(input.size());
    int out_len = 0;
    int total_len = 0;

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64 input");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 padding or length");
    }
    total_len += out_len;

    EVP_ENCODE_CTX_free(ctx);

    std::string dec_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    // Base64 could decode to anything, fallback to octet-stream
    // Or we could try to guess from the new extension if possible, but let's just use octet-stream unless we know.
    // Actually we don't have a robust way to guess without writing it. So let's keep it simple.
    artifact.content_type = "application/octet-stream";
    artifact.data = dec_str;

    return { artifact };
}
