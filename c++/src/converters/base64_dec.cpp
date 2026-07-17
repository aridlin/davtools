#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

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

    std::size_t max_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(max_len);

    std::unique_ptr<EVP_ENCODE_CTX, decltype(&EVP_ENCODE_CTX_free)> ctx(EVP_ENCODE_CTX_new(), EVP_ENCODE_CTX_free);
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx.get());

    int outl = 0;
    int total_len = 0;

    if (EVP_DecodeUpdate(ctx.get(), out_buf.data(), &outl, input.data(), input.size()) < 0) {
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += outl;

    if (EVP_DecodeFinal(ctx.get(), out_buf.data() + total_len, &outl) < 0) {
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += outl;

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
