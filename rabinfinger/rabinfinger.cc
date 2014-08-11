/*
 * $RCSfile: rabinfinger2.c,v $
 * $Revision: 1.10 $
 * $Date: 2009/05/18 01:37:09 $
 *
 * Copyright (c) 2006 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE.TXT
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA,
 * 94704.  Attention:  Intel License Inquiry.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "rabinfinger.h"
#include "fixedCtx.h"
#include <string.h>
// for node binding
#define BUILDING_NODE_EXTENSION
#include <node.h>

/*--------------------------------------------------------------------------*/
#define INT64(x) ((unsigned long long)(x))
#define MSB64 INT64(0x8000000000000000ULL)
#define MIN_RABIN_CHUNK_SIZE (64)
#define MAX_RABIN_CHUNK_SIZE (128*1024)

#define IRREDUCIBLE_POLY (0x946d1d8dcfee41e5LL)
#define NELEM(a) (sizeof(a)/sizeof(a[0]))
/*--------------------------------------------------------------------------*/
static int chunk_sizes[] = {32, 64, 128, 256, 512, 1024, 2*1024, 4*1024, 
  8*1024, 16*1024, 32*1024, 64*1024, 128*1024};
static u_int masks[] = {32 - 1 , 64 - 1, 128 - 1, 256 - 1, 512 - 1, 1024 - 1,
  2*1024 - 1, 4*1024 - 1, 8*1024 - 1, 16*1024 - 1, 32*1024 - 1,
  64*1024 - 1, 128*1024 - 1};
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/



static int 
GetDegree(u_int64 poly)
{
  int k;

  for (k = 63; (poly & (INT64(1) << k)) == 0 && k > 0; k--) ;

  return k;
}
/*--------------------------------------------------------------------------*/
u_int64
GetFingerprint(u_int64 h, u_int64 l, u_int64 poly)
{
  int i;
  int k = GetDegree(poly);

  poly <<= (63 - k);
  if (h) {
    if (h & MSB64)
      h ^= poly;
    for (i = 62; i >= 0; i--)
      if (h & INT64 (1) << i) {
  h ^= (poly >> (63 - i));
  l ^= (poly << (i + 1));
      }
  }
  for (i = 63; i >= k; i--)
    if (l & INT64 (1) << i)
      l ^= (poly >> (63 - i));

  return l;
}
/*--------------------------------------------------------------------------*/
void
MultiplyPoly(u_int64* h, u_int64* l, u_int64 x, u_int64 y)
{
  u_int64 ih = 0, il = 0;
  int i;

  if (h == NULL || l == NULL)
  {
    fprintf(stderr, "wrong multiply argument\n");
    return;
  }


  if (x & 1)
    il = y;

  for (i = 63; i > 0; i--) {
    if (x & (INT64(1) << i)) {
      ih ^= (y >> (64 - i));
      il ^= (y << i);
    }
  }
  *h = ih;
  *l = il;
}
/*--------------------------------------------------------------------------*/
static u_int64 
ConcatByte(RABINPOLY* rp, u_int64 finger, u_int8 b)
{
  finger ^= rp->regionT[rp->region_buf[rp->region_pos]];
  rp->region_buf[rp->region_pos] = b;
  rp->region_pos = (rp->region_pos + 1) % rp->region_size;

  return (finger << 8 | b) ^ rp->moduloT[finger >> rp->shift];
}
/*--------------------------------------------------------------------------*/


static void 
InitializeRabin(RABINPOLY* rp, int region_size)
{
  int i, k;
  u_int64 T, h, l;
  u_int64 sizeshift = 1;

  if (rp->ir_poly) {
    return;
  }

  rp->ir_poly     = IRREDUCIBLE_POLY;
  rp->region_size = region_size;
  rp->region_pos  = 0;

  k = GetDegree(rp->ir_poly);
  rp->shift = k - 8;

  /* calculate the modulo table */
  T = GetFingerprint(0, INT64(1) << 63, rp->ir_poly);
  for (i = 0; i < 256; i++) {
    MultiplyPoly(&h, &l, i, T);
    rp->moduloT[i] = GetFingerprint(h, l, rp->ir_poly) | (INT64(i) << k);
  }
  /* calculate the region table */
  for (i = 1; i < rp->region_size; i++) 
    sizeshift = (sizeshift << 8) ^ rp->moduloT[sizeshift >> rp->shift];
  for (i = 0; i < 256; i++) {
    MultiplyPoly(&h, &l, i, sizeshift);
    rp->regionT[i] = GetFingerprint(h, l, rp->ir_poly);
  }
}


