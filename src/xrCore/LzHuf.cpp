// LzHuf.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#pragma hdrstop
#if defined(WINDOWS)
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>

#define MODULE

// typedef unsigned char BYTE;

unsigned textsize = 0, codesize = 0;

char wterr[] = "Can't write.";

/********** LZSS compression **********/

#define N 4096 /* buffer size */
#define F 60 /* lookahead buffer size */
#define THRESHOLD 2
#define NIL N /* leaf of tree */

#define N_CHAR (256 - THRESHOLD + F) /* kinds of characters (character code = 0..N_CHAR-1) */
#define T (N_CHAR * 2 - 1) /* size of table */
#define R (T - 1) /* position of root */
#define MAX_FREQ 0x4000 /* updates tree when the */

u8 text_buf[N + F];
int match_position, match_length, lson[N + 1], rson[N + 257], dad[N + 1];

unsigned freq[T + 1]; /* frequency table */

int prnt[T + N_CHAR + 1]; /* pointers to parent nodes, except for the */
/* elements [T..T + N_CHAR - 1] which are used to get */
/* the positions of leaves corresponding to the codes. */

int son[T]; /* pointers to child nodes (son[], son[] + 1) */

//************************** Internal FS
// typedef xr_vector<BYTE> vecB;
class LZfs
{
private:
    unsigned getbuf;
    unsigned getlen;

    unsigned putbuf;
    unsigned putlen;

    u8* in_start;
    u8* in_end;
    u8* in_iterator;

    u8* out_start;
    u8* out_end;
    u8* out_iterator;

public:
    IC int _getb()
    {
        if (in_iterator == in_end)
            return EOF;
        return *in_iterator++;
    }
    IC void _putb(int c)
    {
        if (out_iterator == out_end)
        {
            u32 out_size = u32(out_end - out_start);
            out_start = (u8*)xr_realloc(out_start, out_size + 1024);
            out_iterator = out_start + out_size;
            out_end = out_iterator + 1024;
        }
        *out_iterator++ = u8(c & 0xFF);
    }

    LZfs() : getbuf(0), getlen(0), putbuf(0), putlen(0)
    {
        in_start = in_end = in_iterator = 0;
        out_start = out_end = out_iterator = 0;
    }

    IC void Init_Input(u8* _start, u8* _end)
    {
        // input
        in_start = _start;
        in_end = _end;
        in_iterator = in_start;

        // bitwise input/output
        getbuf = getlen = putbuf = putlen = 0;
    }
    IC void Init_Output(int _rsize)
    {
        // output
        out_start = (u8*)xr_malloc(_rsize);
        out_end = out_start + _rsize;
        out_iterator = out_start;
    }
    IC u32 InputSize() { return u32(in_end - in_start); }
    IC u32 OutSize() { return u32(out_iterator - out_start); }
    IC u8* OutPointer() { return out_start; }
    IC void OutRelease()
    {
        xr_free(out_start);
        out_start = 0;
        out_end = 0;
        out_iterator = 0;
    }
    IC int GetBit(void) /* get one bit */
    {
        unsigned i;

        while (getlen <= 8)
        {
            if ((int)(i = _getb()) < 0)
                i = 0;
            getbuf |= i << (8 - getlen);
            getlen += 8;
        }
        i = getbuf;
        getbuf <<= 1;
        getlen--;
        return (int)((i & 0x8000) >> 15);
    }

    IC int GetByte(void) /* get one byte */
    {
        unsigned i;

        while (getlen <= 8)
        {
            if ((int)(i = _getb()) < 0)
                i = 0;
            getbuf |= i << (8 - getlen);
            getlen += 8;
        }
        i = getbuf;
        getbuf <<= 8;
        getlen -= 8;
        return (int)((i & 0xff00) >> 8);
    }

