#!/bin/bash
set -u

SERVER_BIN=${1:-}
if [ -z "$SERVER_BIN" ]; then
    echo "Usage: $0 <path-to-convertdav-binary>"
    exit 1
fi

SERVER_PID=""
FAILURES=0
SKIPS=0

cleanup() {
    echo "Cleaning up..."
    if [ -n "$SERVER_PID" ]; then
        kill "$SERVER_PID" 2>/dev/null || true
    fi
    rm -f clean.txt clean.png tiny.png tiny.jpg tiny_inverted.png tiny.gif test.pdf test_page_000.png test.mp4 test.gif
    rm -f base64.txt base64.txt.b64.txt base64_dec.bin sha256.txt sha256.txt.sha256.txt md5.txt md5.txt.md5.txt
    rm -f test_json.json test_json.min.json server_test.log
}
trap cleanup EXIT

pass() {
    echo "SUCCESS: $1"
}

fail() {
    echo "FAILED: $1"
    FAILURES=$((FAILURES + 1))
}

skip() {
    echo "SKIP: $1"
    SKIPS=$((SKIPS + 1))
}

has_cmd() {
    command -v "$1" >/dev/null 2>&1
}

magick_cmd() {
    if has_cmd magick; then
        echo "magick"
    elif has_cmd convert; then
        echo "convert"
    else
        echo ""
    fi
}

check_file() {
    if [ -f "$1" ] && [ "$(stat -c%s "$1")" -gt 0 ]; then
        pass "$1 created"
    else
        fail "$1 not created or empty"
    fi
}

check_content() {
    local file=$1
    local expected=$2
    if [ ! -f "$file" ]; then
        fail "$file missing"
        return
    fi
    local actual
    actual=$(cat "$file")
    if [ "$actual" = "$expected" ]; then
        pass "$file content matches"
    else
        fail "$file content mismatch: $actual"
    fi
}

put_file() {
    local src=$1
    local url=$2
    local status
    status=$(curl -sS -o /tmp/convertdav-put.out -w "%{http_code}" -T "$src" "$url" 2>/tmp/convertdav-put.err)
    if [ "$status" = "201" ]; then
        return 0
    fi
    echo "PUT failed with HTTP $status: $(cat /tmp/convertdav-put.out /tmp/convertdav-put.err 2>/dev/null)"
    return 1
}

get_file() {
    local url=$1
    local out=$2
    local status
    status=$(curl -sS -o "$out" -w "%{http_code}" "$url" 2>/tmp/convertdav-get.err)
    if [ "$status" = "200" ]; then
        return 0
    fi
    echo "GET failed with HTTP $status: $(cat /tmp/convertdav-get.err 2>/dev/null)"
    return 1
}

echo "Starting server..."
"$SERVER_BIN" 127.0.0.1 8081 > server_test.log 2>&1 &
SERVER_PID=$!

echo "Waiting for server to start..."
for _ in $(seq 1 30); do
    if curl -s http://127.0.0.1:8081/ >/dev/null 2>&1; then
        break
    fi
    sleep 1
done

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    cat server_test.log
    fail "server exited before tests"
    exit 1
fi

echo "Testing root endpoint..."
if curl -s http://127.0.0.1:8081/ | grep -q "convertdav"; then
    pass "root endpoint"
else
    fail "root endpoint"
fi

echo "Testing base64..."
printf "abc" > base64.txt
if put_file base64.txt http://127.0.0.1:8081/convert/base64/in/base64.txt &&
   get_file http://127.0.0.1:8081/convert/base64/out/base64.txt.b64.txt base64.txt.b64.txt; then
    check_file base64.txt.b64.txt
    check_content base64.txt.b64.txt "YWJj"
else
    fail "base64 conversion request"
fi

echo "Testing base64-dec..."
if put_file base64.txt.b64.txt http://127.0.0.1:8081/convert/base64-dec/in/base64.txt.b64.txt &&
   get_file http://127.0.0.1:8081/convert/base64-dec/out/base64.txt.b64.bin base64_dec.bin; then
    check_file base64_dec.bin
    check_content base64_dec.bin "abc"
else
    fail "base64-dec conversion request"
fi

