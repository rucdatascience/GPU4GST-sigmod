#ifndef _ENACTOR_H_
#define _ENACTOR_H_
#include "gpu_graph.cuh"
#include "meta_data.cuh"

#include "util.h"
#include "header.h"
#include "prefix_scan.cuh"
#include <limits.h>
#include "barrier.cuh"

__global__ void
balanced_push_kernel(
	feature_t *level,
	gpu_graph ggraph,
	meta_data mdata,
	mapper compute_mapper,
	reducer worklist_gather,
	Barrier global_barrier)
{

	//__shared__ vertex_t smem[32]; // 32位共享内存
	const index_t TID = threadIdx.x + blockIdx.x * blockDim.x;
	const index_t GRNTY = blockDim.x * gridDim.x;
	const index_t tid_in_wrp = threadIdx.x & 31;   // 线程在warp的位置
	const index_t wid_in_blk = threadIdx.x >> 5;   // warp在block的位置
	const index_t wid_in_grd = TID >> 5;		   // warp在grid的位置
	const index_t wcount_in_blk = blockDim.x >> 5; // 一个block里面有多少warp
	const index_t WGRNTY = GRNTY >> 5;			   // warp stride

	feature_t level_thd = level[0];

	if (!TID)
	{
		level[6] = 0;
		mdata.worklist_sz_mid[0] = 0;
	}

	global_barrier.sync_grid_opt();

	int r = 0;
	while (true && ++r < 20)
	{
		// 平衡
		mdata.future_work[0] = 0;
		global_barrier.sync_grid_opt();

		if (!TID)
		{
			int tot = mdata.worklist_sz_mid[0] + mdata.worklist_sz_sml[0] + mdata.worklist_sz_lrg[0];
			level[6] += tot;
			mdata.worklist_sz_mid[0] = 0; // 下面应该是在扫描执行了 先把新的队列长度置0
			mdata.worklist_sz_sml[0] = 0; // indicate whether bin overflow
			mdata.worklist_sz_lrg[0] = 0;
			if (tot > level[5])
				level[5] = tot;
		}

		worklist_gather._push_coalesced_scan_random_list(TID, wid_in_blk, tid_in_wrp, wcount_in_blk, GRNTY, level_thd + 1);

		global_barrier.sync_grid_opt();
		if (mdata.worklist_sz_sml[0] +
				mdata.worklist_sz_mid[0] +
				mdata.worklist_sz_lrg[0] ==
			0)
			break;

		compute_mapper.mapper_push(
			mdata.worklist_sz_lrg[0],
			mdata.worklist_lrg,
			mdata.cat_thd_count_lrg,
			blockIdx.x,	 /*group id*/
			blockDim.x,	 /*group size*/
			gridDim.x,	 /*group count*/
			threadIdx.x, /*thread off intra group*/
			level_thd);

		compute_mapper.mapper_push(
			mdata.worklist_sz_mid[0],
			mdata.worklist_mid,
			mdata.cat_thd_count_mid,

			wid_in_grd, /*group id*/
			32,			/*group size*/
			WGRNTY,		/*group count*/
			tid_in_wrp, /*thread off intra group*/
			level_thd);

		compute_mapper.mapper_push(
			mdata.worklist_sz_sml[0],
			mdata.worklist_sml,
			mdata.cat_thd_count_sml,
			TID,   /*group id*/
			1,	   /*group size*/
			GRNTY, /*group count*/
			0,	   /*thread off intra group*/
			level_thd);

		//      global_barrier.sync_grid_opt();

		_grid_sum<vertex_t, index_t>(mdata.cat_thd_count_sml[TID] +
										 mdata.cat_thd_count_mid[TID] +
										 mdata.cat_thd_count_lrg[TID],
									 mdata.future_work);

		global_barrier.sync_grid_opt();
		if (mdata.future_work[0] > ggraph.edge_count * SWITCH_TO && 0)
		{

			break;
		}
	}

	if (!TID)
		level[0] = level_thd;
}

int balanced_push(
	int cfg_blk_size,
	feature_t *level,
	gpu_graph ggraph,
	meta_data mdata,
	mapper compute_mapper,
	reducer worklist_gather,
	Barrier global_barrier)
{
	// 分成三种情况大中小 现在要改的函数！！！
	int blk_size = 0;
	int grd_size = 0;
	// cudaFuncGetAttributes
	cudaOccupancyMaxPotentialBlockSize(&grd_size, &blk_size,
									   balanced_push_kernel, 0, 0);

	grd_size = (blk_size * grd_size) / cfg_blk_size;
	blk_size = cfg_blk_size;

	assert(blk_size * grd_size <= BLKS_NUM * THDS_NUM);

	// push_pull_opt_kernel
	double time = wtime();
	balanced_push_kernel<<<grd_size, blk_size>>>(level,
												 ggraph,
												 mdata,
												 compute_mapper,
												 worklist_gather,
												 global_barrier);

	H_ERR(cudaDeviceSynchronize());

	return 0;
}

