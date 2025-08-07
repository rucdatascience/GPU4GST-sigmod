#ifndef __MAPPER__
#define __MAPPER__
#include "header.h"
#include "util.h"
#include "gpu_graph.cuh"
#include "meta_data.cuh"
#include <thrust/detail/minmax.h>
#include <assert.h>
typedef feature_t (*cb_mapper)(vertex_t active_edge_src,
							   vertex_t active_edge_end,
							   feature_t level,
							   index_t *beg_pos,
							   weight_t weight_list,
							   feature_t *vert_status,
							   feature_t *vert_status_prev);

// Bitmap could also be helpful
/* mapper kernel function */
class mapper
{
public:
	// variable
	vertex_t *adj_list;
	weight_t *weight_list;
	index_t *beg_pos;
	index_t vert_count;
	feature_t *vert_status;
	feature_t *vert_status_prev;
	feature_t *one_label_lower_bound;
	feature_t *lb_record;
	feature_t *in_queue;
	int width, full, *lb0;
	volatile int *best;
	int *merge_pointer;
	int *merge_groups;
	// index_t *cat_thd_count_sml;
	cb_mapper edge_compute;
	cb_mapper edge_compute_push;
	cb_mapper edge_compute_pull;
	
	/*大度数节点分割映射*/
	vertex_t *son_d;    // GPU端：节点到子节点的映射
	vertex_t *mother_d; // GPU端：子节点到父节点的映射
	
	/*大度数节点分割映射扩展*/
	index_t *son_range_start_d;  // GPU端：每个节点的子节点范围起始位置
	vertex_t *son_list_d;        // GPU端：所有子节点的列表
	index_t son_list_d_size;     // son_list_d数组的大小

public:
	// constructor
	mapper(gpu_graph ggraph, meta_data mdata,
		   cb_mapper user_mapper_push,
		   cb_mapper user_mapper_pull,
		   int wid)
	{
		adj_list = ggraph.adj_list;
		weight_list = ggraph.weight_list;
		beg_pos = ggraph.beg_pos;
		width = wid;
		lb0 = mdata.lb0;
		in_queue = mdata.in_queue;
		lb_record = mdata.lb_record;
		one_label_lower_bound = mdata.record;
		best = mdata.best;
		full = width - 1;
		merge_groups = ggraph.merge_groups_d;
		merge_pointer = ggraph.merge_pointer_d;

		vert_count = ggraph.vert_count;
		vert_status = mdata.vert_status;
		vert_status_prev = mdata.vert_status_prev;
		// cat_thd_count_sml = mdata.cat_thd_count_sml;
		
		// 初始化大度数节点分割映射
		son_d = ggraph.son_d;
		mother_d = ggraph.mother_d;
		
		// 初始化大度数节点分割扩展映射
		son_range_start_d = ggraph.son_range_start_d;
		son_list_d = ggraph.son_list_d;
		son_list_d_size = ggraph.son_list_d_size;

		edge_compute_push = user_mapper_push;
		edge_compute_pull = user_mapper_pull;
	}

	~mapper() {}

public:

