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

For example, to run experiments for TrimCDP-WB, **using the following instruction**:

 ```
sh sh/exp_TrimCDP-WB.sh
 ```

For D-TrimCDP-WB experiments, **using the following instruction**:

 ```
sh sh/exp_D-TrimCDP-WB.sh
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
All codes are located in the 'code' folder. There are 2 subfolders, each corresponding to codes of one of 2 algorithms in the paper.

### Basic Algorithms:
- **TrimCDP-WB**. This is the TrimCDP-WB version code without diameter constraint for GST.
- **D-TrimCDP-WB**. This is the TrimCDP-WB version code with diameter constraints for GST.

In the 2 subfolders, there are .h, .cu, .cuh, and .cpp files used for conducting experiments in the paper. The .h and .cuh files are in the "include" directory, while the .cpp files are in the "src" directory. The explanations for them are as follows.

### TrimCDP-WB:
- "TrimCDP-WB/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB. 
- "TrimCDP-WB/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB.
- "TrimCDP-WB/include/mapper.cuh" contains codes for performing specific operations on vertices, such as grow and merge operations.
- "TrimCDP-WB/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:
 ```
sh sh/exp_TrimCDP-WB.sh
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


