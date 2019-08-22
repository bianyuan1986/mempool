#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mempool.h"

#define ALIGN_ROUND_UP(x, align) (((x)+(align-1))&(~(align-1)))
#define RTE_CACHE_LINE_SIZE 64
#define MAGIC_END 0xCCCCCCCC

typedef void* (*getObjectFunc)(struct Mempool *mp);
typedef int (*putObjectFunc)(struct Mempool *mp, void *obj);
typedef int (*createMemblockFunc)(struct Mempool *mp, unsigned int objSize, unsigned int elementCount);
typedef void (*mempoolFreeFunc)(struct Mempool *mp);

struct MempoolOps
{
	mallocFunc mallocPtr;
	freeFunc freePtr;
	getObjectFunc getObject;
	putObjectFunc putObject;
	createMemblockFunc createMemblock;
	mempoolFreeFunc mempoolFree;
};

struct Mempool
{
	struct BlockHeader *block;
	unsigned int elementSize;
	unsigned int blockCount;
	unsigned int maxElementCount;

	unsigned int initElementCount;
	unsigned int expandElementCount;
	unsigned int currentElementCount;
	unsigned int totalSize;
	unsigned int headerSize;
	unsigned int trailerSize;
	unsigned int objectSize;

	struct MempoolOps ops;
};

struct BlockHeader
{
	struct BlockHeader *next;
	unsigned int firstFree;
	unsigned int free;

	unsigned int elementCount;
	unsigned int dataSize;
	unsigned long long addrBoundry;
	unsigned char data[0];
};

static int round_up(int size)
{
	return size;
}

static void mempool_free_internal(struct Mempool *mp)
{
	struct BlockHeader *block = NULL;

	block = mp->block;
	while( block )
	{
		block = block->next;
	}
}

static void *mempool_get_object_internal(struct Mempool *mp)
{
	void *obj = NULL;
	struct BlockHeader *block = NULL;
	struct BlockHeader *prev = NULL;
	int ret = 0;

	block = mp->block;
	while( block && (block->free <= 0) )
	{
		prev = block;
		block = block->next;
	}

	if( !block )
	{
		ret = mp->ops.createMemblock(mp, mp->objectSize, mp->expandElementCount);
		if( ret < 0 )
		{
			return NULL;
		}
		block = mp->block;
	}

	if( block->free <= 0 || (block->firstFree >= block->elementCount) )
	{
		printf("Exception occured!\n");
		return NULL;
	}
	obj = block->data + block->firstFree * mp->objectSize;
	block->firstFree = *(unsigned int*)obj;
	block->free--;
	if( block->free > 0 && (block != mp->block) )
	{
		prev->next = block->next;
		block->next = mp->block;
		mp->block = block;
	}

	return (char*)obj+mp->headerSize;
}

static int mempool_put_object_internal(struct Mempool *mp, void *obj)
{
	struct BlockHeader *block = NULL;
	int idx = 0;

	obj = (unsigned char*)obj-mp->headerSize;
	block = mp->block;
	while( block )
	{
		if( ((unsigned char*)obj >= block->data) && ((unsigned long long)obj < block->addrBoundry) )
		{
			break;
		}
		block = block->next;
	}

	if( !block || (((unsigned char*)obj - block->data) % mp->objectSize != 0) )
	{
		printf("Put object back to mempool failed! Exception occured!\n");
		return -1;
	}
	*(int*)obj = block->firstFree;
	idx = ((unsigned char*)obj - block->data)/mp->objectSize;
	block->firstFree = idx;
	block->free++;

	return 0;
}

