# Fast Optimal Group Steiner Tree Search using GPUs

## GST_data
The dataset of the paper is stored on [OneDrive](https://1drv.ms/f/c/683d9dd9f262486b/Ek6Fl_brQzhDnI2cmhGIHxMBQ-L1ApeSqxwZKE4NBsDXSQ?e=3RBc8S). There are eight datasets: Twitch, Musae, Github,  Youtube, Orkut, DBLP, Reddit, LiveJournal. There are 8 files for each dataset. For example, the Twitch dataset contains the following 8 files:
1. "Twitch.in". This readable file contains the basic information of this dataset. The two numbers on the first line of the file represent the number of vertices and edges in the graph. Each of the following lines contains three numbers representing the two end vertices and the weight of an edge. For example, "16 14919 100" shows that there is an edge between vertex 16 and vertex 14919, with an edge weight of 100.

2. "Twitch_beg_pos.bin". This is a binary file. It contains V elements, each element representing the starting position of a vertex's adjacency list. Therefore, the position of a vertex can be obtained by subtracting the starting position of the next vertex from the starting position of that vertex.

3. "Twitch_csr.bin". This is a binary file. It contains E elements, and this file stores an adjacency list of vertices, where each element represents an endpoint of an edge.

4. "Twitch_weight.bin". This is a binary file. It contains E elements, which store the weights of edges, with each element representing the weight of an edge.

5. "Twitch.g". Each line of this file represents which vertices in the graph are included in a group. For example, "g7:2705 13464 16088 16341 22323" indicates that group 7 contains five vertices: 2705, 13464, 16088, 16341, and 22323.

6. "Twitch3.csv". Each line of this file represents a query of size 3. For example, "2475 2384 159" indicates that the return tree of this query must contain group 2475, 2384, and 159.

7. "Twitch5.csv". Each line of this file represents a query of size 5. For example, "1016 2941 1613 1105 2228" indicates that the return tree of this query must contain group 1016, 2941, 1613, 1105, 2228.

8. "Twitch7.csv". Each line of this file represents a query of size 7. For example, "3137 393 742 25 2125 2122 727" indicates that the return tree of this query must contain group 3137, 393, 742, 25, 2125, 2122, 727.

Take the generation of the binary file for the Twitch dataset as an example. If you need to generate a corresponding binary file for a new dataset, use the following command:

```
cd data
g++ tuple_text_to_bin.cpp -o tuple_text_to_bin
./tuple_text_to_bin Twitch 1 1
```
The explanation of the last line of instructions is as follows:
| Parameter | Description |
|-----------|-------------|
| `./tuple_text_to_bin` | Execute binary files |
| `Twitch.in` | The filename of the dataset to be converted |
| `1` | 0 indicates that the graph is a directed graph, while 1 indicates that the graph is an undirected graph. |
| `1` | Indicates to ignore the first line. If you need to ignore more lines, you can modify this parameter. |

Regarding how the program reads binary files, you can refer to the code located at "code/TrimCDP-WB/include/graph.hpp"
   
## Running code example
Here, we show how to build and run experiments on a Linux server with the Ubuntu 20.04 system, an Intel(R) Xeon(R) Platinum 8360Y CPU @ 2.40GHz, and 1 NVIDIA GeForce RTX A6000 GPU. The environment is as follows:
- gcc version 9.3.0 (GCC)
- CUDA compiler NVIDIA 11.8.89
- CMake version 3.28.3
- Boost

Please note:
- The CMake version should be 3.27 or above.
- In the above environment, if you need to run GPU code, it is recommended to install the corresponding CUDA version to avoid possible compilation errors caused by version incompatibility.
- The above environment is introduced by CMake after installation.  At the same time, the instructions in the CMake file will download some external libraries, so please run the following mentioned sh command on a machine with network connection.

We will provide a detailed introduction to the experimental process as follows.

In your appropriate directory, execute the following commands:

 **Download the code**:
```
git clone https://anonymous.4open.science/r/GPU4GST/
```
 **Switch the working directory to GPU4GST.**
```
cd GPU4GST
```
 **Download the dataset from [OneDrive](https://1drv.ms/f/c/683d9dd9f262486b/Ek6Fl_brQzhDnI2cmhGIHxMBQ-L1ApeSqxwZKE4NBsDXSQ?e=3RBc8S).**

Download the dataset from OneDrive to the "data" folder. **Please note that "data" is the default path for storing the dataset. We have already provided the Twitch dataset in the "data" folder in advance. Please store the other datasets in a similar manner.  If the storage location is incorrect, the program will not be able to read the data.**



After preparing the environment and dataset according to the above suggestions, we can use the sh files in the "sh" folder to compile and run the code.
Among them, example.sh conducts experiments on six base algorithms introduced in "GST_Code" section of this readme using the Twitch dataset, with each algorithm executing 50 queries of size 3. **The running instruction is**:
 ```
sh sh/example.sh
 ```
**The experiment results will be automatically saved as CSV files in the "data/result" folder. The CSV file will store data such as the queried group, cost of the solution, running time, and number of processing vertices.**


The other six sh files correspond to complete experiments of an algorithm on eight datasets, with 300 queries of sizes 3, 5, and 7 executed on each dataset. For example, to run experiments for TrimCDP-WB, **using the following instruction**:

 ```
sh sh/exp_TrimCDP-WB.sh
 ```

For optimization analysis algorithms, the sh files are located in the "sh/additional_exp" folder. **For example, to run experiments for TrimCDP-WB without kernel fusion and shared memory:**

 ```
sh sh/additional_exp/exp_TrimCDP-WB-no_kernel_fusion-no_shared_memory-coalescing.sh
 ```

For multi-core CPU algorithms, the sh files are also located in the "sh/additional_exp" folder. **For example, to run experiments for TrimCDP multi-core CPU version:**

 ```
sh sh/additional_exp/exp_TrimCDP-multi-core-CPU.sh
 ```


Taking exp_D-TrimCDP-WB.sh as an example, The explanation for the sh file is as follows:

| Command | Description |
|---------|-------------|
| `cd code/D-TrimCDP-WB/build` | Navigate to the algorithm directory |
| `mkdir build` | Create build directory |
| `cd build` | Enter build directory |
| `cmake ..` | Configure the build with CMake |
| `make` | Compile the code into executable file |
```
./bin/D-TrimCDP-WB 2 ../../../data/ Musae 3 5 0 299
```
The explanation for this line is as follows:

| Parameter | Description |
|-----------|-------------|
| `./bin/D-TrimCDP-WB` | Execute binary files |
| `2` | Number of GPU threads to use for computation |
| `../../../data/` | Directory path where the dataset files are stored |
| `Musae` | Name of the dataset to be used for the experiment |
| `3` | Size of each query (number of groups to connect) |
| `5` | Upper bound for the diameter constraint of the solution tree |
| `0` | Starting index of queries to execute  |
| `299` | Ending index of queries to execute  |

This command will execute 300 queries (from index 0 to 299) of size 3 on the Musae dataset using the D-TrimCDP-WB algorithm with a diameter constraint of 5.


## GST_code
All codes are located in the 'code' folder. There are 16 subfolders, each corresponding to codes of one of 16 experiments in the paper.

### Basic Algorithms (without diameter constraints):
- **PrunedDP++**. This is the PrunedDP++ version code of GST without diameter constraints.
- **TrimCDP**. This is the TrimCDP version code without diameter constraint for GST.
- **TrimCDP-WB**. This is the TrimCDP-WB version code without diameter constraint for GST.

### Basic Algorithms (with diameter constraints):
- **D-PrunedDP++**. This is the PrunedDP++ version code with diameter constraints for GST.
- **D-TrimCDP**. This is the TrimCDP version code with diameter constraints for GST.
- **D-TrimCDP-WB**. This is the TrimCDP-WB version code with diameter constraints for GST.

### Optimization Analysis Algorithms (without diameter constraints):

| Algorithm Variant | Kernel Fusion | shared_memory_prefix_scan | Global Memory Coalescing | Description |
|-------------------|---------------|--------------------------|-------------------------|-------------|
| **TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing** | ✓ | ✓ | ✓ | kernel fusion, shared memory prefix scan, global memory coalescing |
| **TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing** | ✓ | ✗ | ✓ | kernel fusion, global memory coalescing, no shared memory prefix scan |
| **TrimCDP-WB-no_kernel_fusion** | ✗ | ✗ | ✓ | no kernel fusion, no shared memory prefix scan, global memory coalescing |
| **TrimCDP-WB-no_global_memory_coalescing** | ✓ | ✗ | ✗ | kernel fusion, no shared memory prefix scan, no global memory coalescing |

### Optimization Analysis Algorithms (with diameter constraints):

| Algorithm Variant | Kernel Fusion | shared_memory_prefix_scan | Global Memory Coalescing | Description |
|-------------------|---------------|--------------------------|-------------------------|-------------|
| **D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing** | ✓ | ✓ | ✓ | kernel fusion, shared memory prefix scan, global memory coalescing |
| **D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing** | ✓ | ✗ | ✓ | kernel fusion, global memory coalescing, no shared memory prefix scan |
| **D-TrimCDP-WB-no_kernel_fusion** | ✗ | ✗ | ✓ | no kernel fusion, no shared memory prefix scan, global memory coalescing |
| **D-TrimCDP-WB-no_global_memory_coalescing** | ✓ | ✗ | ✗ | kernel fusion, no shared memory prefix scan, no global memory coalescing |

### Multi-core CPU Algorithms:
- **TrimCDP-multi-core-CPU**. This is the multi-threaded version of TrimCDP without using a priority queue.
- **D-TrimCDP-multi-core-CPU**. This is the multi-threaded version of D-TrimCDP without using a priority queue.

In the 16 subfolders, there are .h, .cu, .cuh, and .cpp files used for conducting experiments in the paper. The .h and .cuh files are in the "include" directory, while the .cpp files are in the "src" directory. The explanations for them are as follows.


### PrunedDP++:
- "PrunedDP++/src/main.cpp" contains codes for conducting experiments for PrunedDP++. 
- "PrunedDP++/include/CPUNONHOP.h" contains codes of PrunedDP++.

The command to run this experiment is:
 ```
sh sh/exp_prunedDP++.sh
 ```

### TrimCDP:
- "TrimCDP/src/main.cpp" contains codes for conducting experiments for TrimCDP. 
- "TrimCDP/include/exp_GPU_nonHop.h" contains code for reading the graph, groups, and queries.
- "TrimCDP/src/DPQ.cu" contains codes of TrimCDP.

The command to run this experiment is:
 ```
sh sh/exp_TrimCDP.sh
 ```

### TrimCDP-WB:
- "TrimCDP-WB/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB. 
- "TrimCDP-WB/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB.
- "TrimCDP-WB/include/mapper.cuh" contains codes for performing specific operations on vertices, such as grow and merge operations.
- "TrimCDP-WB/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
 ```
sh sh/exp_TrimCDP-WB.sh
 ```

### D-PrunedDP++:
- "D-PrunedDP++/src/main.cpp" contains codes for conducting experiments for D-PrunedDP++. 
- "D-PrunedDP++/include/CPUHOP.h" contains codes of D-PrunedDP++.

The command to run this experiment is:
 ```
sh sh/exp_D-prunedDP++.sh
 ```

### D-TrimCDP:
- "D-TrimCDP/src/main.cpp" contains codes for conducting experiments for D-TrimCDP. 
- "D-TrimCDP/include/exp_GPU_Hop.h" contains code for reading the graph, groups, and queries.
- "D-TrimCDP/src/DPQ.cu" contains codes of D-TrimCDP.

The command to run this experiment is:
 ```
sh sh/exp_D-TrimCDP.sh
 ```

### D-TrimCDP-WB:
- "D-TrimCDP-WB/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB. 
- "D-TrimCDP-WB/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB.
- "D-TrimCDP-WB/include/mapper.cuh" contains codes for performing specific operations on vertices, such as grow and merge operations.
- "D-TrimCDP-WB/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
 ```
sh sh/exp_D-TrimCDP-WB.sh
 ```

### TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing:
- "TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB with kernel fusion, shared memory prefix scan, and global memory coalescing optimizations.
- "TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB with optimizations.
- "TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices with optimizations.
- "TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing.sh
```

### TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing:
- "TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB with kernel fusion and global memory coalescing, without shared memory prefix scan.
- "TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB with these optimizations.
- "TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices with these optimizations.
- "TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing.sh
```

### TrimCDP-WB-no_kernel_fusion:
- "TrimCDP-WB-no_kernel_fusion/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB without kernel fusion but with global memory coalescing.
- "TrimCDP-WB-no_kernel_fusion/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB without kernel fusion.
- "TrimCDP-WB-no_kernel_fusion/include/mapper.cuh" contains codes for performing specific operations on vertices without kernel fusion.
- "TrimCDP-WB-no_kernel_fusion/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_TrimCDP-WB-no_kernel_fusion.sh
```

### TrimCDP-WB-no_global_memory_coalescing:
- "TrimCDP-WB-no_global_memory_coalescing/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB with kernel fusion but without global memory coalescing.
- "TrimCDP-WB-no_global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB without global memory coalescing.
- "TrimCDP-WB-no_global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices without global memory coalescing.
- "TrimCDP-WB-no_global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_TrimCDP-WB-no_global_memory_coalescing.sh
```

### D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing:
- "D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB with kernel fusion, shared memory prefix scan, and global memory coalescing optimizations.
- "D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB with optimizations.
- "D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices with optimizations.
- "D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_D-TrimCDP-WB-kernel_fusion-shared_memory_prefix_scan-global_memory_coalescing.sh
```

### D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing:
- "D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB with kernel fusion and global memory coalescing, without shared memory prefix scan.
- "D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB with these optimizations.
- "D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices with these optimizations.
- "D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_D-TrimCDP-WB-kernel_fusion-no_shared_memory_prefix_scan-global_memory_coalescing.sh
```

### D-TrimCDP-WB-no_kernel_fusion:
- "D-TrimCDP-WB-no_kernel_fusion/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB without kernel fusion but with global memory coalescing.
- "D-TrimCDP-WB-no_kernel_fusion/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB without kernel fusion.
- "D-TrimCDP-WB-no_kernel_fusion/include/mapper.cuh" contains codes for performing specific operations on vertices without kernel fusion.
- "D-TrimCDP-WB-no_kernel_fusion/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/exp_D-TrimCDP-WB-no_kernel_fusion.sh
```

### D-TrimCDP-WB-no_global_memory_coalescing:
- "D-TrimCDP-WB-no_global_memory_coalescing/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB with kernel fusion but without global memory coalescing.
- "D-TrimCDP-WB-no_global_memory_coalescing/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB without global memory coalescing.
- "D-TrimCDP-WB-no_global_memory_coalescing/include/mapper.cuh" contains codes for performing specific operations on vertices without global memory coalescing.
- "D-TrimCDP-WB-no_global_memory_coalescing/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/additional_exp/D-TrimCDP-WB-no_global_memory_coalescing.sh
```

### TrimCDP-WB-no_virtual_split:
- "TrimCDP-WB-no_virtual_split/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB without virtual splitting optimization.
- "TrimCDP-WB-no_virtual_split/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/mapper.cuh" contains codes for performing specific operations on vertices without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/exp_TrimCDP-WB-no_virtual_split.sh
```

### D-TrimCDP-WB-no_virtual_split:
- "D-TrimCDP-WB-no_virtual_split/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB without virtual splitting optimization.
- "D-TrimCDP-WB-no_virtual_split/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB without virtual splitting.
- "D-TrimCDP-WB-no_virtual_split/include/mapper.cuh" contains codes for performing specific operations on vertices without virtual splitting.
- "D-TrimCDP-WB-no_virtual_split/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
```
sh sh/exp_D-TrimCDP-WB-no_virtual_split.sh
```

### TrimCDP-multi-core-CPU:
- "TrimCDP-multi-core-CPU/src/main.cpp" contains codes for conducting experiments for TrimCDP multi-core CPU version.
- "TrimCDP-multi-core-CPU/include/CPUNONHOP.h" contains codes of TrimCDP multi-core CPU implementation.

The command to run this experiment is:
```
sh sh/additional_exp/exp_TrimCDP-multi-core-CPU.sh
```

### D-TrimCDP-multi-core-CPU:
- "D-TrimCDP-multi-core-CPU/src/main.cpp" contains codes for conducting experiments for D-TrimCDP multi-core CPU version.
- "D-TrimCDP-multi-core-CPU/include/CPUHOP.h" contains codes of D-TrimCDP multi-core CPU implementation.

The command to run this experiment is:
```
sh sh/additional_exp/exp_D-TrimCDP-multi-core-CPU.sh
```


