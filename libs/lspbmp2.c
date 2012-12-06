#if _MSC_VER >= 1400 // If we are using VS 2005 or greater
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lspbmp.h"

/** Load bitmap from file */
Bitmap *loadBitmap(const char *fname)
{
    printf("loadBitmapHeader\n");
    Bitmap *out = loadBitmapHeader(fname);
    printf("loadBitmapContent\n");
    if (loadBitmapContent(out, fname)) {
        printf("loadBitmapContent done\n");
        return out;
    } else {
        printf("loadBitmapContent failed\n");
        destroyBitmap(out);
        return NULL;
    }
}

void destroyBitmap(Bitmap *bmp)
{
    if (bmp->pal != NULL) {
        free(bmp->pal);
    }
    if (bmp->data != NULL) {
        free(bmp->data);
    }
    free(bmp);
}

Bitmap *loadBitmapHeader(const char *fname)
{
    unsigned int bo;
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

    out=(Bitmap*)malloc(sizeof(Bitmap));

    if(out==NULL)
    {
        printf("cannot allocate memory for Bitmap");
        return NULL;
    }

    BMHeader *hd = &(out->hd);
    BMInfo *nf = &(out->nf);

    fread(hd,sizeof(BMHeader),1,fptr);
    fread(nf,sizeof(BMInfo),1,fptr);

    printf("HD.b: %c\n", hd->b);

    if(hd->b!='B' || hd->m!='M')
    {
        printf("loadBitmapHeader: Invalid file type in '%s'\n",fname);
        return NULL;
    }
    if(nf->cs0!=sizeof(BMInfo))
    {
        printf("loadBitmapHeader: Unknown file format in '%s'\n",fname);
        return NULL;
    }
    if(nf->bits0==8)
    {
        out->pal = (unsigned char *) calloc(sizeof(unsigned char), 1024);
        unsigned char *pal = out->pal;
        int cu;
        // Palettized format
        cu=nf->cu0+(nf->cu1<<8);
        if(cu==0)
            cu=256;
        if(cu>256)
        {
            printf("loadBitmapHeader: Too many palette colors in '%s'\n",fname);
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
    else if(nf->bits0!=24 && nf->bits0!=32)
    {
        printf("loadBitmap: Cannot handle %d bpp in '%s'\n",nf->bits0,fname);
        return NULL;
    }

    bo=hd->o0+(hd->o1<<8)+(hd->o2<<16)+(hd->o3<<24);

    w=nf->w0+(nf->w1<<8)+(nf->w2<<16)+(nf->w3<<24);
    h=nf->h0+(nf->h1<<8)+(nf->h2<<16)+(nf->h3<<24);

    out->width=w;
    out->height=h;
    out->depth=mode==0?24:8;
    fclose(fptr);
    return out;
}

int loadBitmapContent(Bitmap *out, const char* fname)
{
    int mode=0;
    int x,y,w,h;
    FILE *fptr;
    unsigned char *pal = out->pal;
    unsigned char *trg;

    BMHeader *hd = &(out->hd);
    BMInfo *nf = &(out->nf);

    printf("HD.b: %c\n", hd->b);
    printf("nfbits %c\n", nf->bits0);

    if(fname==NULL)
    {
        printf("loadBitmapContent: NULL filename\n");
        return 0;
    }

    fptr=fopen(fname,"rb");
    if(fptr==NULL)
    {
        printf("loadBitmapContent: Could not open '%s'\n",fname);
        return 0;
    }

    w = out->width;
    h = out->height;
    mode = out->depth==24?0:1;
    out->data = (unsigned char *) calloc(sizeof(unsigned char), w*h*(mode==0?3:1));
    if(out->data==NULL) {
        printf("loadBitmapContent: Cannot allocate data\n");
        return 0;
    }
    printf("Reading %d @%d\n", w*h*(mode==0?3:1), out->data);
    for(y=0;y<h;y++)
    {
        trg=&out->data[(h-1-y)*(out->depth/8)*w];
        for(x=0;x<w;x++)
        {
            if(nf->bits0==8)
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
            else if(nf->bits0==24)
            {
                int b=fgetc(fptr);
                int g=fgetc(fptr);
                int r=fgetc(fptr);
                *trg++=b;
                *trg++=g;
                *trg++=r;
            }
            else if(nf->bits0==32)
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
        if((w*nf->bits0/8)&3)
        {
            int s=4-((w*nf->bits0/8)&3);
            for(x=0;x<s;x++)
                fgetc(fptr);
        }
    }
    fclose(fptr);
    return 1;
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
    if (out==NULL) {
        return NULL;
    }
    out->data=(unsigned char *)calloc(sizeof(unsigned char *), w*h*d/8);
    if (out->data==NULL) {
        return NULL;
    }
    out->width=w;
    out->height=h;
    out->depth=d;
    return out;
}
