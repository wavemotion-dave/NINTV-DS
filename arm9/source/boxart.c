// =====================================================================================
// Copyright (c) 2021-2026 Dave Bernazzani (wavemotion-dave)
//
// Copying and distribution of this emulator, its source code and associated
// readme files, with or without modification, are permitted in any medium without
// royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
// and Kyle Davis (BLISS) are thanked profusely.
//
// The NINTV-DS emulator is offered as-is, without any warranty.
// =====================================================================================

// =====================================================================================
// Box art display helper: reads a 148x204 PNG and paints its cropped portion on the
// top screen. Yes, this is an odd size... but it happens to match the 'small' artwork
// of the Sprint.
//
// SELF-CONTAINED: includes its own small PNG decoder (zlib inflate + scanline unfilter),
// so there is nothing to download and nothing extra to link.
//
// Supported PNGs: exactly 148x204, NOT interlaced, any of: palette (1/2/4/8-bit),
// grayscale (1/2/4/8/16-bit), gray+alpha, RGB and RGBA (8 or 16-bit). Alpha is ignored.
//
// RAM: one scanline buffer of (rowbytes+1)*204 bytes, malloc'd and freed before return
// (about 49KB for an 8-bit palette image, 148KB for RGB, 197KB for RGBA) plus ~4KB.
// The compressed file is streamed, never loaded whole. Save as RGB or palette (not RGBA)
// to keep memory lowest. Decode is bit-by-bit and simple: expect a fraction of a second
// on real hardware, so this is only called from the menu/loader screen pre-gameplay.
//
// Display method: main 2D engine, BG3 as a 16-bit direct-color bitmap in VRAM bank A.
// No palette is touched, so the emulator's 256-color BG_PALETTE is left alone. This DOES
// reconfigure the main engine and overwrite VRAM bank A, so we re-run the normal emulator
// video setup afterwards.
// =====================================================================================
#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fat.h>
#include <dirent.h>
#include <unistd.h>
#include "config.h"
#include "nintv-ds.h"

#define BOXART_WIDTH        148     // PNG source width
#define BOXART_HEIGHT       204     // PNG source height
#define BOXART_SCREEN_WIDTH 256
#define BOXART_SCREEN_HEIGHT 192
#define BOXART_X_OFFSET     ((BOXART_SCREEN_WIDTH - BOXART_WIDTH) / 2) // 54 pixels
#define BOXART_TOP_CROP     3       // Skip first 3 source rows
#define BOXART_VRAM_BANK    VRAM_A_MAIN_BG  // 128K bank holds the 256x256x16bpp bitmap

#define PNG_IHDR  0x49484452u
#define PNG_PLTE  0x504C5445u
#define PNG_IDAT  0x49444154u
#define PNG_IEND  0x49454E44u

typedef struct
{
    FILE          *fp;
    unsigned char  buf[2048];       // Buffered file input
    int            pos, len;
    unsigned       chunk_left;      // Bytes left in the current IDAT chunk
    int            in_idat;         // An IDAT chunk was started (its CRC is pending when it ends)
    int            idat_done;
    unsigned       bitbuf;          // Inflate bit reader
    int            bitcnt;
    unsigned char *out;             // Inflate output (== the raw PNG scanlines)
    unsigned       outpos, outcap;
    int            err;
} PngState;

PngState myPNG;

// ------------------------------------------------------------------------------------
// Input: buffered file bytes, and a byte source that flows across IDAT chunk boundaries
// ------------------------------------------------------------------------------------
static int raw_get(PngState *s)
{
    if (s->pos >= s->len)
    {
        s->len = (int)fread(s->buf, 1, sizeof(s->buf), s->fp);
        s->pos = 0;
        if (s->len <= 0) { s->len = 0; return -1; }
    }
    return s->buf[s->pos++];
}

static int read_u32(PngState *s, unsigned *v)
{
    unsigned r = 0;
    for (int i = 0; i < 4; i++)
    {
        int c = raw_get(s);
        if (c < 0) return -1;
        r = (r << 8) | (unsigned)c;
    }
    *v = r;
    return 0;
}

static int skip_bytes(PngState *s, unsigned n)
{
    while (n--) if (raw_get(s) < 0) return -1;
    return 0;
}

