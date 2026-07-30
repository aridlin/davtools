#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.txt";

    // Replace .txt with .bin or add .decoded.bin
    if (out_name.ends_with(".b64.txt")) {
        out_name = out_name.substr(0, out_name.size() - 8);
    } else if (out_name.ends_with(".txt")) {
        out_name = out_name.substr(0, out_name.size() - 4);
    }
    out_name += ".decoded.bin";

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // A valid Base64 encoded string produces max 3 bytes for every 4 bytes.
    // The decoded length is at most input.size().
    std::vector<unsigned char> out_buf(input.size());

    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    int outl = 0;
    int total_len = 0;

    // Process input
    if (EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeUpdate failed: invalid base64 input");
    }
    total_len += outl;

    // Finalize
    int outl2 = 0;
    if (EVP_DecodeFinal(ctx, out_buf.data() + total_len, &outl2) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("EVP_DecodeFinal failed: invalid base64 input");
    }
    total_len += outl2;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
