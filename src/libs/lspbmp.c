#ifndef WIN32
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lspbmp.h"

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

#ifndef WIN32
Bitmap *loadBitmapMode(const char *fname, int o_mode)
{
    BMHeader hd;
    BMInfo nf;
    unsigned int bo;
    unsigned char pal[1024];
    int mode=0;
    int x,w,h;
    Bitmap *out;
    FILE *fptr;

    if(fname==NULL)
    {
        printf("loadBitmap: NULL filename\n");
        return NULL;
    }

    fptr=fopen(fname,"rb");
    if(fptr==NULL)
    {
        printf("loadBitmap: Could not open '%s'\n",fname);
        return NULL;
    }

    fread(&hd,sizeof(hd),1,fptr);
    fread(&nf,sizeof(nf),1,fptr);

    if(hd.b!='B' || hd.m!='M')
    {
        printf("loadBitmap: Invalid file type in '%s'\n",fname);
        return NULL;
    }
    if(nf.cs0!=sizeof(nf))
    {
        printf("loadBitmap: Unknown file format in '%s'\n",fname);
        return NULL;
    }
    if(nf.bits0==8)
    {
        int cu;
        // Palettized format
        memset(pal,0,1024);
        cu=nf.cu0+(nf.cu1<<8);
        if(cu==0)
            cu=256;
        if(cu>256)
        {
            printf("loadBitmap: Too many palette colors in '%s'\n",fname);
            return NULL;
        }
        fread(pal,cu,4,fptr);
        mode=1;
        for(x=0;x<256;x++)
        {
            if(pal[x*4]!=pal[x*4+1] || pal[x*4]!=pal[x*4+2])
                mode=0;
        }
    }
    else if(nf.bits0!=24 && nf.bits0!=32)
    {
        printf("loadBitmap: Cannot handle %d bpp in '%s'\n",nf.bits0,fname);
        return NULL;
    }

    bo=hd.o0+(hd.o1<<8)+(hd.o2<<16)+(hd.o3<<24);
    fseek(fptr,bo,0);

    w=nf.w0+(nf.w1<<8)+(nf.w2<<16)+(nf.w3<<24);
    h=nf.h0+(nf.h1<<8)+(nf.h2<<16)+(nf.h3<<24);

    out=(Bitmap*)malloc(sizeof(Bitmap));
    if(out==NULL){
        return NULL;
    }

    out->width=w;
    out->height=h;
    out->depth=mode==0?24:8;
    fclose(fptr);
    if(nf.bits0==24)
    {
        int fd = open(fname, o_mode);
        struct stat sb;
        if(fd == -1){
            perror("open");
            return NULL;
        }
        if (fstat(fd, &sb) == -1){
            perror("fstat");
            return NULL;
        }

        off_t offset = bo, pa_offset;
        size_t length = w*h*(mode==0?3:1);
        pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
        if (offset >= sb.st_size) {
            perror("offset is past end of file\n");
            return NULL;
        }
        int flag;
        switch (o_mode) {
            case O_RDONLY:
                flag = PROT_READ;
                break;
            case O_WRONLY:
                flag = PROT_WRITE;
                break;
            case O_RDWR:
            default:
                flag = PROT_READ | PROT_WRITE;
                break;
        }
        out->mmap=(unsigned char *)mmap(out->mmap, length + offset - pa_offset, flag, MAP_SHARED, fd, pa_offset);
        close(fd);
        if(out->mmap==MAP_FAILED){
            return NULL;
        }
        out->data = &(out->mmap[offset]);
        out->length = length + offset - pa_offset;
    }
    else
    {
        printf("TODO!");
        return NULL;
    }
    return out;
}
#endif