static int idat_get(PngState *s)
{
    while (s->chunk_left == 0)
    {
        unsigned len, type;
        if (s->idat_done) return -1;
        if (s->in_idat && skip_bytes(s, 4)) { s->idat_done = 1; return -1; }   // CRC of the chunk that just ended
        if (read_u32(s, &len) || read_u32(s, &type) || type != PNG_IDAT) { s->idat_done = 1; return -1; }
        s->in_idat = 1;
        s->chunk_left = len;
    }
    s->chunk_left--;
    return raw_get(s);
}

// ------------------------------------------------------------------------------------
// Inflate (RFC 1951) - a compact canonical-Huffman decoder in the style of zlib's puff.c.
// The output buffer doubles as the 32K history window, so no separate window is needed.
// ------------------------------------------------------------------------------------
#define MAXBITS    15
#define MAXLCODES  286
#define MAXDCODES  30
#define MAXCODES   (MAXLCODES + MAXDCODES)
#define FIXLCODES  288

typedef struct { short count[MAXBITS + 1]; short symbol[MAXCODES]; } Huffman;

// 8-bit fast Huffman lookup tables. Entries pack code length in the upper bits
// and symbol in the lower 9 bits. Long codes use the original slow decoder.
#define HUFF_FAST_BITS 8
#define HUFF_FAST_SIZE (1u << HUFF_FAST_BITS)
#define HUFF_FAST_SYMBOL_MASK 0x01FFu
#define HUFF_FAST_LEN_SHIFT 9
static unsigned short fast_lit_table[HUFF_FAST_SIZE];
static unsigned short fast_dist_table[HUFF_FAST_SIZE];
static unsigned short fixed_lit_table[HUFF_FAST_SIZE];
static unsigned short fixed_dist_table[HUFF_FAST_SIZE];

static int bits(PngState *s, int need)
{
    long val = s->bitbuf;
    while (s->bitcnt < need)
    {
        int b = idat_get(s);
        if (b < 0) { s->err = 1; return 0; }
        val |= (long)b << s->bitcnt;
        s->bitcnt += 8;
    }
    s->bitbuf = (unsigned)(val >> need);
    s->bitcnt -= need;
    return (int)(val & ((1L << need) - 1));
}

static int decode(PngState *s, const Huffman *h)
{
    int len, code = 0, first = 0, count, index = 0;
    for (len = 1; len <= MAXBITS; len++)
    {
        code |= bits(s, 1);
        if (s->err) return -1;
        count = h->count[len];
        if (code - count < first) return h->symbol[index + (code - first)];
        index += count;
        first += count;
        first <<= 1;
        code <<= 1;
    }
    return -10;
}

static int construct(Huffman *h, const short *length, int n)
{
    int symbol, len, left;
    static short offs[MAXBITS + 1];

    for (len = 0; len <= MAXBITS; len++) h->count[len] = 0;
    for (symbol = 0; symbol < n; symbol++) h->count[length[symbol]]++;
    if (h->count[0] == n) return 0;

    left = 1;
    for (len = 1; len <= MAXBITS; len++)
    {
        left <<= 1;
        left -= h->count[len];
        if (left < 0) return left;
    }

    offs[1] = 0;
    for (len = 1; len < MAXBITS; len++) offs[len + 1] = offs[len] + h->count[len];
    for (symbol = 0; symbol < n; symbol++)
        if (length[symbol] != 0) h->symbol[offs[length[symbol]]++] = (short)symbol;

    return left;
}

// Build a low-bit-first lookup table for codes up to HUFF_FAST_BITS long.
static void build_fast_table(const Huffman *h, const short *length, int n,
                             unsigned short *table)
{
    unsigned next_code[MAXBITS + 1];
    unsigned code = 0;

    for (int i = 0; i < (int)HUFF_FAST_SIZE; i++) table[i] = 0;

    for (int len = 1; len <= MAXBITS; len++)
    {
        // count[0] includes unused symbols in this representation; canonical
        // code assignment must treat it as zero.
        code = (code + (len > 1 ? (unsigned)h->count[len - 1] : 0u)) << 1;
        next_code[len] = code;
    }

    for (int symbol = 0; symbol < n; symbol++)
    {
        int len = length[symbol];
        if (len <= 0 || len > HUFF_FAST_BITS) continue;

        unsigned c = next_code[len]++;
        unsigned rev = 0;
        for (int b = 0; b < len; b++)
        {
            rev = (rev << 1) | (c & 1u);
            c >>= 1;
        }

        unsigned entry = ((unsigned)len << HUFF_FAST_LEN_SHIFT) | (unsigned)symbol;
        for (unsigned i = rev; i < HUFF_FAST_SIZE; i += (1u << len))
            table[i] = (unsigned short)entry;
    }
}

