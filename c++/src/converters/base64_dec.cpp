#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.bin" : conv::replace_extension(input_name, "dec.bin");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to allocate EVP_ENCODE_CTX");
    }

    EVP_DecodeInit(ctx);

    // Max possible output size is input.size() (3 bytes for every 4 base64 chars + ignoring newlines)
    std::vector<unsigned char> out_buf(input.size());
    int out_len = 0;
    int total_len = 0;

    int res = EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size());
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: Invalid Base64 input");
    }
    total_len += out_len;

    res = EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len);
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: Invalid Base64 padding");
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