    IC void PutCode(int l, unsigned c) /* output c bits of code */
    {
        putbuf |= c >> putlen;
        if ((putlen += l) >= 8)
        {
            _putb(putbuf >> 8);
            if ((putlen -= 8) >= 8)
            {
                _putb(putbuf);
                codesize += 2;
                putlen -= 8;
                putbuf = c << (l - putlen);
            }
            else
            {
                putbuf <<= 8;
                codesize++;
            }
        }
    }
    IC void PutFlush()
    {
        if (putlen)
        {
            _putb(putbuf >> 8);
            codesize++;
        }
    }
};
static LZfs fs;
//************************** Internal FS
IC void InitTree(void) /* initialize trees */
{
    int i;

    for (i = N + 1; i <= N + 256; i++)
        rson[i] = NIL; /* root */
    for (i = 0; i < N; i++)
        dad[i] = NIL; /* node */
}

void InsertNode(int r) /* insert to tree */
{
    int i, p, cmp;
    u8* key;
    unsigned c;

    cmp = 1;
    key = &text_buf[r];
    p = N + 1 + key[0];
    rson[r] = lson[r] = NIL;
    match_length = 0;
    for (;;)
    {
        if (cmp >= 0)
        {
            if (rson[p] != NIL)
                p = rson[p];
            else
            {
                rson[p] = r;
                dad[r] = p;
                return;
            }
        }
        else
        {
            if (lson[p] != NIL)
                p = lson[p];
            else
            {
                lson[p] = r;
                dad[r] = p;
                return;
            }
        }
        for (i = 1; i < F; i++)
            if ((cmp = key[i] - text_buf[p + i]) != 0)
                break;
        if (i > THRESHOLD)
        {
            if (i > match_length)
            {
                match_position = ((r - p) & (N - 1)) - 1;
                if ((match_length = i) >= F)
                    break;
            }
            if (i == match_length)
            {
                if ((c = ((r - p) & (N - 1)) - 1) < (unsigned)match_position)
                {
                    match_position = c;
                }
            }
        }
    }
    dad[r] = dad[p];
    lson[r] = lson[p];
    rson[r] = rson[p];
    dad[lson[p]] = r;
    dad[rson[p]] = r;
    if (rson[dad[p]] == p)
        rson[dad[p]] = r;
    else
        lson[dad[p]] = r;
    dad[p] = NIL; /* remove p */
}

void DeleteNode(int p) /* remove from tree */
{
    int q;

    if (dad[p] == NIL)
        return; /* not registered */
    if (rson[p] == NIL)
        q = lson[p];
    else
    {
        if (lson[p] == NIL)
        {
            q = rson[p];
        }
        else
        {
            q = lson[p];
            if (rson[q] != NIL)
            {
                do
                {
                    q = rson[q];
                } while (rson[q] != NIL);

                rson[dad[q]] = lson[q];
                dad[lson[q]] = dad[q];
                lson[q] = lson[p];
                dad[lson[p]] = q;
            }
            rson[q] = rson[p];
            dad[rson[p]] = q;
        }
    }
    dad[q] = dad[p];
    if (rson[dad[p]] == p)
        rson[dad[p]] = q;
    else
        lson[dad[p]] = q;
    dad[p] = NIL;
}

/* Huffman coding */