static int decode_fast(PngState *s, const Huffman *h, const unsigned short *table)
{
    // Fill enough bits to index the table, but allow a short final code to
    // fall back to the original decoder if the stream ends before 8 bits.
    while (s->bitcnt < HUFF_FAST_BITS)
    {
        int b = idat_get(s);
        if (b < 0) break;
        s->bitbuf |= (unsigned)b << s->bitcnt;
        s->bitcnt += 8;
    }

    if (s->bitcnt >= HUFF_FAST_BITS)
    {
        unsigned entry = table[s->bitbuf & (HUFF_FAST_SIZE - 1)];
        if (entry)
        {
            unsigned len = entry >> HUFF_FAST_LEN_SHIFT;
            s->bitbuf >>= len;
            s->bitcnt -= (int)len;
            return (int)(entry & HUFF_FAST_SYMBOL_MASK);
        }
    }

    return decode(s, h);
}

static int codes(PngState *s, const Huffman *lencode, const Huffman *distcode,
                 const unsigned short *lit_table, const unsigned short *dist_table)
{
    static const short lens[29]  = {3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258};
    static const short lext[29]  = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};
    static const short dists[30] = {1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577};
    static const short dext[30]  = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};
    int symbol;

    do
    {
        symbol = decode_fast(s, lencode, lit_table);
        if (symbol < 0) return symbol;
        if (symbol < 256)
        {
            if (s->outpos >= s->outcap) return 1;
            s->out[s->outpos++] = (unsigned char)symbol;
        }
        else if (symbol > 256)
        {
            unsigned len, dist;
            symbol -= 257;
            if (symbol >= 29) return -10;
            len = (unsigned)(lens[symbol] + bits(s, lext[symbol]));
            symbol = decode_fast(s, distcode, dist_table);
            if (symbol < 0) return symbol;
            dist = (unsigned)(dists[symbol] + bits(s, dext[symbol]));
            if (s->err) return -1;
            if (dist > s->outpos) return -11;
            if (s->outpos + len > s->outcap) return 1;
            while (len--)
            {
                s->out[s->outpos] = s->out[s->outpos - dist];
                s->outpos++;
            }
        }
    } while (symbol != 256);

    return 0;
}

static int stored_block(PngState *s)
{
    s->bitbuf = 0;
    s->bitcnt = 0;

    int b0 = idat_get(s), b1 = idat_get(s), b2 = idat_get(s), b3 = idat_get(s);
    if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0) return -1;

    unsigned len  = (unsigned)(b0 | (b1 << 8));
    unsigned nlen = (unsigned)(b2 | (b3 << 8));
    if (len != (~nlen & 0xFFFFu)) return -2;
    if (s->outpos + len > s->outcap) return 1;

    while (len--)
    {
        int c = idat_get(s);
        if (c < 0) return -1;
        s->out[s->outpos++] = (unsigned char)c;
    }
    return 0;
}

static int fixed_block(PngState *s)
{
    static Huffman lencode, distcode;
    static short lengths[FIXLCODES];
    static int initialized = 0;
    int symbol;

    if (!initialized)
    {
        for (symbol = 0;   symbol < 144; symbol++) lengths[symbol] = 8;
        for (; symbol < 256; symbol++) lengths[symbol] = 9;
        for (; symbol < 280; symbol++) lengths[symbol] = 7;
        for (; symbol < FIXLCODES; symbol++) lengths[symbol] = 8;
        construct(&lencode, lengths, FIXLCODES);
        build_fast_table(&lencode, lengths, FIXLCODES, fixed_lit_table);

        for (symbol = 0; symbol < MAXDCODES; symbol++) lengths[symbol] = 5;
        construct(&distcode, lengths, MAXDCODES);
        build_fast_table(&distcode, lengths, MAXDCODES, fixed_dist_table);
        initialized = 1;
    }

    return codes(s, &lencode, &distcode, fixed_lit_table, fixed_dist_table);
}

