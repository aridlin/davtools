#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.bin" : input_name;

    // Strip trailing .b64.txt or .b64 or .txt if it ends with .b64.txt
    if (out_name.size() > 8 && out_name.substr(out_name.size() - 8) == ".b64.txt") {
        out_name = out_name.substr(0, out_name.size() - 8);
    } else if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".b64") {
        out_name = out_name.substr(0, out_name.size() - 4);
    } else if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".txt") {
        out_name = out_name.substr(0, out_name.size() - 4);
    }

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Conservative bound for base64 decode buffer size
    std::size_t max_out_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(max_out_len);

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    int out_len1 = 0;
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len1, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }

    int out_len2 = 0;
    if (EVP_DecodeFinal(ctx, out_buf.data() + out_len1, &out_len2) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }

    EVP_ENCODE_CTX_free(ctx);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = std::string(reinterpret_cast<char*>(out_buf.data()), out_len1 + out_len2);

    return { artifact };
}
