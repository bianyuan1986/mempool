/*
 *  Author: baohuren
 *  Email: baohuren@tencent.com
 *  Data: 2019-8-21
 *  This file implement a simple memory pool algorithm to manage a bulk of small object with same size.
 *  The malloc operation is fast and the free operation is a bit of slow. Compared to DPDK mempool and system
 *  malloc, the malloc operation of this mempool is faster than DPDK and system malloc. Allocate object and
 *  free object 1 million times individually, the statistics is as following:
 *  ---------------------------------------------------------
 *  DPDK Mempool Get Object Consume 9731us
 *  DPDK Mempool Put Object Consume 7758us
 *  ---------------------------------------------------------
 *  Get Object Consume 7284us
 *  Put Object Consume 26482us
 *  ---------------------------------------------------------
 *  Malloc Object Consume 54707us
 *  Free Object Consume 23737us
 *  ---------------------------------------------------------
 */

#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_

struct Mempool;

typedef void* (*mallocFunc)(size_t size);
typedef void (*freeFunc)(void *ptr);

/*
 * @Create mempool and init
 *
 * @param
 *  elementSize: the size of object stored in the mempool
 *  maxElementCount: the capacity of the mempool
 *  mallocPtr: custom memory malloc function, if null, the default value is malloc. It means the mempool will use heap memory.
 *  freePtr: custom memory release function, if null, the default value is free.
 *
 * @return
 *  the Mempool create by this function 
 */
struct Mempool *mempool_create(unsigned int elementSize, unsigned int maxElementCount, mallocFunc mallocPtr, freeFunc freePtr);

/*
 * @Get an object from the mempool
 *
 * @param
 *  mp: pointer to the Mempool
 *
 * @return
 *  the address of the object got from the Mempool
 */
void *mempool_get_object(struct Mempool *mp);

/*
 * @Put the object back to mempool
 *
 * @param
 *  mp: pointer to the Mempool
 *  obj: the pointer to the object to be released
 *
 * @return
 *  0: success
 *  -1: failed
 *
 */
int mempool_put_object(struct Mempool *mp, void *obj);

/*
 * @Release the entire mempool
 *
 * @param
 *  mp: the target Mempool to be released
 *
 * @return
 */
void mempool_free(struct Mempool *mp);

/*
 * @Release the unused memblock in the mempool back to system, the memory still in use will be keep intact
 *
 * @param
 *  mp: Mempool
 *
 * @return
 */
void mempool_release_unused(struct Mempool *mp);

#endif