/*--------------------------------------------------------------------------*/
static void 
ZeroRegionBuf(RABINPOLY *rp)
{
  memset(rp->region_buf, 0, sizeof(rp->region_buf));
}
/*--------------------------------------------------------------------------*/
static int 
GetNormalizedSize(int b_size)
{
  int i;
  for (i = 0; i < (int)NELEM(chunk_sizes) - 1; i++) {
    if (b_size >= chunk_sizes[i] && b_size < chunk_sizes[i+1])
      return chunk_sizes[i];
  }

  fprintf(stderr, "block size %d is out of range(%d to %d)\n",  b_size,
      chunk_sizes[0], chunk_sizes[(int)NELEM(chunk_sizes) - 1]);
  return 0;
}
/*--------------------------------------------------------------------------*/
static int
GetMRCIndex(int b_size)
{
  int i;
  for (i = 0; i < (int)NELEM(chunk_sizes) - 1; i++) {
    if (b_size >= chunk_sizes[i] && b_size < chunk_sizes[i+1])
      return i;
  }

  fprintf(stderr, "block size %d is out of range(%d to %d)\n",  b_size,
      chunk_sizes[0], chunk_sizes[(int)NELEM(chunk_sizes) - 1]);
  return -1;
}
/*--------------------------------------------------------------------------*/
u_int GetMRCMask(int b_size)
{
  return masks[GetMRCIndex(b_size)];
}
/*--------------------------------------------------------------------------*/
#if 0
int32 
GetRabinChunkSize(const u_char* src, int32 srclen, int32 b_size)
{
#if 0
  /* pre-computed polynomials */
    0x946d1d8dcfee41e5LL
    0xbffd0d2d1803866bLL
    0xa77031279e124d9dLL
    0xd4eaaf59cb8c3605LL
    0xdf5b417b815f94f5LL
    0x8750d49cca554e29LL
    0xac021e7b5ce814e1LL
    0xd6864a7794875001LL
    0xd1f83f66a41cfb3dLL
    0x9d24c10e18a55d97LL
    0xc19b3824ffca799bLL
#endif

  const u_char* end, *p;
  static int avg_bsize = 0;
  static u_int mask = 0;
  u_int64 finger = 0;
  static RABINPOLY rp = {{0}};
  
  if (avg_bsize == 0) {
    avg_bsize = GetNormalizedSize(b_size);
    mask = avg_bsize - 1;
    /* use the default windows size of 48 bytes */
    InitializeRabin(&rp, 48);
  }
  
  if (srclen <= MIN_RABIN_CHUNK_SIZE)
    return srclen;

  ZeroRegionBuf(&rp);
  srclen = MIN(srclen, MAX_RABIN_CHUNK_SIZE);
  end = src + srclen;
  for (p = src + MIN_RABIN_CHUNK_SIZE; p < end; p++) {
    finger = ConcatByte(&rp, finger, *p);
    if ((finger & mask) == mask) 
      return (p - src + 1);
  }
  return srclen;
}
#endif
/*--------------------------------------------------------------------------*/
void InitRabinCtx(RabinCtx* rctx, int32 bsize, int window, int min, int max)
{
  rctx->bsize = GetNormalizedSize(bsize);
  rctx->mask = rctx->bsize - 1;
  rctx->finger = 0;
  rctx->rp.ir_poly = 0;
  rctx->min_size = min;
  rctx->max_size = max;

  /* this should be done very quickly? */
  InitializeRabin(&(rctx->rp), window);

  #if 0
  /* initialize rabin only once */
  static RABINPOLY rp;
  static char init = FALSE;
  if (!init) {
    InitializeRabin(&rp, window);
    init = TRUE;
  }

  /* just copy the initialized one */
  memcpy(&(rctx->rp), &rp, sizeof(RABINPOLY));
  #endif
}
/*--------------------------------------------------------------------------*/
int32 GetRabinChunkSizeEx(RabinCtx* rctx, const u_char* src, int32 srclen,
    u_int min_mask, u_int max_mask, char resetRegionBuffer,
    RabinBoundary* outBoundaries, unsigned int* outNumBoundaries)
{
  const u_char* end, *p;
  assert(rctx != NULL);
  if (resetRegionBuffer) {
    rctx->finger = 0;
    ZeroRegionBuf(&(rctx->rp));
  }

  srclen = srclen < rctx->max_size ? srclen : rctx->max_size;
  end = src + srclen;
  *outNumBoundaries = 0;
  
  for (p = src + rctx->min_size; p < end; p++) {
    rctx->finger = ConcatByte(&(rctx->rp), rctx->finger, *p);
    if ((rctx->finger & min_mask) == min_mask) {
      /* min chunk boundary detected */
      outBoundaries[*outNumBoundaries].offset = (p - src + 1);
      outBoundaries[*outNumBoundaries].finger = rctx->finger;
      (*outNumBoundaries)++;
    }

    if (max_mask != 0 && ((rctx->finger & max_mask) == max_mask)) {
      /* max chunk boundary detected, stop here */
      assert((rctx->finger & min_mask) == min_mask);
      assert((p - src + 1) <= srclen);
      assert(outBoundaries[*outNumBoundaries - 1].offset
          == (p - src + 1));
      break;
    }
  }

  if (*outNumBoundaries == 0) {
    /* we should have at least one */
    outBoundaries[*outNumBoundaries].offset = srclen;
    outBoundaries[*outNumBoundaries].finger = rctx->finger;
    (*outNumBoundaries)++;
  }

  /* return the last chunk */
  return outBoundaries[*outNumBoundaries - 1].offset;
}
/*--------------------------------------------------------------------------*/

