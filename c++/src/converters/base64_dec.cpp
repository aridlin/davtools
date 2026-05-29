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
    out_name = conv::basename_no_ext(out_name) + ".bin";

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // output buffer can be up to the same size as the input
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
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }

    if (EVP_DecodeFinal(ctx, out_buf.data() + out_len, &final_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }

    EVP_ENCODE_CTX_free(ctx);

    std::size_t total_len = out_len + final_len;

    std::string out_data(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = out_data;

    return { artifact };
}