	__forceinline__ __device__ void
	mapper_push(		 
		vertex_t wqueue, // 这个是本轮的用值
		vertex_t *worklist,
		index_t *cat_thd_count, // 这个是本轮返回值
		const index_t GRP_ID,
		const index_t GRP_SZ,
		const index_t GRP_COUNT,
		const index_t THD_OFF,
		feature_t level,
		volatile vertex_t *bests,
		feature_t *records)
	{
		index_t appr_work = 0;
		weight_t weight;
		const vertex_t WSZ = wqueue;
		for (index_t i = GRP_ID; i < WSZ; i += GRP_COUNT)
		{

			vertex_t frontier = worklist[i];

			int v = frontier / width, p = frontier % width;
			index_t beg = beg_pos[v], end = beg_pos[v + 1], x_slash = full - p, vline = v * width;
			//grow
			int parent_vertex = mother_d[v];
			for (index_t j = beg + THD_OFF; j < end; j += GRP_SZ)
			{
				
				vertex_t vert_end = adj_list[j];
				weight = weight_list[j];
				vertex_t update_dest = vert_end * width + p;
				feature_t dist = vert_status[parent_vertex * width + p] + weight;

			
				if (vert_status[update_dest] > dist)
				{
					int lb = get_lb(one_label_lower_bound, lb0, parent_vertex * width, x_slash);
					atomicMin(vert_status + update_dest, dist);
					if (lb + dist <= (*best))
					{
						// 将vert_end的所有子节点标记为入队
						index_t target_start = son_range_start_d[vert_end];
						index_t target_end = (vert_end + 1 < vert_count) ? son_range_start_d[vert_end + 1] : son_list_d_size;
						
						for (index_t k = target_start; k < target_end; k++) {
							vertex_t target_son = son_list_d[k];
							vertex_t son_queue_pos = target_son * width + p;
							in_queue[son_queue_pos] = 1;
						}
					}
				}
			}

			// merge - 修改为对节点的父节点进行操作
			
			beg = merge_pointer[p], end = merge_pointer[p + 1];
			
			for (index_t j = beg + THD_OFF; j < end; j += GRP_SZ)
			{
				weight_t weight = vert_status[parent_vertex * width + merge_groups[j]];
				vertex_t update_dest = parent_vertex * width + p + merge_groups[j];

				feature_t dist = vert_status[parent_vertex * width + p] + weight;
				x_slash = full - p - merge_groups[j];
				if (vert_status[update_dest] > dist)
				{
					atomicMin(vert_status + update_dest, dist);
					int lb = get_lb_m(one_label_lower_bound, lb0, parent_vertex * width, x_slash, dist);
					if (lb + dist <= (*best))
					{
						// 如果有更新，将父节点的所有子节点入队
						index_t parent_start = son_range_start_d[parent_vertex];
						index_t parent_end = (parent_vertex + 1 < vert_count) ? son_range_start_d[parent_vertex + 1] : son_list_d_size;
						
						// 遍历父节点的所有子节点并标记入队
						for (index_t k = parent_start; k < parent_end; k++) {
							vertex_t child = son_list_d[k];
							vertex_t child_queue_pos = child * width + merge_groups[j]+p;
							in_queue[child_queue_pos] = 1;
						}
					}
				}
			}
		}


		cat_thd_count[threadIdx.x + blockIdx.x * blockDim.x] = appr_work;
	}

	
	__device__ __forceinline__ int get_lb(feature_t *status, feature_t *lb0, int vline, int x_slash)
	{
		
		int ret = 0;
		for (int i = 1; i <= x_slash; i <<= 1)
		{
			if ((x_slash & i) && ret < status[vline + i])
			{
				ret = status[vline + i];
			}
		}
		ret = thrust::max(ret, lb0[vline + x_slash]);
		return ret;
	}
	__device__ __forceinline__ int get_lb_m(feature_t *status, feature_t *lb0, int vline, int x_slash, int w)
	{
		
		int ret = 0;
		for (int i = 1; i <= x_slash; i <<= 1)
		{
			if ((x_slash & i) && ret < status[vline + i])
			{
				ret = status[vline + i];
			}
		}
		ret = thrust::max(ret, lb0[vline + x_slash]);
		ret = ret > 0.5 * w ? ret : 0.5 * w;
		return ret;
	}
	__device__ __forceinline__ void
	mapper_bin_push( 
		index_t &appr_work,
		volatile vertex_t *overflow_indicator,
		vertex_t &my_front_count, 
		vertex_t *worklist_bin,	  // 也是工作队列
		vertex_t wqueue,		  // 传进来的时候只有这个是队列大小
		vertex_t *worklist,		  // 这个是工作队列
		const index_t GRP_ID,	  // 上面一层还是核函数 这里给定了对应的参数 warp在grid的位置
		const index_t GRP_SZ,	  // warp的大小
		const index_t GRP_COUNT,  // warp步长
		const index_t THD_OFF,	  // 线程在warp的位置
		feature_t level,
		index_t bin_off,
		volatile vertex_t *bests,
		feature_t *records,
		feature_t *lb_record)		 
	{								 // 从任务队列中获得对各个指向点的更新值 并且根据这些值对对应位置更新
		const vertex_t WSZ = wqueue; // 遍历工作队列

		for (index_t i = GRP_ID; i < WSZ; i += GRP_COUNT)
		{

			vertex_t frontier = worklist[i];
			int v = frontier / width, p = frontier % width;
			index_t beg = beg_pos[v], x_slash = full - p, vline = v * width;
			index_t end = beg_pos[v + 1];
			if (vert_status[vline + x_slash] != inf)
			{
				int new_value = vert_status[vline + x_slash] + vert_status[frontier];
				atomicMin((int *)bests, new_value);
			}
			else
			{
				int complement = 0;
				for (int i = 1; i <= x_slash; i <<= 1)
				{

					if (x_slash & i)
					{
						complement += records[vline + i];
					}
				}
				atomicMin((int *)bests, complement + vert_status[frontier]);
			}
			if (vert_status[frontier] - 1 > (*bests) / 2)
			{
				continue;
			}
			for (index_t j = beg + THD_OFF; j < end; j += GRP_SZ)
			{

				vertex_t vert_end = adj_list[j];
				weight_t weight = weight_list[j];
				vertex_t update_dest = vert_end * width + p;
				feature_t dist = (*edge_compute_push)(frontier, update_dest,
													  level, beg_pos, weight, vert_status, vert_status_prev); // 获取值 根据任务选择操作
				int lb = get_lb(one_label_lower_bound, lb0, vert_end * width, x_slash);
				lb_record[update_dest] = lb;
				if (vert_status[update_dest] > dist)
				{

					if (atomicMin(vert_status + update_dest, dist) > dist)
						if (dist + lb <= (*bests))
						{
							if (my_front_count < BIN_SZ)
							{
								worklist_bin[bin_off + my_front_count] = update_dest;
								my_front_count++;
								appr_work += beg_pos[vert_end + 1] - beg_pos[vert_end];
							}

							else
								overflow_indicator[0] = -1;
						}
				}
			}

			// merge
			beg = merge_pointer[p];
			end = merge_pointer[p + 1];
			vertex_t d = beg_pos[v + 1] - beg_pos[v];
			int dist;
			for (index_t j = beg + THD_OFF; j < end; j += GRP_SZ)
			{
				weight_t weight = vert_status[v * width + merge_groups[j]];
				vertex_t update_dest = frontier + merge_groups[j];

				dist = (*edge_compute_push)(frontier, update_dest,
											level, beg_pos, weight, vert_status, vert_status_prev); // 获取值 根据任务选择操作

				x_slash = full - p - merge_groups[j];
				int lb = get_lb(one_label_lower_bound, lb0, v * width, x_slash);
				lb_record[update_dest] = lb;
				if (vert_status[update_dest] > dist)
				{
					// atomicMin does not return mininum
					// instead the old val, in this case vert_status[vert_end].
					if (atomicMin(vert_status + update_dest, dist) > dist)

						if (dist - 1 <= 0.667 * (*bests) && dist + lb <= (*bests)) // 0.667 
						{
							if (my_front_count < BIN_SZ)
							{
								worklist_bin[bin_off + my_front_count] = update_dest;
								my_front_count++;
								appr_work += d;
							}

							else
								overflow_indicator[0] = -1;
						}
				}
			}
		}

		// Make sure NO overflow!
		if (my_front_count >= BIN_SZ)
			overflow_indicator[0] = -1;
	}

