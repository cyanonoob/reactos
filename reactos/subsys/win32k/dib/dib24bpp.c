#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include <ddk/winddi.h>
#include "../eng/objects.h"
#include "dib.h"

VOID DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, RGBTRIPLE c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  *addr = c;
}

RGBTRIPLE DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;

  return *addr;
}

VOID DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, RGBTRIPLE c)
{
  PBYTE byteaddr = SurfObj->pvBits + y * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x1;
  LONG cx = x1;

  while(cx <= x2) {
    *addr = c;
    ++addr;
    ++cx;
  }
}

VOID DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, RGBTRIPLE c)
{
  PBYTE byteaddr = SurfObj->pvBits + y1 * SurfObj->lDelta;
  PRGBTRIPLE addr = (PRGBTRIPLE)byteaddr + x;
  ULONG  lDelta = SurfObj->lDelta;

  byteaddr = (PBYTE)addr;
  while(y1++ <= y2) {
    *addr = c;

    byteaddr += lDelta;
    addr = (PRGBTRIPLE)byteaddr;
  }
}

VOID DIB_24BPP_BltTo_24BPP(PSURFOBJ dstpsd, LONG dstx, LONG dsty, LONG w, LONG h,
  PSURFOBJ srcpsd, LONG srcx, LONG srcy)
{
  PRGBTRIPLE  dst;
  PRGBTRIPLE  src;
  PBYTE  bytedst;
  PBYTE  bytesrc;
  int  i;
  int  dlDelta = dstpsd->lDelta;
  int  slDelta = srcpsd->lDelta;

  bytedst = (char *)dstpsd->pvBits + dsty * dlDelta;
  bytesrc = (char *)srcpsd->pvBits + srcy * slDelta;
  dst = (PRGBTRIPLE)bytedst + dstx;
  src = (PRGBTRIPLE)bytesrc + srcx;

  while(--h >= 0) {
    PRGBTRIPLE  d = dst;
    PRGBTRIPLE  s = src;
    LONG  dx = dstx;
    LONG  sx = srcx;
    for(i=0; i<w; ++i) {
      *d = *s;
      ++d;
      ++s;
    }
    dst += dlDelta;
    src += slDelta;
  }
}

BOOLEAN DIB_To_24BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
        ULONG   Delta,     XLATEOBJ *ColorTranslation)
{
  ULONG    i, j, sx, xColor, f1;
  PBYTE    DestBits, SourceBits_24BPP, DestLine, SourceLine_24BPP;
  PRGBTRIPLE  SPDestBits, SPSourceBits_24BPP, SPDestLine, SPSourceLine_24BPP; // specially for 24-to-24 blit
  PBYTE    SourceBits_4BPP, SourceBits_8BPP, SourceLine_4BPP, SourceLine_8BPP;
  PWORD    SourceBits_16BPP, SourceLine_16BPP;
  PDWORD    SourceBits_32BPP, SourceLine_32BPP;

  DestBits = DestSurf->pvBits + (DestRect->top * DestSurf->lDelta) + DestRect->left * 3;

  switch(SourceGDI->BitsPerPixel)
  {
    case 4:
      SourceBits_4BPP = SourceSurf->pvBits + (SourcePoint->y * SourceSurf->lDelta) + SourcePoint->x;

      for (j=DestRect->top; j<DestRect->bottom; j++)
      {
        SourceLine_4BPP = SourceBits_4BPP;
        DestLine = DestBits;
        sx = SourcePoint->x;
        f1 = sx & 1;

        for (i=DestRect->left; i<DestRect->right; i++)
        {
          xColor = XLATEOBJ_iXlate(ColorTranslation,
              (*SourceLine_4BPP & altnotmask[sx&1]) >> (4 * (1-(sx & 1))));
          *DestLine++ = xColor & 0xff;
          *(PWORD)DestLine = xColor >> 8;
          DestLine += 2;
          if(f1 == 1) { SourceLine_4BPP++; f1 = 0; } else { f1 = 1; }
          sx++;
        }

        SourceBits_4BPP += SourceSurf->lDelta;
        DestBits += DestSurf->lDelta;
      }
      break;

    default:
      DbgPrint("DIB_24BPP_Bitblt: Unhandled Source BPP: %u\n", SourceGDI->BitsPerPixel);
      return FALSE;
  }

  return TRUE;
}