/*
using namespace v8;
/*
Handle<Value> GetRabinChunkSize(const Arguments& args)
{
  HandleScope scope;
   if (args.Length() != 8) {
      ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
          return scope.Close(Undefined());
            }

  if (!args[0]->IsString()) {
        ThrowException(Exception::TypeError(String::New("Wrong arguments")));
            return scope.Close(Undefined());
              }

    v8::String::AsciiValue inStr(args[0]);
    if( !(char* content = (char*) malloc(inStr.length() + 1)) ){
      ThrowException(v8::String::New("Could not allocate string buffer"));
      return scope.Close(Undefined());
    }

    strcpy(content, *inStr);

    return scope.Close(String::Concat(String::New("Hello there, "), str));

}


Handle<Value> InitRabinCtx(const Arguments& args)
{
  if (args.Length() != 4) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
  }

  for(int i =0; i < args.Length(); i++)
  {
    if(!args[i]->isNumber())
    {
      ThrowException(Exception::TypeError(String::New("Wrong arguments")));
      return scope.Close(Undefined());
    }
  }

  int32 bsize = args[0]->Int32Value();
  int window = args[1]->IntegerValue();
  int min = args[2]->IntegerValue();
  int max = args[3]->IntegerValue();

  if( !(RabinCtx* context = (RabinCtx*) malloc(sizeof(RabinCtx))) )
  {
    ThrowException(Exception::TypeError(String::New("Could not allocate memory for context")));
    return scope.Close(Undefined());
  } 

  // Initialize the Rabin C struct
  InitRabinCtx(context, bsize, window, min, max);

  // Make it a JS object
  Handle<Object> obj =  Object::New();
  


}
*/


/* Dumps a rabin context to a file (as a string). Unfortunately
there is no good way to save binary blobs in v8 so this is the
best we gon' get.  Returns 0 if failure, 1 otherwise */

