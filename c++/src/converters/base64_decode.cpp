#include "common.hpp"

#include <openssl/evp.h>
#include <string>
#include <vector>
#include <stdexcept>

std::vector<OutputArtifact> convert_base64_decode(
    const std::string& input_name,
    const std::vector<std::uint8_t>& input)
{
    std::string out_name = conv::replace_extension(input_name, "decoded.bin");

    // Attempt to guess better extension if original had .b64.txt
    if (input_name.ends_with(".b64.txt")) {
        out_name = input_name.substr(0, input_name.length() - 8);
        if (out_name.empty()) out_name = "decoded.bin";
    } else if (input_name.ends_with(".b64")) {
        out_name = input_name.substr(0, input_name.length() - 4);
        if (out_name.empty()) out_name = "decoded.bin";
    }

    if (input.empty()) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Heuristic pre-check: empty after stripping whitespace?
    bool all_ws = true;
    for (auto c : input) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            all_ws = false;
            break;
        }
    }
    if (all_ws) {
        OutputArtifact artifact;
        artifact.name = out_name;
        artifact.content_type = "application/octet-stream";
        artifact.data = "";
        return { artifact };
    }

    // Decoding via streaming API handles newlines safely
    EVP_ENCODE_CTX* ctx = EVP_ENCODE_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_ENCODE_CTX_new failed");
    }

    EVP_DecodeInit(ctx);

    // Max potential output size: each 4 bytes of input produces up to 3 bytes
    // plus potential block sizes. Size * 3 / 4 is generally safe, but EVP handles padding.
    std::vector<unsigned char> out_buf(input.size() * 3 / 4 + 4);

    int out_len = 0;
    int total_len = 0;

    // EVP_DecodeUpdate returns > 0 on success, 0 on EOF, < 0 on error
    int res = EVP_DecodeUpdate(ctx, out_buf.data(), &out_len, input.data(), input.size());
    if (res < 0) {
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("Invalid Base64 input (EVP_DecodeUpdate failed)");
    }
    total_len += out_len;

    res = EVP_DecodeFinal(ctx, out_buf.data() + total_len, &out_len);
    if (res < 0) {
        // According to OpenSSL docs, -1 from DecodeFinal indicates padding issues.
        // It's technically an error for strict Base64, but we'll try to use what we got,
        // or just throw to fail loudly as per the "fail loudly" rule.
        EVP_ENCODE_CTX_free(ctx);
        throw std::runtime_error("Invalid Base64 input padding (EVP_DecodeFinal failed)");
    }
    total_len += out_len;

    EVP_ENCODE_CTX_free(ctx);

    std::string decoded_str(reinterpret_cast<char*>(out_buf.data()), total_len);

    OutputArtifact artifact;
    artifact.name = out_name;
    // Guess type if possible or just stick to plain/octet-stream
    // Let's rely on server.cpp to guess the content type based on name if we leave it empty.
    artifact.content_type = "";
    artifact.data = std::move(decoded_str);

    return { artifact };
}
