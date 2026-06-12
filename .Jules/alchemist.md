## 2024-05-24 - Streaming EVP API for Base64 Decode
**Learning:** `EVP_DecodeBlock` fails entirely on Base64 inputs with newlines or padding spaces, which are common in real-world inputs. The streaming API (`EVP_DecodeInit`, `EVP_DecodeUpdate`, `EVP_DecodeFinal`) processes input chunk-by-chunk and safely ignores whitespace, making it the robust choice for decoding Base64.
**Action:** When implementing Base64 decoding, use the streaming EVP functions (`EVP_DecodeInit/Update/Final`) rather than `EVP_DecodeBlock` to ensure resilience against typical formatting elements in input strings.
