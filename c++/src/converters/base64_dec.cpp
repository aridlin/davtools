#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name.empty() ? "input.dec.bin" : conv::replace_extension(input_name, "dec.bin");

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Calculate required output size: 3 * ((len + 3) / 4)
    std::size_t required_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(required_len);

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    int outl = 0;
    int total_len = 0;

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += outl;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &outl) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += outl;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
