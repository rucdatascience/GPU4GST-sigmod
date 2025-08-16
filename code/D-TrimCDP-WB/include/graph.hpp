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
	
	// 第二步：计算需要增加的空间（包括分割节点）
	new_vert_count = vert_count;
	std::vector<new_index_t> split_counts; // 记录每个大度数节点需要分割成几个子节点
	split_counts.resize(vert_count, 0);
	
	for (auto v : high_degree_vertices) {
		new_index_t degree = beg_pos[v + 1] - beg_pos[v];
		new_index_t num_splits = (degree + 511) / 512; // 向上取整，确保每个子节点度 <= 512
		split_counts[v] = num_splits;
		new_vert_count += num_splits; // 子节点
	}
	
	// 为每个大度数节点组添加一个分割节点
	new_vert_count += high_degree_vertices.size();
	
	std::cout << "new_vert_count: " << new_vert_count << std::endl;
	
	// 第三步：分配空间
	mother_map.resize(new_vert_count);
	son_d_map.resize(new_vert_count);
	son_range_start_map.resize(new_vert_count);
	son_range_end_map.resize(new_vert_count);
	
	// 第四步：初始化映射数组
	// 首先，所有节点默认映射到自己
	for (new_index_t i = 0; i < vert_count; i++) {
		mother_map[i] = i;
		son_d_map[i] = i;  // 节点默认映射到自己
		son_range_start_map[i] = i;
		son_range_end_map[i] = i; // 非大度数节点没有子节点，范围为空
	}
	
	// 第五步：为每个大度数节点创建子节点和分割节点
	new_index_t current_new_vertex = vert_count;
	for (auto v : high_degree_vertices) {
		new_index_t num_splits = split_counts[v];
		
		// 设置子节点范围起始位置
		son_range_start_map[v] = current_new_vertex;
		
		// 设置大度数节点的son_d_map为第一个子节点
		son_d_map[v] = current_new_vertex;
		
		// 创建子节点
		for (new_index_t i = 0; i < num_splits; i++) {
			mother_map[current_new_vertex] = v; // 子节点映射到父节点
			son_d_map[current_new_vertex] = current_new_vertex; // 子节点映射到自己
			current_new_vertex++;
		}
		
		// 设置子节点范围结束位置
		son_range_end_map[v] = current_new_vertex;
		
		// 创建分割节点（不映射到任何父节点，仅用于CSR分割）
		mother_map[current_new_vertex] = -1; // 分割节点没有父节点
		son_d_map[current_new_vertex] = current_new_vertex; // 分割节点映射到自己
		current_new_vertex++;
		
		// 打印调试信息
		std::cout << "设置大度数点 " << v << " 的子节点范围: [" << son_range_start_map[v] 
				  << ", " << (current_new_vertex - 1) << "] (分割节点: " << (current_new_vertex - 1) << ")" << std::endl;
	}
	
	// 第六步：构建son_list_map - 类似CSR格式
	son_list_map.clear();
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t start = son_range_start_map[i];
		new_index_t end;
		
		// 检查是否是大度数点
		new_index_t degree = beg_pos[i + 1] - beg_pos[i];
		if (degree > 1024) {
			// 大度数点：找到下一个大度数点的起始位置，或者到new_vert_count
			end = new_vert_count;
			for (auto v : high_degree_vertices) {
				if (v > i) {
					end = son_range_start_map[v];
					break;
				}
			}
		} else {
			// 非大度数点：没有子节点，范围为空
			end = start;
		}
		
		// 确保索引在有效范围内
		if (start < new_vert_count && end <= new_vert_count && start <= end) {
			for (new_index_t j = start; j < end; j++) {
				son_list_map.push_back(j);
			}
		}
	}
	std::cout << "son_list_map.size(): " << son_list_map.size() << std::endl;
	
	// 第七步：对于新增的子节点和分割节点，范围起始位置为自己
	for (new_index_t i = original_vert_count; i < new_vert_count; i++) {
		son_range_start_map[i] = i;
		son_range_end_map[i] = i + 1; // 结束位置是下一个位置
		son_d_map[i] = i;  // 子节点映射到自己
	}
	std::cout << "son_range_start_map.size(): " << son_range_start_map.size() << std::endl;
	
	// 第八步：保存原始的子节点范围信息，用于后续打印
	original_son_ranges.resize(original_vert_count);
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t start = son_range_start_map[i];
		new_index_t end;
		
		// 检查是否是大度数点
		new_index_t degree = beg_pos[i + 1] - beg_pos[i];
		if (degree > 1024) {
			// 大度数点：找到下一个大度数点的起始位置，或者到new_vert_count
			end = new_vert_count;
			for (auto v : high_degree_vertices) {
				if (v > i) {
					end = son_range_start_map[v];
					break;
				}
			}
		} else {
			// 非大度数点：没有子节点，范围为空
			end = start;
		}
		
		original_son_ranges[i] = std::make_pair(start, end);
		
		// 打印调试信息
		if (end - start > 1024) {
			std::cout << "节点 " << i << " 的子节点范围: [" << start << ", " << end << ")" << std::endl;
		}
		if(end < start)
		{
			std::cout<<"end<start "<<i<<" "<<start<<" "<<end<<std::endl;
		}
	}
}