/** Load bitmap from file */
Bitmap *loadBitmap(const char *fname)
{
    BMHeader hd;
    BMInfo nf;
    unsigned int bo;
    unsigned char pal[1024];
    int mode=0;
    int x,y,w,h;
    Bitmap *out;
    unsigned char *trg;
    FILE *fptr;

    if(fname==NULL)
    {
        printf("loadBitmap: NULL filename\n");
        return NULL;
    }

    fptr=fopen(fname,"rb");
    if(fptr==NULL)
    {
        printf("loadBitmap: Could not open '%s'\n",fname);
        return NULL;
    }

    fread(&hd,sizeof(hd),1,fptr);
    fread(&nf,sizeof(nf),1,fptr);

    if(hd.b!='B' || hd.m!='M')
    {
        printf("loadBitmap: Invalid file type in '%s'\n",fname);
        return NULL;
    }
    if(nf.cs0!=sizeof(nf))
    {
        printf("loadBitmap: Unknown file format in '%s'\n",fname);
        return NULL;
    }
    if(nf.bits0==8)
    {
        int cu;
        // Palettized format
        memset(pal,0,1024);
        cu=nf.cu0+(nf.cu1<<8);
        if(cu==0)
            cu=256;
        if(cu>256)
        {
            printf("loadBitmap: Too many palette colors in '%s'\n",fname);
            return NULL;
        }
        fread(pal,cu,4,fptr);
        mode=1;
        for(x=0;x<256;x++)
        {
            if(pal[x*4]!=pal[x*4+1] || pal[x*4]!=pal[x*4+2])
                mode=0;
        }
    }
    else if(nf.bits0!=24 && nf.bits0!=32)
    {
        printf("loadBitmap: Cannot handle %d bpp in '%s'\n",nf.bits0,fname);
        return NULL;
    }

    bo=hd.o0+(hd.o1<<8)+(hd.o2<<16)+(hd.o3<<24);
    fseek(fptr,bo,0);

    w=nf.w0+(nf.w1<<8)+(nf.w2<<16)+(nf.w3<<24);
    h=nf.h0+(nf.h1<<8)+(nf.h2<<16)+(nf.h3<<24);

    out=(Bitmap*)malloc(sizeof(Bitmap));
    if(out==NULL){
        return NULL;
    }
    out->data=(unsigned char *)calloc(sizeof(unsigned char),w*h*(mode==0?3:1));
    if(out->data==NULL){
        return NULL;
    }
    out->width=w;
    out->height=h;
    out->depth=mode==0?24:8;

    for(y=0;y<h;y++)
    {
        trg=&out->data[(h-1-y)*(out->depth/8)*w];
        for(x=0;x<w;x++)
        {
            if(nf.bits0==8)
            {
                int c=fgetc(fptr);
                if(mode==0)
                {
                    *trg++=pal[c*4+0];
                    *trg++=pal[c*4+1];
                    *trg++=pal[c*4+2];
                }
                else
                    *trg++=pal[c*4];
            }
            else if(nf.bits0==24)
            {
                int b=fgetc(fptr);
                int g=fgetc(fptr);
                int r=fgetc(fptr);
                *trg++=b;
                *trg++=g;
                *trg++=r;
            }
            else if(nf.bits0==32)
            {
                int b=fgetc(fptr);
                int g=fgetc(fptr);
                int r=fgetc(fptr);
                fgetc(fptr);
                *trg++=b;
                *trg++=g;
                *trg++=r;
            }
        }
        if((w*nf.bits0/8)&3)
        {
            int s=4-((w*nf.bits0/8)&3);
            for(x=0;x<s;x++)
                fgetc(fptr);
        }
    }
    fclose(fptr);
    return out;
}

/** Load bitmap header from file */
Bitmap *loadBitmapHeaderOnly(const char *fname)
{
    BMHeader hd;
    BMInfo nf;
    unsigned int bo;
    unsigned char pal[1024];
    int mode=0;
    int x,w,h;
    Bitmap *out;
    FILE *fptr;

    if(fname==NULL)
    {
        printf("loadBitmapHeaderOnly: NULL filename\n");
        return NULL;
    }

    fptr=fopen(fname,"rb");
    if(fptr==NULL)
    {
        printf("loadBitmapHeaderOnly: Could not open '%s'\n",fname);
        return NULL;
    }

    fread(&hd,sizeof(hd),1,fptr);
    fread(&nf,sizeof(nf),1,fptr);

    if(hd.b!='B' || hd.m!='M')
    {
        printf("loadBitmapHeaderOnly: Invalid file type in '%s'\n",fname);
        return NULL;
    }
    if(nf.cs0!=sizeof(nf))
    {
        printf("loadBitmapHeaderOnly: Unknown file format in '%s'\n",fname);
        return NULL;
    }
    if(nf.bits0==8)
    {
        int cu;
        // Palettized format
        memset(pal,0,1024);
        cu=nf.cu0+(nf.cu1<<8);
        if(cu==0)
            cu=256;
        if(cu>256)
        {
            printf("loadBitmapHeaderOnly: Too many palette colors in '%s'\n",fname);
            return NULL;
        }
        fread(pal,cu,4,fptr);
        mode=1;
        for(x=0;x<256;x++)
        {
            if(pal[x*4]!=pal[x*4+1] || pal[x*4]!=pal[x*4+2])
                mode=0;
        }
    }
    else if(nf.bits0!=24 && nf.bits0!=32)
    {
        printf("loadBitmapHeaderOnly: Cannot handle %d bpp in '%s'\n",nf.bits0,fname);
        return NULL;
    }

    bo=hd.o0+(hd.o1<<8)+(hd.o2<<16)+(hd.o3<<24);
    fseek(fptr,bo,0);

    w=nf.w0+(nf.w1<<8)+(nf.w2<<16)+(nf.w3<<24);
    h=nf.h0+(nf.h1<<8)+(nf.h2<<16)+(nf.h3<<24);

    out=(Bitmap*)malloc(sizeof(Bitmap));
    if(out == NULL) {
        printf("loadBitmapHeaderOnly: Cannot allocate bitmap\n");
        return NULL;
    }
    out->width=w;
    out->height=h;
    out->depth=mode==0?24:8;

    fclose(fptr);
    return out;
}

