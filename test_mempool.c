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

#define RTE_PROC_MAX 2
#define MEMPOOL_SIZE 1000000
#define ELEMENT_SIZE 40
//#define ELEMENT_SIZE 48
//#define ELEMENT_SIZE 16 

typedef int (*pFunc)(void*);

#ifdef DPDK_TEST
static __attribute__((unused))void *rte_malloc_wrap(size_t size)
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

static int primary_process(__attribute__((unused))void *args)
{
	return 0;
}

static int secondary_process(__attribute__((unused))void *args)
{
	return 0;
}

pFunc gProcessFunc[RTE_PROC_MAX] = { primary_process, secondary_process };

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
	struct rte_config *cfg = NULL;
	unsigned int lcore_id = 0;

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
	mmp = rte_mempool_create("tiger_test", MEMPOOL_SIZE, ELEMENT_SIZE, 128, 0, NULL, NULL, NULL, NULL, rte_socket_id(), flags);
	if( !mmp )
	{
		printf("Can't create mempool!\n");
	}
#endif

	gettimeofday(&t1, NULL);
	mp = mempool_create(ELEMENT_SIZE, MEMPOOL_SIZE, rte_malloc_wrap, rte_free_wrap);
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
		array[i] = malloc(ELEMENT_SIZE);
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
	rte_mempool_free(mmp);

	/*
	cfg = rte_eal_get_configuration();
	rte_eal_mp_remote_launch( gProcessFunc[cfg->process_type], NULL, SKIP_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore_id)
	{
		if( rte_eal_wait_lcore(lcore_id) < 0 )
		{
			printf("lcore %u failed!\n", lcore_id);
		}
	}
	*/

	return 0;
}

