#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.bin" : conv::basename_no_ext(input_name);
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    std::vector<unsigned char> out_buf(input.size());

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }
    EVP_DecodeInit(ctx);

    int outl = 0;
    int totall = 0;

    int res = EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size());
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    totall += outl;

    res = EVP_DecodeFinal(ctx, out_buf.data() + totall, &outl);
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    totall += outl;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), totall);

    std::string out_name = input_name.empty() ? "input.bin" : conv::basename_no_ext(input_name);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