int dumpRabinCtx(RabinCtx* ctx, char* outFileName)
{
  FILE* fp;
  if( !(fp = fopen(outFileName, "w")) )
  {
    printf("Can't open file %s\n", outFileName);
    return 0;
  }

  assert(ctx);
  fprintf(fp, "RABINPOLY fixedPoly;\n");
  fprintf(fp, "fixedPoly.moduloT = \n");
  fprintf(fp, "{");
  for(int i =0; i < 255; i++)
  {
    fprintf(fp, "%lu, ", ctx->rp.moduloT[i]);
  }
  fprintf(fp, "%lu};\n\n", ctx->rp.moduloT[255]);
  fprintf(fp, "fixedPoly.regionT = \n");
  fprintf(fp, "{");
  for(int i =0; i < 255; i++)
  {
    fprintf(fp, "%lu, ", ctx->rp.regionT[i]);
  }
  fprintf(fp, "%lu};\n\n", ctx->rp.regionT[255]);
  fprintf(fp, "fixedPoly.region_size = %d;\n", ctx->rp.region_size);
  fprintf(fp, "fixedPoly.region_post %d;\n", ctx->rp.region_pos);
  fprintf(fp, "fixedPoly.shift = %d;\n", ctx->rp.shift);
  fprintf(fp, "fixedPoly.region_size =%lu;\n\n", ctx->rp.ir_poly);

  fprintf(fp, "fixedCtx.rp = fixedPoly;\n");
  fprintf(fp, "fixedCtx.mask = %d;\n", ctx->mask);
  fprintf(fp, "fixedCtx.finger = %lu;\n", ctx->finger);
  fprintf(fp, "fixedCtx.bsize = %d;\n", ctx->bsize);
  fprintf(fp, "fixedCtx.min_size = %d;\n", ctx->min_size);
  fprintf(fp, "fixedCtx.max_size = %d;\n\n", ctx->max_size);


  fclose(fp);
  return 1;

}

using namespace v8;

Handle<Value> n_getMRCMask(const Arguments& args)
{
    HandleScope scope;
    if (args.Length() != 1) {
    ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
    return scope.Close(Undefined());
    }

  if (!args[0]->IsNumber()) {
    ThrowException(Exception::TypeError(String::New("Wrong arguments")));
    return scope.Close(Undefined());
  }

  int32 b_size = args[0]->Int32Value();
  return scope.Close(Number::New(GetMRCMask(b_size)));

}

Handle<Value> n_getRabinChunkSize(const Arguments& args)
{
  HandleScope scope;

  if(args.Length() != 4){
      ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
      return scope.Close(Undefined());
  }
  if (!args[0]->IsString() && !args[1]->IsNumber()  && !args[2]->IsNumber() && !args[3]->IsNumber() ) {
    ThrowException(Exception::TypeError(String::New("Wrong arguments")));
    return scope.Close(Undefined());
  }

  v8::String::AsciiValue str(args[0]);
  char* src = (char*) malloc(str.length() + 1);
  strcpy(src, *str);
  int32 srclen = args[1]->Int32Value();
  u_int min_mask = args[2]->IntegerValue();
  u_int max_mask = args[3]->IntegerValue();


  int g_ChunkSize = 1024; 
  RabinBoundary* boundaries = (RabinBoundary*) malloc(sizeof(RabinBoundary) * g_ChunkSize);
  if (boundaries == NULL) {
    ThrowException(Exception::TypeError(String::New("Couldn't allocate memory for boundaries")));
    return scope.Close(Undefined());
  }
  u_int numBoundaries;

  int32 result = GetRabinChunkSizeEx(&fixedCtx, (const u_char*) src, srclen, min_mask, max_mask, 
    1, boundaries, &numBoundaries);

  Handle<Array> results = Array::New(2*numBoundaries);

  if(results.IsEmpty())
  {
    ThrowException(Exception::TypeError(String::New("Couldn't allocate memory for boundary results")));
    return scope.Close(Undefined());
  }

  for(int j = 0; j < numBoundaries; j++)
  {
    results->Set(2*j, Number::New(boundaries[j].finger));
    results->Set(2*j+1, Number::New(boundaries[j].offset));
  }

  free(boundaries);
  free(src);
  return scope.Close(results);

}

