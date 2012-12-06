#ifndef INCLUDED_LSPBMP_H
#define INCLUDED_LSPBMP_H

#ifdef __cplusplus
extern "C"
{
#endif

/** Bitmap file header */
typedef struct BMHeader
{
    char b, m;
    unsigned char s0, s1, s2, s3;   // File size
    unsigned char r0, r1, r2, r3;   // Reserved
    unsigned char o0, o1, o2, o3;   // Bit offset
} BMHeader;

/** Bitmap descriptor
All fields are split into individual bytes in order to ensure
correct files get written on all platforms independently of
endianness.
*/
typedef struct BMInfo
{
    unsigned char cs0,cs1,cs2,cs3;          // Structure size
    unsigned char w0,w1,w2,w3;              // Width
    unsigned char h0,h1,h2,h3;              // Height
    unsigned char planes0,planes1;          // Planes
    unsigned char bits0,bits1;              // Bits per pixel
    unsigned char cmp0,cmp1,cmp2,cmp3;      // Compression mode
    unsigned char s0,s1,s2,s3;              // image size
    unsigned char xRes, r4,r5,r6;           // X resolution
    unsigned char yRes, r7,r8,r9;           // Y resolution
    unsigned char cu0,cu1,cu2,cu3;          // Colours used
    unsigned char ci0,ci1,ci2,ci3;          // Colours important
} BMInfo;

typedef struct Bitmap
{
	int width;
	int height;
	int depth;
    BMHeader hd;
    BMInfo nf;
    unsigned char *pal;
	unsigned char *data;
} Bitmap;

Bitmap *loadBitmap(const char *fname);
Bitmap *loadBitmapHeader(const char *fname);
int loadBitmapContent(Bitmap *bmp, const char *fname);
int saveBitmap(const char *fname, Bitmap *bmp);
void destroyBitmap(Bitmap *bmp);
Bitmap *createBitmap(int w, int h, int d);

#ifdef __cplusplus
}
#endif

#endif
