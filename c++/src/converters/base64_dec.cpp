#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.bin.dec" : conv::replace_extension(input_name, "dec");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    std::vector<unsigned char> out_buf(input.size());
    int outl = 0;
    int total = 0;

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64 input");
    }
    total += outl;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total, &outl) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 input");
    }
    total += outl;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
