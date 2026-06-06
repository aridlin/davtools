#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.b64.txt.decoded.bin" : conv::replace_extension(input_name, "decoded.bin");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    // Padding + max theoretical output length
    std::size_t max_out_len = (input.size() * 3) / 4 + 10;
    std::vector<unsigned char> out_buf(max_out_len);

    int outl = 0;
    int total_len = 0;

    int rv = EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size());
    if (rv < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += outl;

    rv = EVP_DecodeFinal(ctx, out_buf.data() + total_len, &outl);
    if (rv < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += outl;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.b64.txt";

    // Check if it ends in .b64.txt, if so just strip it and maybe add .bin.
    // For simplicity, we just use replace_extension to .decoded.bin as per plan.
    out_name = conv::replace_extension(out_name, "decoded.bin");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
