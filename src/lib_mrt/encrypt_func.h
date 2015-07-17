#ifndef __ENCRYPT_FUNC_H__
#define __ENCRYPT_FUNC_H__

/* max. buffer size required for base64_encode() */
#define MAX_BASE64_ENCODED_SIZE(size) \
	((size) / 3 * 4 + 2+2)
/* max. buffer size required for base64_decode() */
#define MAX_BASE64_DECODED_SIZE(size) \
	((size) / 4 * 3 + 3)

void base64_encode(const void *src, size_t src_size, char *dest);

int base64_decode(const void *src, size_t src_size, char *dest);

uint32_t crc32_data(const void *data, size_t size) ;
uint32_t crc32_str(const char *str);

uint32_t crc32_data_more(uint32_t crc, const void *data, size_t size);
uint32_t crc32_str_more(uint32_t crc, const char *str);

void uri_decode(const char * psrc, const int len, char * pres, int * plen);

void uri_encode(const char * psrc, const int len, char * pres, int * plen);

char *string_xxtea_encode(char *src, char *key);

char *string_xxtea_decode(char *src, char *key);

#endif
