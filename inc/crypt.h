#ifndef INC_CRYPT_H
#define INC_CRYPT_H 1

int
pkcs5_pbkdf2(const uint8_t *pass, size_t pass_len, const uint8_t *salt,
             size_t salt_len, uint8_t *key, size_t key_len, unsigned int rounds);

#endif /* INC_CRYPT_H  */
