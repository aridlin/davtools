#include "common.hpp"

#include <openssl/evp.h>
#include <string>

std::vector<OutputArtifact> convert_base64_dec(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = input_name;
    // Remove .txt if exists
    if (out_name.ends_with(".txt")) {
        out_name = out_name.substr(0, out_name.size() - 4);
    }
    // Remove .b64 if exists
    if (out_name.ends_with(".b64")) {
        out_name = out_name.substr(0, out_name.size() - 4);
    }

    if (out_name == input_name || out_name.empty()) {
        if (out_name.empty()) {
            out_name = "input.bin";
        }
        out_name += ".dec";
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
    int res = EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size());

    int totall = outl;
    int outl2 = 0;
    int res2 = EVP_DecodeFinal(ctx, out_buf.data() + totall, &outl2);

    totall += outl2;
    EVP_ENCODE_CTX_free(ctx);

    if (res < 0 || res2 < 0) {
        throw std::runtime_error("Invalid base64 input");
    }

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), totall);

    OutputArtifact artifact;
    artifact.name = out_name;
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
