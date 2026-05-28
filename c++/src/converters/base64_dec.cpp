#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.b64.txt.dec.bin" : input_name + ".dec.bin";
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    if (input.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("input too large for OpenSSL API");
    }

    std::vector<unsigned char> out_buf(input.size() * 3 / 4 + 1);

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    int out_len1 = 0;
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len1, reinterpret_cast<const unsigned char*>(input.data()), static_cast<int>(input.size())) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed");
    }

    int out_len2 = 0;
    if (EVP_DecodeFinal(ctx, out_buf.data() + out_len1, &out_len2) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed");
    }

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), out_len1 + out_len2);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.b64.txt";
    out_name += ".dec.bin";

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