static int dynamic_block(PngState *s)
{
    static const short order[19] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    Huffman lencode, distcode;
    static short lengths[MAXCODES];
    int nlen, ndist, ncode, index, err;

    nlen  = bits(s, 5) + 257;
    ndist = bits(s, 5) + 1;
    ncode = bits(s, 4) + 4;
    if (s->err || nlen > MAXLCODES || ndist > MAXDCODES) return -3;

    for (index = 0; index < ncode; index++) lengths[order[index]] = (short)bits(s, 3);
    for (; index < 19; index++) lengths[order[index]] = 0;
    if (s->err) return -1;

    err = construct(&lencode, lengths, 19);
    if (err != 0) return -4;

    index = 0;
    while (index < nlen + ndist)
    {
        int symbol = decode(s, &lencode);
        if (symbol < 0) return symbol;
        if (symbol < 16)
        {
            lengths[index++] = (short)symbol;
        }
        else
        {
            int len = 0;
            if (symbol == 16)
            {
                if (index == 0) return -5;
                len = lengths[index - 1];
                symbol = 3 + bits(s, 2);
            }
            else if (symbol == 17) symbol = 3 + bits(s, 3);
            else                   symbol = 11 + bits(s, 7);
            if (s->err) return -1;
            if (index + symbol > nlen + ndist) return -6;
            while (symbol--) lengths[index++] = (short)len;
        }
    }

    if (lengths[256] == 0) return -9;

    err = construct(&lencode, lengths, nlen);
    if (err && (err < 0 || nlen != lencode.count[0] + lencode.count[1])) return -7;
    build_fast_table(&lencode, lengths, nlen, fast_lit_table);

    err = construct(&distcode, lengths + nlen, ndist);
    if (err && (err < 0 || ndist != distcode.count[0] + distcode.count[1])) return -8;
    build_fast_table(&distcode, lengths + nlen, ndist, fast_dist_table);

    return codes(s, &lencode, &distcode, fast_lit_table, fast_dist_table);
}

static int inflate_stream(PngState *s)
{
    int last, type, err;
    do
    {
        last = bits(s, 1);
        type = bits(s, 2);
        if (s->err) return -1;
        if      (type == 0) err = stored_block(s);
        else if (type == 1) err = fixed_block(s);
        else if (type == 2) err = dynamic_block(s);
        else                err = -1;
        if (err != 0) return err;
    } while (!last);
    return 0;
}

// ------------------------------------------------------------------------------------
// PNG scanline unfiltering (in place). Each row is: 1 filter-type byte + rowbytes of data.
// ------------------------------------------------------------------------------------
static int unfilter(unsigned char *raw, int rowbytes, int height, int bpp)
{
    const unsigned char *prev = NULL;

    for (int y = 0; y < height; y++)
    {
        unsigned char *row = raw + (size_t)y * (rowbytes + 1);
        int ft = row[0];
        row++;

        switch (ft)
        {
            case 0: break;
            case 1:
                for (int i = bpp; i < rowbytes; i++) row[i] = (unsigned char)(row[i] + row[i - bpp]);
                break;
            case 2:
                if (prev) for (int i = 0; i < rowbytes; i++) row[i] = (unsigned char)(row[i] + prev[i]);
                break;
            case 3:
                for (int i = 0; i < rowbytes; i++)
                {
                    int a = (i >= bpp) ? row[i - bpp] : 0;
                    int b = prev ? prev[i] : 0;
                    row[i] = (unsigned char)(row[i] + ((a + b) >> 1));
                }
                break;
            case 4:
                for (int i = 0; i < rowbytes; i++)
                {
                    int a = (i >= bpp) ? row[i - bpp] : 0;
                    int b = prev ? prev[i] : 0;
                    int c = (prev && i >= bpp) ? prev[i - bpp] : 0;
                    int p  = a + b - c;
                    int pa = p > a ? p - a : a - p;
                    int pb = p > b ? p - b : b - p;
                    int pc = p > c ? p - c : c - p;
                    int pred = (pa <= pb && pa <= pc) ? a : (pb <= pc ? b : c);
                    row[i] = (unsigned char)(row[i] + pred);
                }
                break;
            default:
                return -1;
        }
        prev = row;
    }
    return 0;
}

#define TO_RGB15(r,g,b)  (RGB15((r) >> 3, (g) >> 3, (b) >> 3) | BIT(15))   // BIT(15) = opaque

// ------------------------------------------------------------------------------------
// The public helper
// ------------------------------------------------------------------------------------
char box_art_filename[256];

