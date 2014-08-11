//Implementation of Rabin Hashing for finding chunk boundaries with SHA1 hashing

#include <openssl/sha.h>

#define MIN_RABIN_CHUNK_SIZE (64)
#define MAX_RABIN_CHUNK_SIZE (128*1024)
#define IRREDUCIBLE_POLY (0x946d1d8dcfee41e5LL)
static int chunk_sizes[] = {32, 64, 128, 256, 512, 1024, 2*1024, 4*1024, 
8*1024, 16*1024, 32*1024, 64*1024, 128*1024};
static u_int masks[] = {32 - 1 , 64 - 1, 128 - 1, 256 - 1, 512 - 1, 1024 - 1,
	2*1024 - 1, 4*1024 - 1, 8*1024 - 1, 16*1024 - 1, 32*1024 - 1,
	64*1024 - 1, 128*1024 - 1};


	
unsigned char md[SHA256_DIGEST_LENGTH]; // 32 bytes

bool simpleSHA256(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if(!SHA256_Init(&context))
        return false;

    if(!SHA256_Update(&context, (unsigned char*)input, length))
        return false;

    if(!SHA256_Final(md, &context))
        return false;

    return true;
}

/* rabinMRC structure, contains number of levels of resolution chunks plus an array of
arrays of SHA1 hashes, one array per number of levels */
struct rabinMRC
{
	int numLevels;
	unsigned char** hashesArray;
}


/* This rabin fingerprinting is done at the resolution of characters, so 1 byte*/
rabinMRC* rabinFingerprint(int numLevels, int maxChunkSize, int minChunkSize, char* content)
{

}