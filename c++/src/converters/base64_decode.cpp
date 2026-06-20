#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name;
    if (input_name.size() > 8 && input_name.substr(input_name.size() - 8) == ".b64.txt") {
        out_name = input_name.substr(0, input_name.size() - 8);
    } else {
        out_name = input_name.empty() ? "input.b64.bin" : input_name + ".bin";
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

    std::vector<unsigned char> out_buf(input.size());
    int outl = 0;
    int total = 0;

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64 input");
    }
    total += outl;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total, &outl) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 padding or length");
    }
    total += outl;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = std::move(decoded_str);

    return { artifact };
}