// -------------------------------------------------------------------------
// Store the save state file - use global config to determine where to 
// place the .png file (or, by default, it goes in the same directory 
// as the game .rom or .bin file that was loaded).
// -------------------------------------------------------------------------
void get_boxart_filename(const char *filename, u8 isDirectory)
{
    if (myGlobalConfig.boxart_dir == 1) 
    {
        DIR* dir = opendir("/roms/art");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/roms/art", 0777);      // Doesn't exist - make it...
        strcpy(box_art_filename, "/roms/art/");
    }
    else if (myGlobalConfig.boxart_dir == 2)
    {
        DIR* dir = opendir("/roms/intv/art");   // See if directory exists
        if (dir) closedir(dir);                 // Directory exists. All good.
        else mkdir("/roms/intv/art", 0777);     // Doesn't exist - make it...
        strcpy(box_art_filename, "/roms/intv/art/");
    }
    else if (myGlobalConfig.boxart_dir == 3)
    {
        DIR* dir = opendir("/data/art");    // See if directory exists
        if (dir) closedir(dir);             // Directory exists. All good.
        else mkdir("/data/art", 0777);      // Doesn't exist - make it...
        strcpy(box_art_filename, "/data/art/");
    }
    else strcpy(box_art_filename, "");
    
    if (isDirectory)
    {
        strcat(box_art_filename, "[");
        strcat(box_art_filename, filename);
        strcat(box_art_filename, "]");
    }
    else
    {
        strcat(box_art_filename, filename);
        box_art_filename[strlen(box_art_filename)-4] = 0;
    }
    strcat(box_art_filename, ".png");
}

