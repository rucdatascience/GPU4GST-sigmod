#include "graph.h"
#include <unistd.h>
#include<vector>
#include<string>
#include <fstream>

template<
typename file_vert_t, typename file_index_t, typename file_weight_t,
typename new_vert_t, typename new_index_t, typename new_weight_t>
graph<file_vert_t,file_index_t, file_weight_t,
new_vert_t,new_index_t,new_weight_t>
::graph(
		const char *beg_file,
		const char *adj_file,
		const char *weight_file
		)
{
	
	double tm=wtime();
	FILE *file=NULL;
	file_index_t ret;


	
	vert_count=fsize(beg_file)/sizeof(file_index_t) - 1;
	edge_count=fsize(adj_file)/sizeof(file_vert_t);
	
	file=fopen(beg_file, "rb");
	if(file!=NULL)
	{
		file_index_t *tmp_beg_pos=NULL;

		if(posix_memalign((void **)&tmp_beg_pos, getpagesize(),
					sizeof(file_index_t)*(vert_count+1)))
			perror("posix_memalign");
//posix_memalign 为数组分配临时空间
		ret=fread(tmp_beg_pos, sizeof(file_index_t), 
				vert_count+1, file);
		assert(ret==vert_count+1);
		fclose(file);
		edge_count=tmp_beg_pos[vert_count];
		std::cout<<"Expected edge count: "<<tmp_beg_pos[vert_count]<<"\n";

        assert(tmp_beg_pos[vert_count]>0);

		//converting to new type when different 
		if(sizeof(file_index_t)!=sizeof(new_index_t))
		{
			if(posix_memalign((void **)&beg_pos, getpagesize(),
					sizeof(new_index_t)*(vert_count+1)))
			perror("posix_memalign");
			for(new_index_t i=0;i<vert_count+1;++i)
				beg_pos[i]=(new_index_t)tmp_beg_pos[i];
			delete[] tmp_beg_pos;
		}else{beg_pos=(new_index_t*)tmp_beg_pos;}
	}else std::cout<<"beg file cannot open\n";

	file=fopen(adj_file, "rb");
	if(file!=NULL)
	{
		file_vert_t *tmp_adj_list = NULL;
		if(posix_memalign((void **)&tmp_adj_list,getpagesize(),
						sizeof(file_vert_t)*edge_count))
			perror("posix_memalign");
		
		ret=fread(tmp_adj_list, sizeof(file_vert_t), edge_count, file);
		assert(ret==edge_count);
		assert(ret==beg_pos[vert_count]);
		fclose(file);
			
		if(sizeof(file_vert_t)!=sizeof(new_vert_t))
		{
			if(posix_memalign((void **)&adj_list,getpagesize(),
						sizeof(new_vert_t)*edge_count))
				perror("posix_memalign");
			for(new_index_t i=0;i<edge_count;++i)
				adj_list[i]=(new_vert_t)tmp_adj_list[i];
			delete[] tmp_adj_list;
		}else adj_list =(new_vert_t*)tmp_adj_list;

	}else std::cout<<"adj file cannot open\n";


	file=fopen(weight_file, "rb");
	if(file!=NULL)
	{
		file_weight_t *tmp_weight = NULL;
		if(posix_memalign((void **)&tmp_weight,getpagesize(),
					sizeof(file_weight_t)*edge_count))
			perror("posix_memalign");
		
		ret=fread(tmp_weight, sizeof(file_weight_t), edge_count, file);
		assert(ret==edge_count);
		fclose(file);
	
		if(sizeof(file_weight_t)!=sizeof(new_weight_t))
		{
			if(posix_memalign((void **)&weight,getpagesize(),
						sizeof(new_weight_t)*edge_count))
				perror("posix_memalign");
			for(new_index_t i=0;i<edge_count;++i)
            {
                weight[i]=(new_weight_t)tmp_weight[i];
                //if(weight[i] ==0)
                //{
                //    std::cout<<"zero weight: "<<i<<"\n";
                //    exit(-1);
                //}
            }

			delete[] tmp_weight;
		}else weight=(new_weight_t*)tmp_weight;
	}
	else std::cout<<"Weight file cannot open\n";
    
	std::cout<<"Graph load (success): "<<vert_count<<" verts, "
		<<edge_count<<" edges "<<wtime()-tm<<" second(s)\n";
		
	// 执行大度数节点分割
	split_high_degree_vertices();
	build_new_csr();
	
	// 输出分割后的最终图信息
	std::cout<<"After high-degree vertex splitting: "<<vert_count<<" verts, "
		<<edge_count<<" edges "<<wtime()-tm<<" second(s)\n";
}

