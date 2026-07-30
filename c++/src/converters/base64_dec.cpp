#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.ends_with(".b64.txt")) {
        out_name = out_name.substr(0, out_name.length() - 8);
    } else {
        if (out_name.empty()) out_name = "input.b64.txt";
        out_name += ".bin";
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

    // Max possible output size: (len * 3) / 4. For safety, add some buffer.
    std::vector<std::uint8_t> out_buf(input.size() * 3 / 4 + 4);
    int out_len = 0;
    int total_len = 0;

    // EVP_DecodeUpdate processes chunks and ignores whitespace
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
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
