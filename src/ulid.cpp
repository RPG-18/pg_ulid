#include <string>
#include <chrono>

extern "C" {
#include <postgres.h>
#include <fmgr.h>
#include <utils/uuid.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(ulid_generate);
Datum ulid_generate(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(ulid_to_string);
Datum ulid_to_string(PG_FUNCTION_ARGS);
}

namespace
{
const std::string ENCODING("0123456789ABCDEFGHJKMNPQRSTVWXYZ");

constexpr uint ENTROPY_OFFSET = 6;
constexpr uint ENCODED_ULID_LEN = 26; // 10 byte timestamp + 16 bytes of entropy
constexpr uint ENCODED_TEXT_LEN = ENCODED_ULID_LEN + VARHDRSZ;
}

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
    SET_VARSIZE(dest, ENCODED_ULID_LEN);

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