template<
typename file_vert_t, typename file_index_t, typename file_weight_t,
typename new_vert_t, typename new_index_t, typename new_weight_t>
void graph<file_vert_t,file_index_t, file_weight_t,
new_vert_t,new_index_t,new_weight_t>::build_new_csr()
{
	std::cout << "开始构建新的CSR..." << std::endl;
	
	// 保存原始数据用于验证
	std::vector<new_index_t> original_beg_pos(beg_pos, beg_pos + original_vert_count + 1);
	std::vector<new_vert_t> original_adj_list(adj_list, adj_list + edge_count);
	std::vector<new_weight_t> original_weight(weight, weight + edge_count);
	
	// 第一步：计算新的边数量
	new_edge_count = edge_count; // 原始边数保持不变
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t degree = original_beg_pos[i + 1] - original_beg_pos[i];
		if (degree > 1024) {
			// 大度数节点：子节点指向父节点边表的子集，不增加新的边
			// 子节点只是重新组织父节点的边表，不复制边
		}
	}
	
	std::cout << "原始边数: " << edge_count << ", 新边数: " << new_edge_count << std::endl;
	
	// 第二步：分配新的CSR数组
	new_beg_pos = new new_index_t[new_vert_count + 1];
	new_adj_list = new new_vert_t[new_edge_count];
	new_weight = new new_weight_t[new_edge_count];
	
	// 第三步：构建新的CSR
	// 首先复制所有原始边表（保持位置不变）
	for (new_index_t i = 0; i < edge_count; i++) {
		new_adj_list[i] = original_adj_list[i];
		new_weight[i] = original_weight[i];
	}
	
	// 复制所有原始节点的beg_pos（保持位置不变）
	for (new_index_t i = 0; i < original_vert_count + 1; i++) {
		new_beg_pos[i] = original_beg_pos[i];
	}
	
	// 然后为子节点设置beg_pos，指向父节点边表的子集
	new_index_t vertex_offset = original_vert_count;
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t degree = original_beg_pos[i + 1] - original_beg_pos[i];
		
		if (degree > 1024) {
			// 大度数节点：为子节点设置beg_pos
			std::cout << "处理大度数点 " << i << " (度数: " << degree << ")" << std::endl;
			
			new_index_t num_splits = (degree + 511) / 512; // 向上取整
			
			new_index_t processed_edges = 0;
			for (new_index_t j = 0; j < num_splits; j++) {
				// 计算当前子节点的边数
				new_index_t edges_for_this_son;
				if (j == num_splits - 1) {
					// 最后一个子节点处理剩余的所有边
					edges_for_this_son = degree - processed_edges;
				} else {
					// 其他子节点处理512条边
					edges_for_this_son = 512;
				}
				
				// 设置子节点的beg_pos，指向父节点边表的子集
				new_beg_pos[vertex_offset + j] = original_beg_pos[i] + processed_edges;
				
				processed_edges += edges_for_this_son;
			}
			
			// 设置分割节点的beg_pos（没有边）
			new_beg_pos[vertex_offset + num_splits] = original_beg_pos[i] + degree; // 指向父节点边表的末尾
			
			vertex_offset += num_splits + 1; // +1 包括分割节点
		}
	}
	
	// 设置最后一个起始位置
	new_beg_pos[new_vert_count] = edge_count;
	
	// 第四步：更新图的CSR数据
	delete[] adj_list;
	delete[] beg_pos;
	delete[] weight;
	
	adj_list = new_adj_list;
	beg_pos = new_beg_pos;
	weight = new_weight;
	// 保持原始vertex_count不变，子节点只在grow中使用
	// vert_count = new_vert_count; // 注释掉这行，保持原始vertex_count
	edge_count = new_edge_count;
	
	std::cout << "新CSR构建完成: 原始顶点数 " << vert_count << ", 扩展顶点数 " << new_vert_count << ", " << new_edge_count << " 条边" << std::endl;
	
	// 打印分割后前5个大度数点的子节点在新CSR中的信息
	std::cout << "\n=== 验证分割结果 ===" << std::endl;
	int print_count = 0;
	for (new_index_t i = 0; i < original_vert_count; i++) {
		new_index_t original_degree = original_beg_pos[i + 1] - original_beg_pos[i];
		if (original_degree > 1024) {
			if (print_count >= 5) break;
			
			std::cout << "\n大度数点 " << i << " (原始度数: " << original_degree << "):" << std::endl;
			
			// 使用start和end数组直接获取子节点范围
			new_index_t start_son = son_range_start_map[i];
			new_index_t end_son = son_range_end_map[i];
			new_index_t num_sons = end_son - start_son;
			
			std::cout << "  子节点范围: [" << start_son << ", " << end_son << ") (共" << num_sons << "个子节点)" << std::endl;
			
			// 打印每个子节点的beg_pos信息
			std::cout << "  子节点beg_pos信息:" << std::endl;
			for (new_index_t j = 0; j < num_sons; j++) {
				new_index_t son_vertex = start_son + j;
				if (son_vertex + 1 < new_vert_count) {
					new_index_t son_degree = beg_pos[son_vertex + 1] - beg_pos[son_vertex];
					std::cout << "    子节点 " << son_vertex << ": beg_pos[" << son_vertex << "]=" << beg_pos[son_vertex] 
							  << ", beg_pos[" << (son_vertex + 1) << "]=" << beg_pos[son_vertex + 1] 
							  << " (度数: " << son_degree << ")" << std::endl;
				}
			}
			
			// 验证每个子节点的度数
			new_index_t total_son_edges = 0;
			for (new_index_t j = 0; j < num_sons; j++) {
				new_index_t son_vertex = start_son + j;
				if (son_vertex + 1 < new_vert_count) {
					new_index_t son_degree = beg_pos[son_vertex + 1] - beg_pos[son_vertex];
					total_son_edges += son_degree;
				}
			}
			
			// 验证总边数是否匹配
			std::cout << "  验证: 原始父节点 " << original_degree << " 条边, 子节点总计 " << total_son_edges << " 条边 ";
			if (original_degree == total_son_edges) {
				std::cout << "✓" << std::endl;
			} else {
				std::cout << "✗" << std::endl;
			}
			
			// 验证子节点度数是否符合512分割规则
			std::cout << "  512分割验证: ";
			bool split_correct = true;
			for (new_index_t j = 0; j < num_sons; j++) {
				new_index_t son_vertex = start_son + j;
				if (son_vertex + 1 < new_vert_count) {
					new_index_t son_degree = beg_pos[son_vertex + 1] - beg_pos[son_vertex];
					if (j < num_sons - 1 && son_degree != 512) {
						split_correct = false;
						break;
					}
					if (j == num_sons - 1 && son_degree > 512) {
						split_correct = false;
						break;
					}
				}
			}
			std::cout << (split_correct ? "✓" : "✗") << std::endl;
			
			// 验证父节点映射正确性
			std::cout << "  父节点映射验证: ";
			bool parent_mapping_correct = true;
			for (new_index_t j = 0; j < num_sons; j++) {
				new_index_t son_vertex = start_son + j;
				if (mother_map[son_vertex] != i) {
					parent_mapping_correct = false;
					break;
				}
			}
			std::cout << (parent_mapping_correct ? "✓" : "✗") << std::endl;
			
			print_count++;
		}
	}
}

