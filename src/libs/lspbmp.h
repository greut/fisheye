#ifndef INCLUDED_LSPBMP_H
#define INCLUDED_LSPBMP_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct Bitmap
{
    int width;
    int height;
    int depth;
    unsigned char *data;
    // mmap
    size_t length;
    unsigned char *mmap;
} Bitmap;

Bitmap *loadBitmap(const char *fname);
#ifndef WIN32
Bitmap *loadBitmapMode(const char *fname, int mode);
#endif
Bitmap *loadBitmapHeaderOnly(const char *fname);
int saveBitmap(const char *fname, Bitmap *bmp);
Bitmap *createBitmap(int w, int h, int d);
void destroyBitmap(Bitmap *bmp);

#ifdef __cplusplus
}
#endif

#endif
