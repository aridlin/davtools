#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <memory>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.b64.txt.bin" : conv::replace_extension(input_name, "bin");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    std::unique_ptr<EVP_ENCODE_CTX, decltype(&EVP_ENCODE_CTX_free)> ctx(EVP_ENCODE_CTX_new(), EVP_ENCODE_CTX_free);
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx.get());

    std::size_t max_out_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(max_out_len);
    int out_len = 0;
    int total_len = 0;

    if (EVP_DecodeUpdate(ctx.get(), out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx.get(), out_buf.data() + total_len, &out_len) < 0) {
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += out_len;

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    std::string out_name = input_name.empty() ? "input.b64.txt.bin" : conv::replace_extension(input_name, "bin");

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
