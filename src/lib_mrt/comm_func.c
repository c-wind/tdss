#include "global.h"
#include <openssl/md5.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdarg.h>


char error_msg[MAX_LINE];

void set_error(char *msg,...)
{
    va_list ap;
    va_start(ap, msg);
    vsnprintf(error_msg, sizeof(error_msg), msg, ap);
    va_end(ap);
}

char *get_error()
{
    return error_msg;
}


static void *tried_signal(const int signo, void *pfunc)
{
    struct sigaction act, oact;

    memset( &act, 0, sizeof(struct sigaction) );
    memset( &oact, 0, sizeof(struct sigaction) );
    act.sa_handler = pfunc;
    if ( sigemptyset( &act.sa_mask ) == MRT_ERR )
        return SIG_ERR;
    act.sa_flags = 0;
    if(signo == SIGALRM)
    {
#ifdef  SA_INTERRUPT
        act.sa_flags |= SA_INTERRUPT;  //SunOS 4.x
#endif
    }
    else
    {
#ifdef  SA_RESTART
        act.sa_flags |= SA_RESTART;  //SVR4,44BSD
#endif
    }
    if ( sigaction(signo, &act, &oact) < 0 )
        return( SIG_ERR );

    return( oact.sa_handler );
}



int daemon_init(char *home)
{
    int i;
    pid_t pid;

    if ((pid=fork()) < 0)
        return MRT_ERR;
    if (pid > 0)
        exit(0);

    setsid();

    if ((pid=fork()) < 0)
        return MRT_ERR;
    if (pid > 0)
        exit(0);

    if (tried_signal(SIGHUP, SIG_IGN) == SIG_ERR)
        return MRT_ERR;
    if (tried_signal(SIGINT, SIG_IGN) == SIG_ERR)
        return MRT_ERR;
    if (tried_signal(SIGWINCH, SIG_IGN) == SIG_ERR)
        return MRT_ERR;
    if (tried_signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return MRT_ERR;

    chdir(home);
    umask(0);

    for (i=0; i< 3; i++ )
        close(i);

    return MRT_OK;
}


void quick_sort(int ary[], int left, int right)
{
    int i = left, j = right, temp = ary[left];

    if(left > right)
        return;

    while(i != j)
    {
        while(ary[j] >= temp && j > i)
            j--;

        if(j > i)
            ary[i++] = ary[j];

        while(ary[i] <= temp && j > i)
            i++;

        if(j > i)
            ary[j--] = ary[i];
    }

    ary[i] = temp;

    quick_sort(ary, left, i-1);
    quick_sort(ary, i+1, right);

}




/*
int trust_addr_array_init(T_array *ary, char *addrs, char *separator)
{
    int sep_num = 0, i=0;
    char *pstr = addrs;
    char caddr[MAX_ADDR] = {0};

    M_cpvril(ary);
    M_cpsril(addrs);
    M_cpsril(separator);

    sep_num = str_part_num(addrs, separator) + 1;

    M_cvril((ary->data = M_alloc(sizeof(int64_t) * sep_num)), "M_alloc error.");

    while((i<sep_num) && (str_separate(&pstr, caddr, separator) == MRT_SUC))
    {
        ((int64_t *)ary->data)[i++] = inet_addr(caddr);

        p_zero(caddr);
    }

    ary->len = i;
    ary->size = sep_num;

    quick_sort(ary->data, 0, ary->len - 1);

    */
    /*
       for(i=0; i< ary->size; i++)
       {
       printf("%u\n", ((int64_t *)ary->data)[i]);
       }

       printf("size:%d,len:%d\n", ary->size, ary->len);
       */

/*
    return MRT_SUC;
}

static int addr_compare(const void *addr1, const void *addr2)
{
*/
    /*
       printf("%u, %u\n", *(int64_t *)addr1 , *(int64_t *)addr2);
       */
    /*
    return *(int64_t *)addr1 - *(int64_t *)addr2;
}
       */


/*
int trust_addr_check(T_array *ary, char *addr)
{
    int64_t *var = NULL;
    int64_t key = inet_addr(addr);


    var = bsearch(&key, ary->data, ary->len, sizeof(int64_t), addr_compare);

    if(var && *var == key)
    {
        return MRT_SUC;
    }

    return MRT_ERR;
}
*/

void Abort(char * msg)
{
    printf("%s", msg);
    exit(1);
}

/* linux does not know anymore about itoa and windows
 * does not support 64bit ints. so take that! :)
 * keep in mind that con_unit sould be <= 10, otherwise
 * you get strange output... (me was too lazy to fix it) */
char *int64toa(off_t num, char * buf, int con_unit)
{
    off_t tmp=num;
    if(num == 0)
    {
        buf[0] = '0', buf[1] = 0;
        return buf;
    }
    while(tmp > 0)
    {
        buf++;
        tmp /= con_unit;
    }
    *buf = 0;
    while(num > 0)
    {
        *--buf = '0' + (char) (num % con_unit);
        num /= con_unit;
    }
    return buf;
}

char *get_port_fmt(int ip, unsigned int port)
{
    unsigned char b[6];
    static char buf[6 * 4];
    /* TODO USS have we got an endian problem here for the ip-address */
    *         (int *) b    = ip;
    *(unsigned int *)(b+4) = htons(port);
    sprintf(buf, "%d,%d,%d,%d,%d,%d",
            b[0], b[1], b[2], b[3], b[4], b[5]);
    return buf;
}

/* adopted from wget */
int file_exists(const char * filename)
{
    return access(filename, F_OK) >= 0;
}

char *get_home_dir(void)
{
    char *home = getenv ("HOME");

    if (!home)
    {
        /* If HOME is not defined, try getting it from the password file.
         * adopted from wget */
        struct passwd *pwd = getpwuid(getuid ());
        if (!pwd || !pwd->pw_dir)
            return NULL;
        home = pwd->pw_dir;
    }

    return home ? str_newcpy(home, strlen(home)) : NULL;
}

char *read_line(FILE *fp)
{
    int length = 0;
    int bufsize = 82;
    char *line = (char *) M_alloc (bufsize);

    while (fgets(line + length, bufsize - length, fp))
    {
        length += strlen (line + length);

        if (line[length - 1] == '\n')
            break;

        /* fgets() guarantees to read the whole line, or to use up the
           space we've given it.  We can double the buffer
           unconditionally.  */
        bufsize <<= 1;
        line = realloc (line, bufsize);
    }
    if (length == 0 || ferror (fp))
    {
        free (line);
        return NULL;
    }
    if (length + 1 < bufsize)
        /* Relieve the memory from our exponential greediness.  We say
           `length + 1' because the terminating \0 is not included in
           LENGTH.  We don't need to zero-terminate the string ourselves,
           though, because fgets() does that.  */
        line = realloc(line, length + 1);
    return line;
}


char *printip(unsigned char * ip)
{
    static char rv[16];
    sprintf(rv, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return rv;
}

int hextoi(char h)
{
    if(h >= 'A' && h <= 'F')
        return h - 'A' + 10;
    if(h >= 'a' && h <= 'f')
        return h - 'a' + 10;
    if(h >= '0' && h <= '9')
        return h - '0';

    set_error("parse-error in escaped character: %c is not a hexadecimal character", h);

    return MRT_ERR;
}
/* transform things like user%40host.com to user@host.com */
/* overwrites str and returns str. */
char * unescape(char * str)
{
    char * ptr = str;
    char * org = str;
    while(*ptr) {
        if(*ptr == '%') {
            ptr += 2;
            *ptr = (hextoi(*(ptr-1)) << 4) + hextoi(*(ptr));
        }
        *(str++) = *(ptr++);
    }
    *str = *ptr;
    return org;
}


/* Engine for legible;         from wget
 * add thousand separators to numbers printed in strings.
 */
static char * legible_1 (const char *repr)
{
    static char outbuf[48]; /* TODO NRV adjust this value to a serious one. who cares? */
    int i, i1, mod;
    char *outptr;
    const char *inptr;

    /* Reset the pointers.  */
    outptr = outbuf;
    inptr = repr;

    /* Ignore the sign for the purpose of adding thousand
       separators.  */
    if (*inptr == '-')
    {
        *outptr++ = '-';
        ++inptr;
    }
    /* How many digits before the first separator?  */
    mod = strlen (inptr) % 3;
    /* Insert them.  */
    for (i = 0; i < mod; i++)
        *outptr++ = inptr[i];
    /* Now insert the rest of them, putting separator before every
       third digit.  */
    for (i1 = i, i = 0; inptr[i1]; i++, i1++)
    {
        if (i % 3 == 0 && i1 != 0)
            *outptr++ = ',';
        *outptr++ = inptr[i1];
    }
    /* Zero-terminate the string.  */
    *outptr = '\0';
    return outbuf;
}

/* Legible -- return a static pointer to the legibly printed long.  */
char *legible(off_t l)
{
    char inbuf[24];
    /* Print the number into the buffer.  */
    int64toa(l, inbuf, 10);
    return legible_1 (inbuf);
}

/* Count the digits in a (signed long) integer.  */
int numdigit (long number)
{
    int cnt = 1;
    if (number < 0)
    {
        number = -number;
        ++cnt;
    }
    while ((number /= 10) > 0)
        ++cnt;
    return cnt;
}


/* gets the filename */
char *basename(char * p)
{
    char * t = p + strlen(p);

    while(*t != '/' && t != p)
        t--;
    return (t == p) ? t : t+1;
}


/* took me ages to write this cute function (it's probably the one i like most in wput):
 * transforms things like "../..gaga/welt/.././.fool/../../new" to "../new"
 * (usually i don't expect such input, but who knows. works quite well now) */
void clear_path(char * path)
{
    char * src = path;
    char * dst = path;
    char * org = path;

    while(*src)
    {
        if(src[0] == '/' &&
           src[1] == '.' &&
           ((src[2] == '.' && (src[3] == '/' || src[3] == 0 ))|| src[2] == '/' || src[2] == 0) &&
           dst != org)
        {
            if(src[2] == '.' &&
               (src[3] == '/' || src[3] == 0))
            {
                if((dst-2 == org && !strncmp(dst-2, "..", 2)) ||
                   !strncmp(dst-3, "/..", 3))
                    *dst++ = *src++;
                else
                {
                    while(dst >= org && *--dst != '/')
                        ;
                    if(dst < org) dst++;
                    src+=3;
                }
            } else if(src[2] == '/' || src[2] == 0)
                src += 2;
        } else
        {
            if(*src == '/' && dst == org)
                src++;
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

/* makes from some/path and some/other/path
 * ../other/path
 * requires src and dst to begin and end with a slash */
char *get_relative_path(char * src, char * dst)
{
    char * tmp = dst;
    char * mark_src = src;
    char * mark_dst = dst;
    int counter = 1;
    /* find the point where they differ and put the mark after the last
     * common slash */
    while( *src != 0 && *dst != 0 && *src == *dst)
    {
        if(*src == '/') {
            mark_src = src+1;
            mark_dst = dst+1;
        }
        src++; dst++;
    }
    /* special case where dst is a complete subpart of src */
    if(*src == '/' && *dst == 0)
    {
        mark_src = src+1;
        mark_dst = dst;
    }
    /* if all of src matches dst, we return the rest of dst */
    if(*src == 0 && *dst == '/')
        return str_newcpy(dst+1, strlen(dst+1));

    /* now count the remaining slashes and add a ../ to the rel path for each of them */
    tmp = mark_src;
    while(*tmp++ != 0)
        if(*tmp == '/')
            counter++;
    tmp = M_alloc(counter * 3 + strlen(mark_dst) + 1);
    *tmp = 0;
    while(counter-- > 1)
        strcat(tmp, "../");
    strcat(tmp, mark_dst);
    /* cut the trailing slash off */
    if(tmp[strlen(tmp)-1] == '/') tmp[strlen(tmp)-1] = 0;
    return tmp;
}





#ifdef COMMON_TEST

int main()
{
    char addrs[] = {"127.0.0.5,127.0.0.4,127.0.0.3,127.0.0.2,127.0.0.1,127.0.0.10,127.0.0.6,127.0.0.7,127.0.0.8,127.0.0.9"};

    memory_pool_init();

    printf("%s \n", addrs);

    T_array ary;

    if(trust_addr_init(&ary, addrs, ",") == MRT_ERR)
    {
        printf("%s trust_addr_init error.\n", __func__);
        return MRT_ERR;
    }

    printf("%s check %s, return:%d\n", __func__, "127.0.0.4", trust_addr_check(&ary, "127.0.0.4"));


    return 0;

}


#endif


