#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.bin" : conv::replace_extension(input_name, "bin");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Maximum possible output length is equal to input length
    std::vector<unsigned char> out_buf(input.size());
    int out_len = 0;
    int final_len = 0;

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: Invalid Base64 input");
    }

    if (EVP_DecodeFinal(ctx, out_buf.data() + out_len, &final_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: Invalid Base64 padding or malformed input");
    }

    EVP_ENCODE_CTX_free(ctx);

    std::string dec_str(reinterpret_cast<char*>(out_buf.data()), out_len + final_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = dec_str;

    return { artifact };
}
