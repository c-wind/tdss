#include "global.h"


#define MX ((z>>5^y<<2) + (y>>3^z<<4)) ^ ((sum^y) + (k[(p&3)^e]^z))

void xxtea_encode(uint32_t *v, int n, uint32_t *k)
{
    uint32_t z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
    uint16_t p, q;

    q = 6 + 52/n;
    while (q-- > 0)
    {
        sum += DELTA;
        e = (sum >> 2) & 3;
        for (p=0; p<n-1; p++)
        {
            y = v[p+1];
            z = v[p] += MX;
        }
        y = v[0];
        z = v[n-1] += MX;
    }
}


void xxtea_decode(uint32_t *v, int n, uint32_t *k)
{
    uint32_t z=v[n-1], y=v[0], sum=0, e, DELTA=0x9e3779b9;
    uint16_t p, q;

    q = 6 + 52/n;
    sum = q*DELTA;

    while (sum != 0)
    {
        e = (sum >> 2) & 3;
        for (p=n-1; p>0; p--)
        {
            z = v[p-1];
            y = v[p] -= MX;
        }
        z = v[n-1];
        y = v[0] -= MX;
        sum -= DELTA;
    }
}


uint32_t *str_to_u32(char *src, int len, int *rlen, int include)
{
    uint32_t *rbuf;
    int i;
    int index = ((len + 3)>> 2);

    if(include)
    {
        rbuf = M_alloc(sizeof(uint32_t) * (index + 1));
        rbuf[index] = len;
        *rlen = index + 1;
    }
    else
    {
        rbuf = M_alloc(sizeof(uint32_t) * (index));
        *rlen = index;
    }
    for (i = 0; i < len; i++)
    {
        rbuf[i >> 2] |= ((uint8_t)src[i] << ((i & 3) << 3));
    }

    return rbuf;
}


char *u32_to_str(uint32_t *dat, int *rlen, int include)
{
    char *rbuf;
    int i;
    int index = *rlen << 2;

    if(include)
    {
        index = dat[*rlen -1];
        rbuf = M_alloc(sizeof(char) * (index + 1));
        *rlen = index;
    }
    else
    {
        rbuf = M_alloc(sizeof(char)* (index + 1));
        *rlen = index;
    }
    for (i = 0; i < index; i++)
    {
        rbuf[i] = (uint8_t)(dat[i >> 2] >> ((i & 3) << 3));
    }

    return rbuf;
}

char *string_xxtea_encode(char *src, char *key)
{
    int uk_len, us_len;
    char *nbuf;
    uint32_t *u32_key = str_to_u32(key, strlen(key), &uk_len, 0);

    uint32_t *u32_src = str_to_u32(src, strlen(src), &us_len, 1);

    xxtea_encode(u32_src, us_len, u32_key);

    char *nstr = u32_to_str(u32_src, &us_len, 0);

    nbuf = M_alloc((us_len + 2)/3 *4+1);

    base64_encode(nstr, us_len, nbuf);

    M_free(u32_key);
    M_free(u32_src);
    M_free(nstr);

    return nbuf;
}


char *string_xxtea_decode(char *src, char *key)
{
    int uk_len, us_len;
    char *nbuf;
    int s_len = strlen(src), n_len;
    uint32_t *u32_key = str_to_u32(key, strlen(key), &uk_len, 0);

    n_len = (s_len + 3)/4 *3;
    nbuf = M_alloc(n_len);

    base64_decode(src, s_len, nbuf);

    uint32_t *u32_src = str_to_u32(nbuf, n_len, &us_len, 0);

    xxtea_decode(u32_src, us_len, u32_key);

    M_free(nbuf);

    nbuf = u32_to_str(u32_src, &us_len, 1);

    M_free(u32_src);
    M_free(u32_key);

    return nbuf;
}


#ifdef XXTEA_DEBUG

int main(int argc, int8_t *argv[])
{
    char data[] = {"success@@1,2011-6-15 12:47:10,geilifa,,,此消息由核能QQ群群发发送！不用开启QQ客户端就可以群发消息的软件"};
    int dlen = strlen(data);
    int nlen;
    int klen;
    char key[17] = {"a!b@r#d$t%j*"};

    uint32_t *ukey = str_to_u32(key, strlen(key), &klen, 0);
    uint32_t *u32 = str_to_u32(data, dlen, &nlen, 1);

    char nbuf[10240] = {0};
    char obuf[10240] = {0};
    char cbuf[10240] = {0};
    char dbuf[10240] = {"XgySOtjqMyOyYnetWV72Dch9kEYJbLhKcWfO0TOUeGxyN0yGJxaQtnwHhxO61LdmOCSV14z0Ey84BLaP8PvLP14+ssRqcLHFQfbo1hwlcpWlBNT/FgTUv9r30Whb4Vy8kytazIhfshHiu/MfftgvzSKnZwfUS/i6DAqk2sUDEMAlZoPCKubAKw=="};

    printf("ukey:%s\n", ukey);
    xxtea_encode(u32, nlen, ukey);

    for(klen=0; klen<nlen; klen++)
    {
        printf("%lu\n", u32[klen]);
    }

    char *nstr = u32_to_str(u32, &nlen, 0);

    printf("\nxxtea_encode nstr:%s\n", nstr);

    base64_encode(nstr, nlen, nbuf);

    printf("\nbase64_encode nbuf:%s, len:%d\n", nbuf, strlen(nbuf));

    base64_decode(dbuf, strlen(dbuf), cbuf);

    printf("\nbase64_decode cbuf:%s\n", cbuf);

    u32  = str_to_u32(cbuf, strlen(cbuf), &dlen, 0);

    for(klen=0; klen<dlen; klen++)
    {
        printf("%lu\n", u32[klen]);
    }

    printf("ukey:%s\n", ukey);
    xxtea_decode(u32, dlen, ukey);

    printf("\nsrc:%s\n", u32);

    return 0;
}


#endif