	__device__ __forceinline__ void
	mapper_bin_push_only( // 最初SSSP调用的函数
		index_t &appr_work,
		volatile vertex_t *overflow_indicator,
		vertex_t &my_front_count, // 初值为0 传引用
		vertex_t *worklist_bin,	  // 也是工作队列
		vertex_t wqueue,		  // 传进来的时候只有这个是队列大小
		vertex_t *worklist,		  // 这个是工作队列
		const index_t GRP_ID,	  // 上面一层还是核函数 这里给定了对应的参数 warp在grid的位置
		const index_t GRP_SZ,	  // warp的大小
		const index_t GRP_COUNT,  // warp步长
		const index_t THD_OFF,	  // 线程在warp的位置
		feature_t level,
		index_t bin_off)			 // 规定的线程bin大小是32 （有溢出可能是在这里
	{								 // 从任务队列中获得对各个指向点的更新值 并且根据这些值对对应位置更新
		const vertex_t WSZ = wqueue; // 遍历工作队列
		for (index_t i = GRP_ID; i < WSZ; i += GRP_COUNT)
		{
			vertex_t frontier = worklist[i];
			int v = frontier / width, p = frontier % width;
			index_t beg = beg_pos[v];
			index_t end = beg_pos[v + 1];

			for (index_t j = beg + THD_OFF; j < end; j += GRP_SZ)
			{
				vertex_t vert_end = adj_list[j];
				weight_t weight = weight_list[j];
				vertex_t update_dest = vert_end * width + p;
				feature_t dist = (*edge_compute_push)(frontier, update_dest,
													  level, beg_pos, weight, vert_status, vert_status_prev); // 获取值 根据任务选择操作
				if (vert_status[update_dest] > dist)
				{
					// atomicMin does not return mininum
					// instead the old val, in this case vert_status[vert_end].
					if (atomicMin(vert_status + update_dest, dist) > dist)
						if (my_front_count < BIN_SZ)
						{
							worklist_bin[bin_off + my_front_count] = update_dest;
							my_front_count++;
							appr_work += beg_pos[vert_end + 1] - beg_pos[vert_end];
						}
						else
							overflow_indicator[0] = -1;
				}
			}
		}

		// Make sure NO overflow!
		if (my_front_count >= BIN_SZ)
			overflow_indicator[0] = -1;
	}




	
};

#endif