bool BoxArt_ShowPNG(const char *filename, u8 isDirectory)
{
    static const unsigned char png_sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    PngState      *s   = NULL;
    unsigned char *raw = NULL;
    bool           ok  = false;
    static u16     pal[256];
    int            i, y, x;
    unsigned       width = 0, height = 0, depth = 0, ctype = 99;
    int            have_ihdr = 0, have_plte = 0;
    int            channels, rowbytes, bpp, indexed;

    // Same base filename but png
    get_boxart_filename(filename, isDirectory);
    
    memset(&myPNG, 0x00, sizeof(myPNG));
    s = (PngState *)&myPNG;

    s->fp = fopen(box_art_filename, "rb");
    if (!s->fp) { return false; }

    for (i = 0; i < 256; i++) pal[i] = BIT(15);     // Unused palette entries = black

    // ---- Signature ----
    for (i = 0; i < 8; i++)
        if (raw_get(s) != png_sig[i]) goto done;

    // ---- Chunks up to and including the start of the first IDAT ----
    for (;;)
    {
        unsigned len, type;
        if (read_u32(s, &len) || read_u32(s, &type)) goto done;

        if (type == PNG_IHDR)
        {
            unsigned char h[13];
            if (len != 13) goto done;
            for (i = 0; i < 13; i++) { int c = raw_get(s); if (c < 0) goto done; h[i] = (unsigned char)c; }
            width  = ((unsigned)h[0] << 24) | ((unsigned)h[1] << 16) | ((unsigned)h[2] << 8) | h[3];
            height = ((unsigned)h[4] << 24) | ((unsigned)h[5] << 16) | ((unsigned)h[6] << 8) | h[7];
            depth  = h[8];
            ctype  = h[9];
            if (width != BOXART_WIDTH || height != BOXART_HEIGHT) goto done;
            if (h[10] != 0 || h[11] != 0 || h[12] != 0) goto done;     // Compression, filter, and NO interlace
            if (ctype == 0)      { if (depth != 1 && depth != 2 && depth != 4 && depth != 8 && depth != 16) goto done; }
            else if (ctype == 3) { if (depth != 1 && depth != 2 && depth != 4 && depth != 8) goto done; }
            else if (ctype == 2 || ctype == 4 || ctype == 6) { if (depth != 8 && depth != 16) goto done; }
            else goto done;
            if (skip_bytes(s, 4)) goto done;                            // CRC
            have_ihdr = 1;
        }
        else if (type == PNG_PLTE)
        {
            if (!have_ihdr || len == 0 || len > 768 || (len % 3)) goto done;
            for (i = 0; i < (int)(len / 3); i++)
            {
                int r = raw_get(s), g = raw_get(s), b = raw_get(s);
                if (r < 0 || g < 0 || b < 0) goto done;
                pal[i] = TO_RGB15(r, g, b);
            }
            have_plte = 1;
            if (skip_bytes(s, 4)) goto done;
        }
        else if (type == PNG_IDAT)
        {
            if (!have_ihdr) goto done;
            s->chunk_left = len;
            s->in_idat = 1;
            break;
        }
        else if (type == PNG_IEND)
        {
            goto done;
        }
        else
        {
            if (skip_bytes(s, len + 4)) goto done;                      // Ignore ancillary chunks
        }
    }

    // ---- Work out the pixel layout ----
    indexed  = (ctype == 3) || (ctype == 0 && depth <= 8);
    channels = (ctype == 2) ? 3 : (ctype == 4) ? 2 : (ctype == 6) ? 4 : 1;
    rowbytes = (int)((width * (unsigned)channels * depth + 7) / 8);
    bpp      = (int)(((unsigned)channels * depth + 7) / 8);

    if (ctype == 3 && !have_plte) goto done;
    if (ctype == 0 && depth <= 8)                                       // Grayscale -> build a gray palette
    {
        int maxv = (1 << depth) - 1;
        for (i = 0; i <= maxv; i++) { int g = (i * 255) / maxv; pal[i] = TO_RGB15(g, g, g); }
    }

    // ---- Inflate the whole image into the scanline buffer ----
    s->outcap = (unsigned)(rowbytes + 1) * height;
    raw = (unsigned char *) ScratchBuffer;
    s->out = raw;

    {
        int cmf = idat_get(s), flg = idat_get(s);
        if (cmf < 0 || flg < 0 || (cmf & 15) != 8 || (((cmf << 8) | flg) % 31) != 0 || (flg & 0x20)) goto done;
    }
    if (inflate_stream(s) != 0 || s->err || s->outpos != s->outcap) goto done;
    if (unfilter(raw, rowbytes, (int)height, bpp) != 0) goto done;

    // ---- Everything decoded fine: NOW take over the top screen ----
    {
        swiWaitForVBlank();
        lcdMainOnTop();
        videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
        vramSetBankA(BOXART_VRAM_BANK);
        int bg = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
        u16 *gfx = bgGetGfxPtr(bg);
        int bps = (int)(depth >> 3);                // Bytes per sample for the non-indexed (8/16-bit) formats

        // Clear the visible screen area to black, then draw the centered, vertically cropped image.
        for (y = 0; y < BOXART_SCREEN_HEIGHT; y++)
            for (x = 0; x < BOXART_SCREEN_WIDTH; x++)
                gfx[y * BOXART_SCREEN_WIDTH + x] = BIT(15);

        for (y = 0; y < BOXART_SCREEN_HEIGHT; y++)
        {
            // Crop 3 rows from the top; displaying 192 rows leaves 9 cropped from the bottom.
            const unsigned char *row = raw + (size_t)(y + BOXART_TOP_CROP) * (rowbytes + 1) + 1;
            u16 *dst = gfx + (y * BOXART_SCREEN_WIDTH) + BOXART_X_OFFSET;

            if (indexed)
            {
                switch (depth)
                {
                    case 8: for (x = 0; x < BOXART_WIDTH; x++) dst[x] = pal[row[x]]; break;
                    case 4: for (x = 0; x < BOXART_WIDTH; x++) dst[x] = pal[(row[x >> 1] >> (4 * (1 - (x & 1)))) & 0x0F]; break;
                    case 2: for (x = 0; x < BOXART_WIDTH; x++) dst[x] = pal[(row[x >> 2] >> (2 * (3 - (x & 3)))) & 0x03]; break;
                    default: for (x = 0; x < BOXART_WIDTH; x++) dst[x] = pal[(row[x >> 3] >> (7 - (x & 7))) & 0x01]; break;
                }
            }
            else if (ctype == 0 || ctype == 4)      // 16-bit gray, or gray+alpha: use the high byte of the gray sample
            {
                int stride = channels * bps;
                for (x = 0; x < BOXART_WIDTH; x++) { int g = row[x * stride]; dst[x] = TO_RGB15(g, g, g); }
            }
            else                                    // RGB / RGBA at 8 or 16 bits per sample (high byte of each)
            {
                int stride = channels * bps;
                for (x = 0; x < BOXART_WIDTH; x++)
                {
                    const unsigned char *p = row + x * stride;
                    dst[x] = TO_RGB15(p[0], p[bps], p[2 * bps]);
                }
            }
        }
    }
    ok = true;

done:
    fclose(s->fp);
    return ok;
}

// End of file
