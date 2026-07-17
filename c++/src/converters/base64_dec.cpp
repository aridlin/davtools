#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.dec" : conv::replace_extension(input_name, "dec");
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
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }

    int outl2 = 0;
    if (EVP_DecodeFinal(ctx, out_buf.data() + outl, &outl2) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded(reinterpret_cast<char*>(out_buf.data()), outl + outl2);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.bin";

    // Attempt to strip `.b64.txt` if it exists
    if (out_name.size() > 8 && out_name.substr(out_name.size() - 8) == ".b64.txt") {
        out_name = out_name.substr(0, out_name.size() - 8);
    } else {
        out_name += ".dec";
    }

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded;

    return { artifact };
}
