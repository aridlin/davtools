#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>
#include <algorithm>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.bin.b64_dec.txt" : input_name + ".b64_dec.txt";
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Use EVP streaming API to handle whitespace and padding correctly
    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    // Calculate maximum required output size
    std::size_t max_out_len = 3 * ((input.size() + 3) / 4);
    std::vector<unsigned char> out_buf(max_out_len);

    int out_len = 0;
    int total_len = 0;

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }
    total_len += out_len;

    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }
    total_len += out_len;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.bin";
    out_name = conv::replace_extension(out_name, "bin"); // Try to replace .txt or similar

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