// 大度数节点分割函数实现
template<
typename file_vert_t, typename file_index_t, typename file_weight_t,
typename new_vert_t, typename new_index_t, typename new_weight_t>
void graph<file_vert_t,file_index_t, file_weight_t,
new_vert_t,new_index_t,new_weight_t>::split_high_degree_vertices()
{
	original_vert_count = vert_count;
	
	// 第一步：计算需要分割的大度数节点
	std::vector<new_index_t> high_degree_vertices;
	for (new_index_t i = 0; i < vert_count; i++) {
		new_index_t degree = beg_pos[i + 1] - beg_pos[i];
		if (degree > 1024) {
			high_degree_vertices.push_back(i);
		}
	}
	
	std::cout << "Found " << high_degree_vertices.size() << " high-degree vertices (>1024)" << std::endl;
	
	// 第二步：计算需要增加的空间
	new_vert_count = vert_count;
	std::vector<new_index_t> split_counts; // 记录每个大度数节点需要分割成几个子节点
	split_counts.resize(vert_count, 0);
	
	for (auto v : high_degree_vertices) {
		new_index_t degree = beg_pos[v + 1] - beg_pos[v];
		new_index_t num_splits = (degree + 511) / 512; // 向上取整，确保每个子节点度 <= 512
		split_counts[v] = num_splits;
		new_vert_count += num_splits;
	}
	
	std::cout << "new_vert_count: " << new_vert_count << std::endl;
	
	// 第三步：分配空间
	mother_map.resize(new_vert_count);
	son_range_start_map.resize(new_vert_count);
	
	// 第四步：初始化映射数组
	// 首先，所有节点默认映射到自己
	for (new_index_t i = 0; i < vert_count; i++) {
		mother_map[i] = i;
		son_range_start_map[i] = i;
	}
	
	// 第五步：为每个大度数节点创建子节点
	new_index_t current_new_vertex = vert_count;
	for (auto v : high_degree_vertices) {
		new_index_t num_splits = split_counts[v];
		
		// 设置子节点范围起始位置
		son_range_start_map[v] = current_new_vertex;
		
		// 创建子节点
		for (new_index_t i = 0; i < num_splits; i++) {
			mother_map[current_new_vertex] = v; // 子节点映射到父节点
			current_new_vertex++;
		}
		
		// 打印调试信息
		// std::cout << "设置大度数点 " << v << " 的子节点范围: [" << son_range_start_map[v] 
		// 		  << ", " << current_new_vertex << ")" << std::endl;
	}
	
	// std::cout << "son_range_start_map.size(): " << son_range_start_map.size() << std::endl;
	
	// 第六步：构建son_list_map - 类似CSR格式
	son_list_map.clear();
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t start = son_range_start_map[i];
		new_index_t end = (i + 1 < original_vert_count) ? son_range_start_map[i + 1] : new_vert_count;
		// 确保索引在有效范围内
		if (start < new_vert_count && end <= new_vert_count && start <= end) {
			for (new_index_t j = start; j < end; j++) {
				son_list_map.push_back(j);
			}
		}
	}
	// std::cout << "son_list_map.size(): " << son_list_map.size() << std::endl;
	
	// 第七步：对于新增的子节点，范围起始位置为自己
	for (new_index_t i = original_vert_count; i < new_vert_count; i++) {
		son_range_start_map[i] = i;
	}
	// std::cout << "son_range_start_map.size(): " << son_range_start_map.size() << std::endl;
	
	// 第八步：保存原始的子节点范围信息，用于后续打印
	original_son_ranges.resize(original_vert_count);
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t start = son_range_start_map[i];
		new_index_t end = (i + 1 < original_vert_count) ? son_range_start_map[i + 1] : new_vert_count;
		original_son_ranges[i] = std::make_pair(start, end);
		
		// 打印调试信息
		// if (end - start>1024) {
		// 	std::cout << "节点 " << i << " 的子节点范围: [" << start << ", " << end << ")" << std::endl;
		// }
		// if(end<start)
		// {
		// 	std::cout<<"end<start "<<i<<" "<<start<<" "<<end<<std::endl;
		// }
	}
	
	// 打印前5个大度数点的分割信息
	// std::cout << "\n=== 前5个大度数点分割信息 ===" << std::endl;
	// int print_count = 0;
	// for (auto v : high_degree_vertices) {
	// 	if (print_count >= 5) break;
	// 	
	// 	new_index_t degree = beg_pos[v + 1] - beg_pos[v];
	// 	new_index_t num_splits = split_counts[v];
	// 	new_index_t start_son = original_son_ranges[v].first;
	// 	new_index_t end_son = original_son_ranges[v].second;
	// 	
	// 	std::cout << "大度数点 " << v << " (度数: " << degree << "):" << std::endl;
	// 	std::cout << "  分割成 " << num_splits << " 个子节点" << std::endl;
	// 	std::cout << "  子节点范围: [" << start_son << ", " << end_son << ")" << std::endl;
	// 	std::cout << "  子节点列表: ";
	// 	
	// 	// 打印子节点列表
	// 	for (new_index_t i = start_son; i < end_son && i < new_vert_count; i++) {
	// 		std::cout << i;
	// 		if (i < end_son - 1 && i < new_vert_count - 1) {
	// 			std::cout << ", ";
	// 		}
	// 	}
	// 	std::cout << std::endl;
	// 	
	// 	// 打印每个子节点的映射信息
	// 	std::cout << "  子节点映射信息:" << std::endl;
	// 	for (new_index_t i = start_son; i < end_son && i < new_vert_count; i++) {
	// 		std::cout << "    子节点 " << i << ": mother_map=" << mother_map[i] 
	// 				  << ", son_range_start=" << son_range_start_map[i] << std::endl;
	// 	}
	// 	std::cout << std::endl;
	// 	
	// 	print_count++;
	// }
}

