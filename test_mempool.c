#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#ifdef DPDK_TEST
#include <rte_common.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_ring.h>
#include <rte_lcore.h>
#include <rte_mempool.h>
#endif

#include "mempool.h"

#define MEMPOOL_SIZE 1000000

#ifdef DPDK_TEST
static __attribute__((unused))void *rte_malloc_wrap(int size)
{
	void *addr = NULL;

	addr = rte_malloc(NULL, size, 64);

	return addr;
}

static __attribute__((unused))void rte_free_wrap(void *addr)
{
	if( addr )
	{
		rte_free(addr);
	}
}
#endif

int main(int argc, char *argv[])
{
	struct Mempool *mp = NULL;
	struct timeval t1;
	struct timeval t2;
	void **array = NULL;
	int cnt = 0;
	int i = 0;
	int ret = 0;

	struct rte_mempool *mmp = NULL;
	int flags = 0;

	if( argc < 2 )
	{
		printf("Usage %s COUNT\n", argv[0]);
		return 0;
	}

#ifdef DPDK_TEST
	ret = rte_eal_init(argc, argv);
	if( ret < 0 )
	{
		printf("DPDK Init Failed!\n");
		return -1;
	}
#endif

	cnt = atoi(argv[1]);
	array = malloc(sizeof(void*)*cnt);

#ifdef DPDK_TEST
	flags = MEMPOOL_F_SP_PUT|MEMPOOL_F_SC_GET; 
	mmp = rte_mempool_create("tiger_test", MEMPOOL_SIZE, 40, 128, 0, NULL, NULL, NULL, NULL, rte_socket_id(), flags);
	if( !mmp )
	{
		printf("Can't create mempool!\n");
	}
#endif

	gettimeofday(&t1, NULL);
	mp = mempool_create(40, MEMPOOL_SIZE, NULL, NULL);
	gettimeofday(&t2, NULL);
	printf("Create Mempool Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));

	printf("---------------------------------------------------------\n");
#ifdef DPDK_TEST
	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		rte_mempool_get_bulk(mmp, &array[i], 1);
	}
	gettimeofday(&t2, NULL);
	printf("DPDK Mempool Get Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));

	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		rte_mempool_put_bulk(mmp, &array[i], 1);
	}
	gettimeofday(&t2, NULL);
	printf("DPDK Mempool Put Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));
	printf("---------------------------------------------------------\n");
#endif

	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		array[i] = mempool_get_object(mp);
	}
	gettimeofday(&t2, NULL);
	printf("Get Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));

	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		mempool_put_object(mp, array[i]);
	}
	gettimeofday(&t2, NULL);
	printf("Put Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));
	printf("---------------------------------------------------------\n");

	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		array[i] = malloc(40);
	}
	gettimeofday(&t2, NULL);
	printf("Malloc Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));

	gettimeofday(&t1, NULL);
	for( i = 0; i < cnt; i++)
	{
		free(array[i]);
	}
	gettimeofday(&t2, NULL);
	printf("Free Object Consume %ldus\n", (t2.tv_sec-t1.tv_sec)*1000000+(t2.tv_usec-t1.tv_usec));
	printf("---------------------------------------------------------\n");

	mempool_free(mp);

	return 0;
}

