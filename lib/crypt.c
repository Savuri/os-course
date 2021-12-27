#include <inc/lib.h>

#include <inc/hmac.h>


#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define SALT_MAX_LEN  128

void
bzero(void *buf, size_t len) {
    memset(buf, 0, len);
}


/*
 * Password-Based Key Derivation Function 2 (PKCS #5 v2.0).
 * Code based on IEEE Std 802.11-2007, Annex H.4.2.
 */
int
pkcs5_pbkdf2(const uint8_t *pass, size_t pass_len, const uint8_t *salt,
             size_t salt_len, uint8_t *key, size_t key_len, unsigned int rounds) {
    uint8_t asalt[SALT_MAX_LEN], obuf[HMAC_SHA1_HASH_SIZE];
    uint8_t d1[HMAC_SHA1_HASH_SIZE], d2[HMAC_SHA1_HASH_SIZE];
    unsigned int i, j;
    unsigned int count;
    size_t r;

    if (rounds < 1 || key_len == 0)
        return -1;
    if (salt_len == 0 || salt_len > SALT_MAX_LEN - 1)
        return -1;

    memcpy(asalt, salt, salt_len);

    for (count = 1; key_len > 0; count++) {
        asalt[salt_len + 0] = (count >> 24) & 0xff;
        asalt[salt_len + 1] = (count >> 16) & 0xff;
        asalt[salt_len + 2] = (count >> 8) & 0xff;
        asalt[salt_len + 3] = count & 0xff;
        hmac_sha1(asalt, salt_len + 4, pass, pass_len, d1);
        memcpy(obuf, d1, sizeof(obuf));

        for (i = 1; i < rounds; i++) {
            hmac_sha1(d1, sizeof(d1), pass, pass_len, d2);
            memcpy(d1, d2, sizeof(d1));
            for (j = 0; j < sizeof(obuf); j++)
                obuf[j] ^= d1[j];
        }

        r = MIN(key_len, HMAC_SHA1_HASH_SIZE);
        memcpy(key, obuf, r);
        key += r;
        key_len -= r;
    };
    bzero(asalt, salt_len + 4);
    bzero(d1, sizeof(d1));
    bzero(d2, sizeof(d2));
    bzero(obuf, sizeof(obuf));

    return 0;
}
