#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.b64.bin" : input_name + ".bin";
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    std::vector<unsigned char> out_buf(input.size() * 3 / 4 + 1);
    int out_len = 0;
    int final_len = 0;

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    if (EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size()) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("Invalid Base64 input");
    }

    if (EVP_DecodeFinal(ctx, out_buf.data() + out_len, &final_len) < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("Invalid Base64 input formatting");
    }

    out_len += final_len;
    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), out_len);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.b64";

    // strip .b64 or .txt if possible, then append .bin
    if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".b64") {
        out_name = out_name.substr(0, out_name.size() - 4);
    } else if (out_name.size() > 4 && out_name.substr(out_name.size() - 4) == ".txt") {
        out_name = out_name.substr(0, out_name.size() - 4);
    }
    out_name += ".bin";

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