template<
typename file_vert_t, typename file_index_t, typename file_weight_t,
typename new_vert_t, typename new_index_t, typename new_weight_t>
void graph<file_vert_t,file_index_t, file_weight_t,
new_vert_t,new_index_t,new_weight_t>::build_new_csr()
{
	// 计算新的边数量
	new_edge_count = 0;
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t degree = beg_pos[i + 1] - beg_pos[i];
		if (degree > 1024) {
			// 大度数节点：只为子节点创建边表，不保留原始边表
			new_edge_count += degree; // 只计算子节点的边表
		} else {
			// 普通节点：边数量不变
			new_edge_count += degree;
		}
	}
	
	// 分配新的CSR数组
	new_beg_pos = new new_index_t[new_vert_count + 1];
	new_adj_list = new new_vert_t[new_edge_count];
	new_weight = new new_weight_t[new_edge_count];
	
	// 构建新的CSR
	new_index_t edge_offset = 0;
	new_index_t vertex_offset = original_vert_count;
	
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t degree = beg_pos[i + 1] - beg_pos[i];
		
		if (degree > 1024) {
			// 大度数节点：不保留原始边表，只创建子节点
			// 大度数节点映射到第一个子节点
			new_index_t first_son = original_son_ranges[i].first;
			new_beg_pos[i] = edge_offset; // 大度数节点指向第一个子节点的边表起始位置
			
			// 为子节点复制边表
			new_index_t num_splits = (degree + 511) / 512;
			new_index_t edges_per_split = degree / num_splits;
			new_index_t remaining_edges = degree % num_splits;
			
			new_index_t current_edge = beg_pos[i];
			for (new_index_t j = 0; j < num_splits; j++) {
				new_index_t split_degree = edges_per_split + (j < remaining_edges ? 1 : 0);
				new_beg_pos[vertex_offset + j] = edge_offset;
				
				// 复制边到子节点
				for (new_index_t k = 0; k < split_degree; k++) {
					new_adj_list[edge_offset + k] = adj_list[current_edge + k];
					new_weight[edge_offset + k] = weight[current_edge + k];
				}
				
				edge_offset += split_degree;
				current_edge += split_degree;
			}
			vertex_offset += num_splits;
		} else {
			// 普通节点：直接复制
			new_beg_pos[i] = edge_offset;
			for (new_index_t j = 0; j < degree; j++) {
				new_adj_list[edge_offset + j] = adj_list[beg_pos[i] + j];
				new_weight[edge_offset + j] = weight[beg_pos[i] + j];
			}
			edge_offset += degree;
		}
	}
	
	// 设置最后一个起始位置
	new_beg_pos[new_vert_count] = edge_offset;
	
	// 更新图的CSR数据为新的分割后的数据
	delete[] adj_list;
	delete[] beg_pos;
	delete[] weight;
	
	adj_list = new_adj_list;
	beg_pos = new_beg_pos;
	weight = new_weight;
	vert_count = new_vert_count;
	edge_count = new_edge_count;
	
	std::cout << "New CSR built: " << new_vert_count << " vertices, " << new_edge_count << " edges" << std::endl;
	
	// 打印分割后前5个大度数点的子节点在新CSR中的信息
	// std::cout << "\n=== 分割后前5个大度数点的子节点CSR信息 ===" << std::endl;
	// int print_count = 0;
	// for (new_index_t i = 0; i < original_vert_count; i++) {
	// 	new_index_t degree = beg_pos[i + 1] - beg_pos[i];
	// 	if (degree > 1024) {
	// 		if (print_count >= 5) break;
	// 		
	// 		std::cout << "大度数点 " << i << " (度数: " << degree << "):" << std::endl;
	// 		std::cout << "  原始beg_pos: [" << beg_pos[i] << ", " << beg_pos[i + 1] << ")" << std::endl;
	// 		
	// 		// 找到对应的子节点
	// 		new_index_t start_son = original_son_ranges[i].first;
	// 		new_index_t end_son = original_son_ranges[i].second;
	// 		
	// 		std::cout << "  子节点范围: [" << start_son << ", " << end_son << ")" << std::endl;
	// 		
	// 		// 打印每个子节点的beg_pos信息
	// 		for (new_index_t j = start_son; j < end_son && j < new_vert_count; j++) {
	// 			if (j + 1 < new_vert_count) {
	// 				std::cout << "    子节点 " << j << ": beg_pos[" << j << "]=" << beg_pos[j] 
	// 						  << ", beg_pos[" << j + 1 << "]=" << beg_pos[j + 1] 
	// 						  << " (度数: " << beg_pos[j + 1] - beg_pos[j] << ")" << std::endl;
	// 			}
	// 		}
	// 		
	// 		// 打印父节点分割给子节点的实际边信息
	// 		std::cout << "  父节点分割给子节点的边信息:" << std::endl;
	// 		
	// 		// 保存原始边信息用于打印
	// 		std::vector<std::pair<new_vert_t, new_weight_t>> original_edges;
	// 		for (new_index_t j = beg_pos[i]; j < beg_pos[i + 1]; j++) {
	// 			original_edges.push_back(std::make_pair(adj_list[j], weight[j]));
	// 		}
	// 		
	// 		// 打印前10条原始边
	// 		std::cout << "    父节点 " << i << " 的前10条边: ";
	// 		for (new_index_t j = 0; j < std::min((new_index_t)10, degree); j++) {
	// 			std::cout << "(" << original_edges[j].first << ", " << original_edges[j].second << ")";
	// 			if (j < std::min((new_index_t)9, degree - 1)) {
	// 				std::cout << ", ";
	// 			}
	// 		}
	// 		std::cout << std::endl;
	// 		
	// 		// 打印每个子节点的前10条边
	// 		for (new_index_t j = start_son; j < end_son && j < new_vert_count; j++) {
	// 			if (j + 1 < new_vert_count) {
	// 				new_index_t son_degree = beg_pos[j + 1] - beg_pos[j];
	// 				std::cout << "    子节点 " << j << " 的前" << std::min((new_index_t)10, son_degree) << "条边: ";
	// 				
	// 				for (new_index_t k = 0; k < std::min((new_index_t)10, son_degree); k++) {
	// 					new_index_t edge_idx = beg_pos[j] + k;
	// 					if (edge_idx < edge_count) {
	// 						std::cout << "(" << adj_list[edge_idx] << ", " << weight[edge_idx] << ")";
	// 						if (k < std::min((new_index_t)9, son_degree - 1)) {
	// 							std::cout << ", ";
	// 						}
	// 					}
	// 				}
	// 				std::cout << std::endl;
	// 			}
	// 		}
	// 		std::cout << std::endl;
	// 		
	// 		print_count++;
	// 	}
	// }
}