__global__ void balanced_push_kernel_atomic(
	feature_t *level,
	gpu_graph ggraph,
	meta_data mdata,
	mapper compute_mapper,
	reducer worklist_gather,
	Barrier global_barrier)
{

	//__shared__ vertex_t smem[32]; // 32位共享内存
	const index_t TID = threadIdx.x + blockIdx.x * blockDim.x;
	const index_t GRNTY = blockDim.x * gridDim.x;
	const index_t tid_in_wrp = threadIdx.x & 31;   // 线程在warp的位置
	const index_t wid_in_blk = threadIdx.x >> 5;   // warp在block的位置
	const index_t wid_in_grd = TID >> 5;		   // warp在grid的位置
	const index_t wcount_in_blk = blockDim.x >> 5; // 一个block里面有多少warp
	const index_t WGRNTY = GRNTY >> 5;			   // warp stride

	feature_t level_thd = level[0];

	if (!TID)
	{
		level[6] = 0;
		mdata.worklist_sz_mid[0] = 0;
	}

	global_barrier.sync_grid_opt();

	int r = 0;
	while (true)
	{
		// 平衡
		mdata.future_work[0] = 0;
		global_barrier.sync_grid_opt();

		if (!TID)
		{
			//printf("vert_count: %d\n", ggraph.vert_count);
			int tot = mdata.worklist_sz_mid[0] + mdata.worklist_sz_sml[0] + mdata.worklist_sz_lrg[0];
			level[6] += tot;
			mdata.worklist_sz_mid[0] = 0; // 下面应该是在扫描执行了 先把新的队列长度置0
			mdata.worklist_sz_sml[0] = 0; // indicate whether bin overflow
			mdata.worklist_sz_lrg[0] = 0;
			mdata.worklist_sz_merge[0] = 0;
			if (tot > level[5])
				level[5] = tot;
		}

		worklist_gather._push_coalesced_scan_random_list_atomic(TID, wid_in_blk, tid_in_wrp, wcount_in_blk, GRNTY, level_thd + 1);

		global_barrier.sync_grid_opt();
		if (mdata.worklist_sz_sml[0] +
				mdata.worklist_sz_mid[0] +
				mdata.worklist_sz_lrg[0]+mdata.worklist_sz_merge[0] ==
			0)
			break;

		compute_mapper.mapper_push_grow(
			mdata.worklist_sz_lrg[0],
			mdata.worklist_lrg,
			mdata.cat_thd_count_lrg,
			blockIdx.x,	 /*group id*/
			blockDim.x,	 /*group size*/
			gridDim.x,	 /*group count*/
			threadIdx.x, /*thread off intra group*/
			level_thd);

		compute_mapper.mapper_push_grow(
			mdata.worklist_sz_mid[0],
			mdata.worklist_mid,
			mdata.cat_thd_count_mid,

			wid_in_grd, /*group id*/
			32,			/*group size*/
			WGRNTY,		/*group count*/
			tid_in_wrp, /*thread off intra group*/
			level_thd);

		compute_mapper.mapper_push_grow(
			mdata.worklist_sz_sml[0],
			mdata.worklist_sml,
			mdata.cat_thd_count_sml,
			TID,   /*group id*/
			1,	   /*group size*/
			GRNTY, /*group count*/
			0,	   /*thread off intra group*/
			level_thd);
		compute_mapper.mapper_push_merge(
			mdata.worklist_sz_merge[0],
			mdata.worklist_merge,
			TID,   /*group id*/
			1,	   /*group size*/
			GRNTY, /*group count*/
			0,	   /*thread off intra group*/
			level_thd);
		//      global_barrier.sync_grid_opt();

		_grid_sum<vertex_t, index_t>(mdata.cat_thd_count_sml[TID] +
										 mdata.cat_thd_count_mid[TID] +
										 mdata.cat_thd_count_lrg[TID],
									 mdata.future_work);

		global_barrier.sync_grid_opt();
		if (mdata.future_work[0] > ggraph.edge_count * SWITCH_TO && 0)
		{

			break;
		}
	}

	if (!TID)
		level[0] = level_thd;
}

int balanced_push_atomic(
	int cfg_blk_size,
	feature_t *level,
	gpu_graph ggraph,
	meta_data mdata,
	mapper compute_mapper,
	reducer worklist_gather,
	Barrier global_barrier)
{
	// 分成三种情况大中小 现在要改的函数！！！
	int blk_size = 0;
	int grd_size = 0;
	// cudaFuncGetAttributes
	cudaOccupancyMaxPotentialBlockSize(&grd_size, &blk_size,
									   balanced_push_kernel, 0, 0);

	grd_size = (blk_size * grd_size) / cfg_blk_size;
	blk_size = cfg_blk_size;

	assert(blk_size * grd_size <= BLKS_NUM * THDS_NUM);

	// push_pull_opt_kernel
	double time = wtime();
	balanced_push_kernel_atomic<<<grd_size, blk_size>>>(level,
												 ggraph,
												 mdata,
												 compute_mapper,
												 worklist_gather,
												 global_barrier);

	H_ERR(cudaDeviceSynchronize());

	return 0;
}


#endif