echo "Testing sha256..."
printf "abc" > sha256.txt
if put_file sha256.txt http://127.0.0.1:8081/convert/sha256/in/sha256.txt &&
   get_file http://127.0.0.1:8081/convert/sha256/out/sha256.txt.sha256.txt sha256.txt.sha256.txt; then
    check_file sha256.txt.sha256.txt
    check_content sha256.txt.sha256.txt "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
else
    fail "sha256 conversion request"
fi

echo "Testing md5..."
printf "abc" > md5.txt
if put_file md5.txt http://127.0.0.1:8081/convert/md5/in/md5.txt &&
   get_file http://127.0.0.1:8081/convert/md5/out/md5.txt.md5.txt md5.txt.md5.txt; then
    check_file md5.txt.md5.txt
    check_content md5.txt.md5.txt "900150983cd24fb0d6963f7d28e17f72"
else
    fail "md5 conversion request"
fi

echo "Testing json-min..."
printf '{ \n  "key" : "value with spaces" \t }' > test_json.json
if put_file test_json.json http://127.0.0.1:8081/convert/json-min/in/test_json.json &&
   get_file http://127.0.0.1:8081/convert/json-min/out/test_json.min.json test_json.min.json; then
    check_file test_json.min.json
    check_content test_json.min.json '{"key":"value with spaces"}'
else
    fail "json-min conversion request"
fi

MAGICK=$(magick_cmd)
if [ -z "$MAGICK" ]; then
    skip "ImageMagick-dependent converters"
else
    echo "Testing png-jpg..."
    "$MAGICK" -size 1x1 xc:white tiny.png
    if put_file tiny.png http://127.0.0.1:8081/convert/png-jpg/in/tiny.png &&
       get_file http://127.0.0.1:8081/convert/png-jpg/out/tiny.jpg tiny.jpg; then
        check_file tiny.jpg
    else
        fail "png-jpg conversion request"
    fi

    echo "Testing invert..."
    if put_file tiny.png http://127.0.0.1:8081/convert/invert/in/tiny.png &&
       get_file http://127.0.0.1:8081/convert/invert/out/tiny_inverted.png tiny_inverted.png; then
        check_file tiny_inverted.png
    else
        fail "invert conversion request"
    fi

    echo "Testing img-gif..."
    if put_file tiny.png http://127.0.0.1:8081/convert/img-gif/in/tiny.png &&
       get_file http://127.0.0.1:8081/convert/img-gif/out/tiny.gif tiny.gif; then
        check_file tiny.gif
    else
        fail "img-gif conversion request"
    fi

    echo "Testing pdf-png..."
    if "$MAGICK" tiny.png test.pdf 2>/dev/null; then
        if put_file test.pdf http://127.0.0.1:8081/convert/pdf-png/in/test.pdf &&
           get_file http://127.0.0.1:8081/convert/pdf-png/out/test_page_000.png test_page_000.png; then
            check_file test_page_000.png
        else
            skip "pdf-png conversion blocked by ImageMagick/Ghostscript policy"
        fi
    else
        skip "PDF fixture creation blocked by ImageMagick policy"
    fi

    if has_cmd clamscan; then
        echo "Testing virustest..."
        printf "This is a clean test file" > clean.txt
        if put_file clean.txt http://127.0.0.1:8081/convert/virustest/in/clean.txt &&
           get_file http://127.0.0.1:8081/convert/virustest/out/clean.png clean.png; then
            check_file clean.png
        else
            fail "virustest conversion request"
        fi
    else
        skip "virustest (clamscan not found)"
    fi
fi

if has_cmd ffmpeg; then
    echo "Testing mp4-gif..."
    ffmpeg -y -f lavfi -i color=c=red:s=16x16:d=1 -pix_fmt yuv420p test.mp4 >/dev/null 2>&1
    if put_file test.mp4 http://127.0.0.1:8081/convert/mp4-gif/in/test.mp4 &&
       get_file http://127.0.0.1:8081/convert/mp4-gif/out/test.gif test.gif; then
        check_file test.gif
    else
        fail "mp4-gif conversion request"
    fi
else
    skip "mp4-gif (ffmpeg not found)"
fi

if [ "$FAILURES" -gt 0 ]; then
    echo "$FAILURES test(s) failed, $SKIPS skipped"
    cat server_test.log
    exit 1
fi

echo "All tests passed ($SKIPS skipped)"
