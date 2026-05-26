#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <vector>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    if (input.empty()) {
        std::string out_name = input_name.empty() ? "input.bin.decoded" : conv::replace_extension(input_name, "decoded");
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Allocate slightly more than the input size (maximum possible decoded length)
    std::vector<unsigned char> out_buf(input.size());

    EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    int outl = 0;
    int res1 = EVP_DecodeUpdate(ctx, out_buf.data(), &outl, input.data(), input.size());

    int outl2 = 0;
    int res2 = EVP_DecodeFinal(ctx, out_buf.data() + outl, &outl2);

    EVP_ENCODE_CTX_free(ctx);

    if (res1 < 0 || res2 < 0) {
        throw std::runtime_error("EVP_Decode failed: invalid base64 input");
    }

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), outl + outl2);

    std::string out_name = input_name;
    if (out_name.empty()) out_name = "input.txt";
    // Usually decoding base64 produces binary or original file, let's name it .decoded
    out_name = conv::replace_extension(out_name, "decoded");

    OutputArtifact artifact;
    artifact.name = out_name;
    // We guess content type based on the name if possible, or just fallback to application/octet-stream
    artifact.content_type = "application/octet-stream";
    artifact.data = decoded_str;

    return { artifact };
}
