#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    // Generate deterministic output name based on input name stripping trailing extension
    std::string out_name;
    if (input_name.empty()) {
        out_name = "input.dec.bin";
    } else {
        std::string base = conv::basename_no_ext(input_name);
        out_name = conv::replace_extension(base, "dec.bin");
    }

    if (input.empty() || (input.size() == 1 && input[0] == ' ')) {
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

    // required size over-estimate
    std::size_t required_len = 3 * (input.size() / 4) + 3;
    std::vector<unsigned char> out_buf(required_len);
    int outl = 0;

    int ret = EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size());
    if (ret < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    int total_len = outl;

    ret = EVP_DecodeFinal(ctx, out_buf.data() + total_len, &outl);
    if (ret < 0) {
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