void Init(Handle<Object> exports){
  exports->Set(String::NewSymbol("getMRCMask"),
        FunctionTemplate::New(n_getMRCMask)->GetFunction());
  exports->Set(String::NewSymbol("getRabinChunkSize"),
        FunctionTemplate::New(n_getRabinChunkSize)->GetFunction());
}

NODE_MODULE(rabinfinger, Init)
/*
int main(int argc, char** argv)
{
  
  #define WAPROX_MIN_CHUNK_SIZE   (30)
  #define WAPROX_RABIN_WINDOW     (16)
  #define WAPROX_MRC_MIN          (0)
  #define WAPROX_MRC_MAX          (128*1024)

  //RabinCtx* context =(RabinCtx*) malloc(sizeof(RabinCtx));
  
  int32 chunkSize;
  int g_MinChunkSize = 64;
  int g_ChunkSize = 1024; 
  
  FILE *fp;
  char* src;
  long lSize;
  int i;
  char* buffer;
  if(argc < 2)
  {
    printf("Usage: %s inputfile\n", argv[0]);
    exit(1);
  }
  if( !(fp = fopen(argv[1], "r")) )
  {
    printf("Can't open file %s\n", argv[1]);
    exit(1);
  }

  fseek(fp, 0L, SEEK_END);
  lSize = ftell(fp);
  rewind(fp);

  if(! (src = calloc(1, lSize + 1)) ){
    fclose(fp);
    printf("Memory allocation failed");
    exit(1);
  };

  if(1 != fread(src, lSize, 1, fp))
  {
    fclose(fp);
    free(src);
    printf("Read failed");
    exit(1);  
  }
  fclose(fp);

  static RabinBoundary* rabinBoundaries = NULL;
  unsigned int numBoundaries = 0;

  rabinBoundaries = (RabinBoundary*) malloc(sizeof(RabinBoundary) * g_ChunkSize);
  if (rabinBoundaries == NULL) {
          fprintf(stderr, "RabinBoundary alloc failed\n");
          return 1;
  }

  u_int min_mask = GetMRCMask(g_MinChunkSize);
  u_int max_mask = GetMRCMask(128*1024-1);
  

  //InitRabinCtx(fixedCtx, g_MinChunkSize, WAPROX_RABIN_WINDOW,
           //     WAPROX_MRC_MIN, WAPROX_MRC_MAX);

  //dumpRabinCtx(context, "outfile.txt");

    chunkSize = GetRabinChunkSizeEx(&fixedCtx, (const u_char*) src, strlen(src), min_mask, max_mask, 1, rabinBoundaries, &numBoundaries);
    //printf("Rabin Fingerprinted %d/%d bytes, finger: %lu, %d\n", chunkSize, (int) strlen(src), rabinBoundaries[numBoundaries-1].finger, numBoundaries);
    for(int i=0; i<numBoundaries; i++)
    {
      if(i == 0)
      {
        buffer = malloc(rabinBoundaries[0].offset + 2);
        for(int j = 0; j <= rabinBoundaries[0].offset; j++)
        {
          buffer[j] = src[j];
        }
        buffer[rabinBoundaries[0].offset + 1] = '\0';
      }
       else    
      {
        if(! (buffer = malloc(rabinBoundaries[i].offset - rabinBoundaries[i-1].offset+ 2)))  exit(1);
        for(int j = 0; j <= (rabinBoundaries[i].offset - rabinBoundaries[i-1].offset); j++)
        {
          buffer[j] = src[rabinBoundaries[i-1].offset + 1 + j];
        }
        buffer[rabinBoundaries[i].offset - rabinBoundaries[i-1].offset + 1] = '\0';
      }
      fprintf(stdout, "start:%d,&%lu,&%d,&%s\n\n", i, rabinBoundaries[i].finger, rabinBoundaries[i].offset, buffer);
      //fprintf(stdout, "%d'th Fingerprint finger: %lu Fingerprint offset: %d\n Content: %s\n\n", i, rabinBoundaries[i].finger, rabinBoundaries[i].offset, buffer);
      free(buffer);
    }

  
  free(src);
  free(rabinBoundaries);

  return 0;
}
*/