/* table for encoding and decoding the upper 6 bits of position */
/* for encoding */
u8 p_len[64] = {0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

u8 p_code[64] = {0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C, 0xA0,
    0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE, 0xD0, 0xD2, 0xD4, 0xD6,
    0xD8, 0xDA, 0xDC, 0xDE, 0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF};

/* for decoding */
u8 d_code[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A,
    0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D,
    0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12,
    0x12, 0x13, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17,
    0x17, 0x17, 0x18, 0x18, 0x19, 0x19, 0x1A, 0x1A, 0x1B, 0x1B, 0x1C, 0x1C, 0x1D, 0x1D, 0x1E, 0x1E, 0x1F, 0x1F, 0x20,
    0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29,
    0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
};

u8 d_len[256] = {
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

/* initialization of tree */

void StartHuff(void)
{
    int i, j;

    for (i = 0; i < N_CHAR; i++)
    {
        freq[i] = 1;
        son[i] = i + T;
        prnt[i + T] = i;
    }
    i = 0;
    j = N_CHAR;
    while (j <= R)
    {
        freq[j] = freq[i] + freq[i + 1];
        son[j] = i;
        prnt[i] = prnt[i + 1] = j;
        i += 2;
        j++;
    }
    freq[T] = 0xffff;
    prnt[R] = 0;
}

/* reconstruction of tree */
void reconst(void)
{
    int i, j, k;
    unsigned f, l;

    /* collect leaf nodes in the first half of the table */
    /* and replace the freq by (freq + 1) / 2. */
    j = 0;
    for (i = 0; i < T; i++)
    {
        if (son[i] >= T)
        {
            freq[j] = (freq[i] + 1) / 2;
            son[j] = son[i];
            j++;
        }
    }
    /* begin constructing tree by connecting sons */
    for (i = 0, j = N_CHAR; j < T; i += 2, j++)
    {
        k = i + 1;
        f = freq[j] = freq[i] + freq[k];
        for (k = j - 1; f < freq[k]; k--)
            ;
        k++;
        l = (j - k) * sizeof(unsigned);
        memmove(&freq[k + 1], &freq[k], l);
        freq[k] = f;
        memmove(&son[k + 1], &son[k], l);
        son[k] = i;
    }
    /* connect prnt */
    for (i = 0; i < T; i++)
    {
        if ((k = son[i]) >= T)
        {
            prnt[k] = i;
        }
        else
        {
            prnt[k] = prnt[k + 1] = i;
        }
    }
}

/* increment frequency of given code by one, and update tree */
void update(int c)
{
    int i, j, k, l;

    if (freq[R] == MAX_FREQ)
    {
        reconst();
    }
    c = prnt[c + T];
    do
    {
        k = ++freq[c];

        /* if the order is disturbed, exchange nodes */
        if ((unsigned)k > freq[l = c + 1])
        {
            while ((unsigned)k > freq[++l])
                ;
            l--;
            freq[c] = freq[l];
            freq[l] = k;

            i = son[c];
            prnt[i] = l;
            if (i < T)
                prnt[i + 1] = l;

            j = son[l];
            son[l] = i;

            prnt[j] = c;
            if (j < T)
                prnt[j + 1] = c;
            son[c] = j;

            c = l;
        }
    } while ((c = prnt[c]) != 0); /* repeat up to root */
}

void EncodeChar(unsigned c)
{
    unsigned i;
    int j, k;

    i = 0;
    j = 0;
    k = prnt[c + T];

    /* travel from leaf to root */
    do
    {
        i >>= 1;

        /* if node's address is odd-numbered, choose bigger brother node */
        if (k & 1)
            i += 0x8000;

        j++;
        k = prnt[k];
    } while (k != R);
    fs.PutCode(j, i);
    update(c);
}

void EncodePosition(unsigned c)
{
    unsigned i;

    /* output upper 6 bits by table lookup */
    i = c >> 6;
    fs.PutCode(p_len[i], (unsigned)p_code[i] << 8);

    /* output lower 6 bits verbatim */
    fs.PutCode(6, (c & 0x3f) << 10);
}

int DecodeChar(void)
{
    unsigned c;

    c = son[R];

    /* travel from root to leaf, */
    /* choosing the smaller child node (son[]) if the read bit is 0, */
    /* the bigger (son[]+1} if 1 */
    while (c < T)
    {
        c += fs.GetBit();
        c = son[c];
    }
    c -= T;
    update(c);
    return (int)c;
}

int DecodePosition(void)
{
    unsigned i, j, c;

    /* recover upper 6 bits from table */
    i = fs.GetByte();
    c = (unsigned)d_code[i] << 6;
    j = d_len[i];

    /* read lower 6 bits verbatim */
    j -= 2;
    while (j--)
    {
        i = (i << 1) + fs.GetBit();
    }
    return (int)(c | (i & 0x3f));
}

/* compression */
void Encode(void) /* compression */
{
    int i, c, r, s, last_match_length;

    textsize = fs.InputSize();
    fs.Init_Output(textsize);
    fs._putb((textsize & 0xff));
    fs._putb((textsize & 0xff00) >> 8);
    fs._putb((textsize & 0xff0000L) >> 16);
    fs._putb((textsize & 0xff000000L) >> 24);
    if (textsize == 0)
        return;
    textsize = 0; /* rewind and re-read */
    StartHuff();
    InitTree();
    s = 0;
    r = N - F;
    for (i = s; i < r; i++)
        text_buf[i] = 0x20;
    int len = 0;
    while (len < F && (c = fs._getb()) != EOF)
    {
        text_buf[r + len] = (unsigned char)c;
        len++;
    }
    textsize = len;
    for (i = 1; i <= F; i++)
        InsertNode(r - i);
    InsertNode(r);
    do
    {
        if (match_length > len)
            match_length = len;
        if (match_length <= THRESHOLD)
        {
            match_length = 1;
            // textsize==56158 - FATAL :(
            EncodeChar(text_buf[r]);
        }
        else
        {
            EncodeChar(255 - THRESHOLD + match_length);
            EncodePosition(match_position);
        }
        last_match_length = match_length;
        for (i = 0; i < last_match_length && (c = fs._getb()) != EOF; i++)
        {
            DeleteNode(s);
            text_buf[s] = (unsigned char)c;
            if (s < F - 1)
                text_buf[s + N] = (unsigned char)c;
            s = (s + 1) & (N - 1);
            r = (r + 1) & (N - 1);
            InsertNode(r);
        }
        textsize += i;
        while (i++ < last_match_length)
        {
            DeleteNode(s);
            s = (s + 1) & (N - 1);
            r = (r + 1) & (N - 1);
            if (--len)
                InsertNode(r);
        }
    } while (len > 0);
    fs.PutFlush();
}

bool Decode(int total_size) /* recover */
{
    int i, j, k, r, c;
    unsigned int count;

    textsize = (fs._getb());
    textsize |= (fs._getb() << 8);
    textsize |= (fs._getb() << 16);
    textsize |= (fs._getb() << 24);
    if (textsize == 0)
        return false;
    if (total_size != -1 && textsize > total_size)
        return false;

    fs.Init_Output(textsize);

    StartHuff();
    for (i = 0; i < N - F; i++)
        text_buf[i] = 0x20;
    r = N - F;
    for (count = 0; count < textsize;)
    {
        c = DecodeChar();
        if (c < 256)
        {
            fs._putb(c);
            text_buf[r++] = (unsigned char)c;
            r &= (N - 1);
            count++;
        }
        else
        {
            i = (r - DecodePosition() - 1) & (N - 1);
            j = c - 255 + THRESHOLD;
            for (k = 0; k < j; k++)
            {
                c = text_buf[(i + k) & (N - 1)];
                fs._putb(c);
                text_buf[r++] = (unsigned char)c;
                r &= (N - 1);
                count++;
            }
        }
    }

    return true;
}

unsigned _writeLZ(int hf, void* d, unsigned size)
{
    u8* start = (u8*)d;
    fs.Init_Input(start, start + size);

    // Actual compression
    Encode();
    // Flush cache
    int size_out = fs.OutSize();
    if (size_out)
        _write(hf, fs.OutPointer(), size_out);
    fs.OutRelease();
    return size_out;
}

void _compressLZ(u8** dest, unsigned* dest_sz, void* src, unsigned src_sz)
{
    u8* start = (u8*)src;
    fs.Init_Input(start, start + src_sz);
    Encode();
    *dest = fs.OutPointer();
    *dest_sz = fs.OutSize();
}

bool _decompressLZ(u8** dest, unsigned* dest_sz, void* src, unsigned src_sz, int total_size /*= -1*/)
{
    u8* start = (u8*)src;
    fs.Init_Input(start, start + src_sz);

    if (!Decode(total_size))
        return false;

    *dest = fs.OutPointer();
    *dest_sz = fs.OutSize();
    return true;
}

unsigned _readLZ(int hf, void*& d, unsigned size)
{
    // Read file in memory
    u8* data = (u8*)xr_malloc(size);
    _read(hf, data, size);

    fs.Init_Input(data, data + size);

    // Actual compression
    Decode(-1);

    // Flush cache
    xr_free(data);
    d = fs.OutPointer();
    return fs.OutSize();
}
