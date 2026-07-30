#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.b64.bin" : conv::replace_extension(input_name, "bin");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Initialize decode context
    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    // Max possible output size
    std::vector<unsigned char> out_buf(input.size());
    int out_len = 0;
    int total_len = 0;

    // Use streaming API to handle whitespace and newlines safely
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64 input");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 padding");
    }
    total_len += out_len;

    EVP_ENCODE_CTX_free(ctx);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.b64.txt";
    out_name = conv::replace_extension(out_name, "bin");

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    // Decoded data might be text or binary, default to application/octet-stream
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
