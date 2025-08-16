
// Graph data structure on GPUs
#ifndef _GPU_GRAPH_H_
#define _GPU_GRAPH_H_

#include "header.h"
#include "util.h"
#include "graph.h"

class gpu_graph
{
public:
	vertex_t *adj_list;
	weight_t *weight_list;
	index_t *beg_pos;
	int *merge_pointer_d, *merge_groups_d;
	index_t vert_count;
	index_t edge_count;
	index_t avg_degree;
	index_t new_vert_count; // 扩展后的顶点数，用于边界检查
	
	/*大度数节点分割映射*/
	vertex_t *son_d;    // GPU端：节点到子节点的映射
	vertex_t *mother_d; // GPU端：子节点到父节点的映射
	
	/*大度数节点分割映射扩展*/
	index_t *son_range_start_d;  // GPU端：每个节点的子节点范围起始位置
	index_t *son_range_end_d;    // GPU端：每个节点的子节点范围结束位置
	vertex_t *son_list_d;        // GPU端：所有子节点的列表
	index_t son_list_d_size;     // son_list_d数组的大小

public:
	~gpu_graph() {}

	gpu_graph(
		graph<long, long, long, vertex_t, index_t, weight_t> *ginst)
	{
		vert_count = ginst->vert_count; // 保持原始vertex_count用于算法逻辑
		edge_count = ginst->edge_count;
		avg_degree = ginst->edge_count / ginst->vert_count;
		new_vert_count = ginst->new_vert_count; // 扩展后的顶点数

		size_t weight_sz = sizeof(weight_t) * edge_count;
		size_t adj_sz = sizeof(vertex_t) * edge_count;
		// 使用扩展后的顶点数来分配beg_pos内存
		size_t beg_sz = sizeof(index_t) * (ginst->new_vert_count + 1);

		/* Alloc GPU space */
		H_ERR(cudaMalloc((void **)&adj_list, adj_sz));
		H_ERR(cudaMalloc((void **)&beg_pos, beg_sz));
		H_ERR(cudaMalloc((void **)&weight_list, weight_sz));

		/* copy it to GPU */
		H_ERR(cudaMemcpy(adj_list, ginst->adj_list,
						 adj_sz, cudaMemcpyHostToDevice));
		H_ERR(cudaMemcpy(beg_pos, ginst->beg_pos,
						 beg_sz, cudaMemcpyHostToDevice));

		H_ERR(cudaMemcpy(weight_list, ginst->weight,
						 weight_sz, cudaMemcpyHostToDevice));
						 
		// 为大度数节点分割映射分配GPU内存
		size_t map_sz = sizeof(vertex_t) * ginst->new_vert_count; // 使用扩展后的顶点数
		H_ERR(cudaMalloc((void **)&son_d, map_sz));
		H_ERR(cudaMalloc((void **)&mother_d, map_sz));
		
		// 复制映射数据到GPU
		H_ERR(cudaMemcpy(son_d, ginst->son_d_map.data(),
						 map_sz, cudaMemcpyHostToDevice));
		H_ERR(cudaMemcpy(mother_d, ginst->mother_map.data(),
						 map_sz, cudaMemcpyHostToDevice));
						 
		// 为大度数节点分割扩展映射分配GPU内存
		size_t range_sz = sizeof(index_t) * ginst->new_vert_count; // 使用扩展后的顶点数
		size_t list_sz = sizeof(vertex_t) * ginst->son_list_map.size();
		H_ERR(cudaMalloc((void **)&son_range_start_d, range_sz));
		H_ERR(cudaMalloc((void **)&son_range_end_d, range_sz));
		H_ERR(cudaMalloc((void **)&son_list_d, list_sz));
		
		// 复制扩展映射数据到GPU
		H_ERR(cudaMemcpy(son_range_start_d, ginst->son_range_start_map.data(),
						 range_sz, cudaMemcpyHostToDevice));
		H_ERR(cudaMemcpy(son_range_end_d, ginst->son_range_end_map.data(),
						 range_sz, cudaMemcpyHostToDevice));
		H_ERR(cudaMemcpy(son_list_d, ginst->son_list_map.data(),
						 list_sz, cudaMemcpyHostToDevice));
						 
		// 设置son_list_d_size
		son_list_d_size = ginst->son_list_map.size();
	}
	void release()
	{
		cudaFree(adj_list);
		cudaFree(beg_pos);
		cudaFree(weight_list);
		cudaFree(merge_groups_d);
		cudaFree(merge_pointer_d);
		cudaFree(son_d);
		cudaFree(mother_d);
		cudaFree(son_range_start_d);
		cudaFree(son_range_end_d);
		cudaFree(son_list_d);
	}
};

#endif