/** Save bitmap to file */
int saveBitmap(const char *fname, Bitmap *bmp)
{
    FILE *fptr;
    int y;
    int pitch=(bmp->width*bmp->depth/8+3)&(~3);
    int bs=bmp->height*pitch;
    int s=bs+sizeof(BMHeader)+sizeof(BMInfo)+(bmp->depth==8?1024:0);
    BMHeader head={
        'B', 'M',
        (unsigned char) (s&0xff),
        (unsigned char) ((s>>8)&0xff),
        (unsigned char) ((s>>16)&0xff),
        (unsigned char) ((s>>24)&0xff),
        0,0,0,0,
        sizeof(BMHeader)+sizeof(BMInfo),
        (unsigned char) ((bmp->depth == 8 ? 4 : 0)),
        0,0
    };
    BMInfo info;

    if((fptr=fopen(fname,"wb")) == NULL)
    {
        printf("saveBitmap: Could not open '%s'\n",fname);
        return 0;
    }

    memset(&info,0,sizeof(info));
    // Setup individual bytes for endian independence
    info.cs0=sizeof(info);
    info.w0=bmp->width&0xff;
    info.w1=(bmp->width>>8)&0xff;
    info.h0=bmp->height&0xff;
    info.h1=(bmp->height>>8)&0xff;
    info.bits0=bmp->depth;
    info.planes0=1;
    info.s0=bs&0xff;
    info.s1=(bs>>8)&0xff;
    info.s2=(bs>>16)&0xff;
    info.s3=(bs>>24)&0xff;
    info.xRes=72;
    info.yRes=72;
    if(bmp->depth==8)
    {
        info.cu1=1;     // 256
        info.ci1=1;     // 256
    }
    // Write file header
    fwrite(&head,sizeof(head),1,fptr);
    // Write info
    fwrite(&info,sizeof(info),1,fptr);
    // Set up monochrome ramp palette
    if(bmp->depth==8)
    {
        for(y=0;y<256;y++)
        {
            // Replicate i into all channels of color
            int c=y+(y<<8)+(y<<16);
            fwrite(&c,4,1,fptr);
        }
    }

    // Write bitplane
    for(y=0;y<bmp->height;y++)
    {
        fwrite(&bmp->data[(bmp->height-y-1)*bmp->width*bmp->depth/8],pitch,1,fptr);
    }
    fclose(fptr);

    return 1;
}

/** Create bitmap from scratch */
Bitmap *createBitmap(int w, int h, int d)
{
    Bitmap *out=(Bitmap*)malloc(sizeof(Bitmap));
    if(out==NULL){
        return NULL;
    }
    out->data=(unsigned char *)calloc(sizeof(unsigned char), w*h*d/8);
    if(out->data==NULL){
        return NULL;
    }
    out->length=0;
    out->width=w;
    out->height=h;
    out->depth=d;
    return out;
}

void destroyBitmap(Bitmap *bmp)
{
#ifndef WIN32
    if(bmp->length != 0){
        munmap(bmp->mmap, bmp->length);
    }else{
#endif
        free(bmp->data);
#ifndef WIN32
    }
#endif
    free(bmp);
}
