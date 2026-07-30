#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "decoded.bin" : conv::basename_no_ext(input_name);
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

    std::vector<uint8_t> out_buf(input.size());
    int out_len = 0;
    int total_len = 0;

    int res = EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size());
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += out_len;

    int final_len = 0;
    res = EVP_DecodeFinal(ctx, out_buf.data() + total_len, &final_len);
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += final_len;

    out_buf.resize(total_len);
    EVP_ENCODE_CTX_free(ctx);

    std::string out_name = input_name.empty() ? "decoded.bin" : conv::basename_no_ext(input_name);

    std::string decoded_str(out_buf.begin(), out_buf.end());

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
