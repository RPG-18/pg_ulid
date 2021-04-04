#include <chrono>
#include <string>
#include <vector>

extern "C" {
#include <fmgr.h>
#include <postgres.h>
#include <utils/uuid.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(ulid_generate);
Datum ulid_generate(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(ulid_to_string);
Datum ulid_to_string(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(ulid_from_string);
Datum ulid_from_string(PG_FUNCTION_ARGS);
}

namespace {
const std::string ENCODING("0123456789ABCDEFGHJKMNPQRSTVWXYZ");

constexpr uint ENTROPY_OFFSET = 6;

constexpr uint ENCODED_ULID_LEN = 26; // 10 byte timestamp + 16 bytes of entropy
constexpr uint ENCODED_TEXT_LEN = ENCODED_ULID_LEN + VARHDRSZ;

const std::vector<uint8_t> DECODING = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xFF, 0x12, 0x13, 0xFF, 0x14, 0x15, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xFF, 0x12, 0x13, 0xFF, 0x14, 0x15, 0xFF,
    0x16, 0x17, 0x18, 0x19, 0x1A, 0xFF, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};
} // namespace

Datum ulid_generate(PG_FUNCTION_ARGS)
{
    using uchar = unsigned char;
    auto ulid = static_cast<pg_uuid_t *>(palloc(sizeof(pg_uuid_t)));

    pg_strong_random(ulid->data + ENTROPY_OFFSET, UUID_LEN - ENTROPY_OFFSET);

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    const uint64 tm = ms.count();

    ulid->data[0] = static_cast<uchar>(tm >> 40);
    ulid->data[1] = static_cast<uchar>(tm >> 32);
    ulid->data[2] = static_cast<uchar>(tm >> 24);
    ulid->data[3] = static_cast<uchar>(tm >> 16);
    ulid->data[4] = static_cast<uchar>(tm >> 8);
    ulid->data[5] = static_cast<uchar>(tm);

    PG_RETURN_UUID_P(ulid);
}

Datum ulid_to_string(PG_FUNCTION_ARGS)
{
    // https://github.com/oklog/ulid/blob/master/ulid.go#L293
    const auto ulid = PG_GETARG_UUID_P(0);
    auto dest = static_cast<text *>(palloc0(ENCODED_TEXT_LEN));
    SET_VARSIZE(dest, ENCODED_TEXT_LEN);

    const auto id = ulid->data;

    // 10 byte timestamp
    dest->vl_dat[0] = ENCODING[(id[0] & 224) >> 5];
    dest->vl_dat[1] = ENCODING[id[0] & 31];
    dest->vl_dat[2] = ENCODING[(id[1] & 248) >> 3];
    dest->vl_dat[3] = ENCODING[((id[1] & 7) << 2) | ((id[2] & 192) >> 6)];
    dest->vl_dat[4] = ENCODING[(id[2] & 62) >> 1];
    dest->vl_dat[5] = ENCODING[((id[2] & 1) << 4) | ((id[3] & 240) >> 4)];
    dest->vl_dat[6] = ENCODING[((id[3] & 15) << 1) | ((id[4] & 128) >> 7)];
    dest->vl_dat[7] = ENCODING[(id[4] & 124) >> 2];
    dest->vl_dat[8] = ENCODING[((id[4] & 3) << 3) | ((id[5] & 224) >> 5)];
    dest->vl_dat[9] = ENCODING[id[5] & 31];

    // 16 bytes of entropy
    dest->vl_dat[10] = ENCODING[(id[6] & 248) >> 3];
    dest->vl_dat[11] = ENCODING[((id[6] & 7) << 2) | ((id[7] & 192) >> 6)];
    dest->vl_dat[12] = ENCODING[(id[7] & 62) >> 1];
    dest->vl_dat[13] = ENCODING[((id[7] & 1) << 4) | ((id[8] & 240) >> 4)];
    dest->vl_dat[14] = ENCODING[((id[8] & 15) << 1) | ((id[9] & 128) >> 7)];
    dest->vl_dat[15] = ENCODING[(id[9] & 124) >> 2];
    dest->vl_dat[16] = ENCODING[((id[9] & 3) << 3) | ((id[10] & 224) >> 5)];
    dest->vl_dat[17] = ENCODING[id[10] & 31];
    dest->vl_dat[18] = ENCODING[(id[11] & 248) >> 3];
    dest->vl_dat[19] = ENCODING[((id[11] & 7) << 2) | ((id[12] & 192) >> 6)];
    dest->vl_dat[20] = ENCODING[(id[12] & 62) >> 1];
    dest->vl_dat[21] = ENCODING[((id[12] & 1) << 4) | ((id[13] & 240) >> 4)];
    dest->vl_dat[22] = ENCODING[((id[13] & 15) << 1) | ((id[14] & 128) >> 7)];
    dest->vl_dat[23] = ENCODING[(id[14] & 124) >> 2];
    dest->vl_dat[24] = ENCODING[((id[14] & 3) << 3) | ((id[15] & 224) >> 5)];
    dest->vl_dat[25] = ENCODING[id[15] & 31];

    PG_RETURN_TEXT_P(dest);
}

// https://github.com/oklog/ulid/blob/master/ulid.go#L144
Datum ulid_from_string(PG_FUNCTION_ARGS)
{
    const auto txt = PG_GETARG_TEXT_P(0);
    const auto len = VARSIZE(txt);

    if (len != ENCODED_TEXT_LEN) {
        elog(ERROR, "invalid length, expected 26 symbols");
    }

    // Check if all the characters in a base32 encoded ULID are part of the
    // expected base32 character set.
    const auto v = reinterpret_cast<uint8_t *>(txt->vl_dat);
    if (DECODING[v[0]] == 0xFF || DECODING[v[1]] == 0xFF || DECODING[v[2]] == 0xFF
        || DECODING[v[3]] == 0xFF || DECODING[v[4]] == 0xFF || DECODING[v[5]] == 0xFF
        || DECODING[v[6]] == 0xFF || DECODING[v[7]] == 0xFF || DECODING[v[8]] == 0xFF
        || DECODING[v[9]] == 0xFF || DECODING[v[10]] == 0xFF || DECODING[v[11]] == 0xFF
        || DECODING[v[12]] == 0xFF || DECODING[v[13]] == 0xFF || DECODING[v[14]] == 0xFF
        || DECODING[v[15]] == 0xFF || DECODING[v[16]] == 0xFF || DECODING[v[17]] == 0xFF
        || DECODING[v[18]] == 0xFF || DECODING[v[19]] == 0xFF || DECODING[v[20]] == 0xFF
        || DECODING[v[21]] == 0xFF || DECODING[v[22]] == 0xFF || DECODING[v[23]] == 0xFF
        || DECODING[v[24]] == 0xFF || DECODING[v[25]] == 0xFF) {
        elog(ERROR, "bad data character");
    }

    // Check if the first character in a base32 encoded ULID will overflow. This
    // happens because the base32 representation encodes 130 bits, while the
    // ULID is only 128 bits.
    //
    // See https://github.com/oklog/ulid/issues/9 for details.
    if (v[0] > '7') {
        elog(ERROR, "overflow");
    }

    auto ulid = static_cast<pg_uuid_t *>(palloc(sizeof(pg_uuid_t)));

    // 6 bytes timestamp (48 bits)
    ulid->data[0] = (DECODING[v[0]] << 5) | DECODING[v[1]];
    ulid->data[1] = (DECODING[v[2]] << 3) | (DECODING[v[3]] >> 2);
    ulid->data[2] = (DECODING[v[3]] << 6) | (DECODING[v[4]] << 1) | (DECODING[v[5]] >> 4);
    ulid->data[3] = (DECODING[v[5]] << 4) | (DECODING[v[6]] >> 1);
    ulid->data[4] = (DECODING[v[6]] << 7) | (DECODING[v[7]] << 2) | (DECODING[v[8]] >> 3);
    ulid->data[5] = (DECODING[v[8]] << 5) | DECODING[v[9]];

    // 10 bytes of entropy (80 bits)
    ulid->data[6] = (DECODING[v[10]] << 3) | (DECODING[v[11]] >> 2);
    ulid->data[7] = (DECODING[v[11]] << 6) | (DECODING[v[12]] << 1) | (DECODING[v[13]] >> 4);
    ulid->data[8] = (DECODING[v[13]] << 4) | (DECODING[v[14]] >> 1);
    ulid->data[9] = (DECODING[v[14]] << 7) | (DECODING[v[15]] << 2) | (DECODING[v[16]] >> 3);
    ulid->data[10] = (DECODING[v[16]] << 5) | DECODING[v[17]];
    ulid->data[11] = (DECODING[v[18]] << 3) | DECODING[v[19]] >> 2;
    ulid->data[12] = (DECODING[v[19]] << 6) | (DECODING[v[20]] << 1) | (DECODING[v[21]] >> 4);
    ulid->data[13] = (DECODING[v[21]] << 4) | (DECODING[v[22]] >> 1);
    ulid->data[14] = (DECODING[v[22]] << 7) | (DECODING[v[23]] << 2) | (DECODING[v[24]] >> 3);
    ulid->data[15] = (DECODING[v[24]] << 5) | DECODING[v[25]];
    PG_RETURN_UUID_P(ulid);
}