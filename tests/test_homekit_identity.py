#!/usr/bin/env python3
"""Focused test vectors for the documented homeBert v1 HomeKit derivation."""

import hashlib


CODE_DOMAIN = b"homeBert/setup-code/v1"
ID_DOMAIN = b"homeBert/setup-id/v1"
ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"


def digest_value(domain, mac, retry):
    digest = hashlib.sha256(domain + mac + bytes([retry])).digest()
    return int.from_bytes(digest[:4], "big")


def allowed(code):
    return not (len(set(code)) == 1 or code in ("12345678", "87654321"))


def setup_code(mac, value_fn=digest_value):
    for retry in range(256):
        code = f"{value_fn(CODE_DOMAIN, mac, retry) % 100000000:08d}"
        if allowed(code):
            return code, retry
    raise AssertionError("no permitted setup code")


def setup_id(mac):
    value = digest_value(ID_DOMAIN, mac, 0)
    chars = []
    for _ in range(4):
        chars.append(ALPHABET[value % 36])
        value //= 36
    return "".join(reversed(chars))


def qr_payload(code, identifier, category=31, protocols=2):
    value = (category << 31) | (protocols << 27) | int(code)
    encoded = []
    for _ in range(9):
        digit = value % 36
        encoded.append(chr(digit + 48 if digit < 10 else digit + 55))
        value //= 36
    return "X-HM://" + "".join(reversed(encoded)) + identifier


def migration_required(marker):
    return marker != 1


def main():
    vectors = {
        "000000000000": ("06558548", "P84K"),
        "ffffffffffff": ("49651981", "DKU7"),
        "020000000001": ("81304194", "3679"),
    }
    for hex_mac, expected in vectors.items():
        mac = bytes.fromhex(hex_mac)
        first = (setup_code(mac), setup_id(mac))
        second = (setup_code(mac), setup_id(mac))
        assert first == second
        assert first == ((expected[0], 0), expected[1])
        assert len(first[0][0]) == 8 and first[0][0].isdigit()
        assert len(first[1]) == 4 and first[1].isalnum()

    assert qr_payload("81304194", "3679") == "X-HM://00UQRGZGI3679"
    assert migration_required(0)
    assert not migration_required(1)
    assert len("homekit_v") <= 15

    # A disallowed result is retried with the next domain-separated counter.
    def forced_value(_domain, _mac, retry):
        return 0 if retry == 0 else 42

    assert setup_code(bytes(6), forced_value) == ("00000042", 1)
    assert not allowed("00000000")
    assert not allowed("12345678")
    assert not allowed("87654321")


if __name__ == "__main__":
    main()