static int mempool_create_memblock(struct Mempool *mp, unsigned int objSize, unsigned int elementCount)
{
	int totalSize = 0;
	struct BlockHeader *block = NULL;
	struct BlockHeader *tmp = NULL;
	void *addr = NULL;
	unsigned int i = 0;

	if( mp->currentElementCount + elementCount > mp->maxElementCount )
	{
		printf("Can't create memblock. Exceed the maximum element count:%d\n", mp->maxElementCount);
		return -1;
	}
	totalSize = objSize * elementCount + sizeof(struct BlockHeader);
	addr = mp->ops.mallocPtr(totalSize);
	if( !addr )
	{
		printf("Malloc memory block failed!\n");
		return -1;
	}
	block = (struct BlockHeader*)addr;
	block->free = mp->initElementCount;
	block->firstFree = 0;
	block->elementCount = elementCount;
	block->dataSize = elementCount*objSize;
	block->addrBoundry = (unsigned long long)((char*)block->data + objSize * block->elementCount);
	for( ; i < block->free; i++)
	{
		addr = block->data+i*mp->objectSize;
		*(int*)addr = i+1;
	}
	*(int*)addr = MAGIC_END;

	mp->totalSize += totalSize;
	mp->currentElementCount += elementCount;
	tmp = mp->block;
	mp->block = block;
	block->next = tmp;
	mp->blockCount++;

	return 0;
}

static int mempool_init(struct Mempool *mp)
{
	int ret = 0;

	mp->elementSize = ALIGN_ROUND_UP(mp->elementSize, 8);
	mp->objectSize = mp->headerSize+mp->elementSize+mp->trailerSize;
	ret = mp->ops.createMemblock(mp, mp->objectSize, mp->initElementCount);
	if( ret < 0 )
	{
		return -1;
	}

	return 0;
}

struct Mempool *mempool_create(unsigned int elementSize, unsigned int maxElementCount, mallocFunc mallocPtr, freeFunc freePtr)
{
	struct Mempool *mp = NULL;
	int ret = 0;

	mp = (struct Mempool*)malloc(sizeof(struct Mempool));
	if( !mp )
	{
		goto FAILED;
	}
	memset(mp, 0x00, sizeof(struct Mempool));

	if( mallocPtr )
	{
		mp->ops.mallocPtr = mallocPtr;
	}
	else
	{
		mp->ops.mallocPtr = malloc;
	}

	if( freePtr )
	{
		mp->ops.freePtr = freePtr;
	}
	else
	{
		mp->ops.freePtr = free;
	}

	maxElementCount = round_up(maxElementCount);

	mp->maxElementCount = maxElementCount;
	mp->elementSize = elementSize;
#ifdef MEMPOOL_DEBUG
	mp->headerSize = ALIGN_ROUND_UP(mp->headerSize, 8);
	mp->trailerSize = ALIGN_ROUND_UP(mp->trailerSize, 8);
#endif
	mp->initElementCount = maxElementCount;
	mp->expandElementCount = 0;
	mp->ops.getObject = mempool_get_object_internal;
	mp->ops.putObject = mempool_put_object_internal;
	mp->ops.createMemblock = mempool_create_memblock;
	mp->ops.mempoolFree = mempool_free_internal;
	ret = mempool_init(mp);
	if( ret < 0 )
	{
		goto FAILED;
	}

	return mp;

FAILED:
	if( mp )
	{
		mempool_free(mp);
	}
	return NULL;
}

void *mempool_get_object(struct Mempool *mp)
{
	return mp->ops.getObject(mp);
}

int mempool_put_object(struct Mempool *mp, void *obj)
{
	return mp->ops.putObject(mp, obj);
}

void mempool_free(__attribute__((unused))struct Mempool *mp)
{
	return mp->ops.mempoolFree(mp);
}

void mempool_release_unused(struct Mempool *mp)
{
	struct BlockHeader *block = NULL;
	struct BlockHeader *prev = NULL;

	block = mp->block;
	prev = mp->block;

	while( block )
	{
		if( block->free*mp->objectSize == block->dataSize )
		{
			if(prev == block)
			{
				mp->block = block->next;
			}
			else
			{
				prev->next = block->next;
			}
			mp->ops.freePtr((void*)block);
		}
		else
		{
			prev = block;
		}
		block = block->next;
	}
}

