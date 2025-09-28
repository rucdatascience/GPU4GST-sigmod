# Fast Optimal Group Steiner Tree Search using GPUs

## Preparation

### Preparation of the environment

Here, we show how to build and run experiments on a Linux server with the `Ubuntu 20.04` system, an `Intel(R) Xeon(R) Platinum 8360Y CPU @ 2.40GHz`, and one `NVIDIA GeForce RTX A6000 GPU`. The environment is as follows:

- gcc version 9.3.0 (GCC)
- CUDA compiler NVIDIA 11.8.89
- CMake version 3.28.3
- Boost 1.85.0

Please note:

- The CMake version should be 3.27 or above.
- In the above environment, if you need to run GPU code, it is recommended to install the corresponding CUDA version to avoid possible compilation errors caused by version incompatibility.
- **Because the instructions in the CMake file will download some external libraries such as rapids-cmake, so please run the following mentioned sh command on a machine with network connection.**

### Test whether the environment preparation is sufficient

**With network connection and in the root directory of the project, run the example.sh**

```
sh sh/example.sh
```

This command will download the dependencies, build all needed algorithms and run simple tests on Twitch dataset that has been prepared in the data folder in advance. If there is no error, you may proceed with further testing.

### Preparation of the datasets

**Download the datasets from [OneDrive](https://1drv.ms/f/c/683d9dd9f262486b/Ek6Fl_brQzhDnI2cmhGIHxMBQ-L1ApeSqxwZKE4NBsDXSQ?e=3RBc8S) into the "data" folder in advance** **(You can click the "Download" button at the top of this page to download the zip of all files. There is no need to download one by one)**. **Each dataset includes 8 parts. For testing, ensure all parts are downloaded to the data directory.**

If you meet problem like:
```
Archive:  OneDrive_2_2025-9-28.zip
warning [OneDrive_2_2025-9-28.zip]:  2236006777 extra bytes at beginning or within zipfile
  (attempting to process anyway)
error [OneDrive_2_2025-9-28.zip]:  start of central directory not found;
  zipfile corrupt.
  (please check that you have transferred or created the zipfile in the
  appropriate BINARY mode and that you have compiled UnZip properly)
```
Try to use `7z` to extract files rather than `unzip`.

Please note that "data" is the default path for storing the dataset. We have already provided the Twitch dataset in the "data" folder in advance. Please store the other datasets in a similar manner. 

> The dataset of the paper is stored on [OneDrive](https://1drv.ms/f/c/683d9dd9f262486b/Ek6Fl_brQzhDnI2cmhGIHxMBQ-L1ApeSqxwZKE4NBsDXSQ?e=3RBc8S). There are eight datasets: Twitch, Musae, Github,  Youtube, Orkut, DBLP, Reddit, LiveJournal. There are 8 files for each dataset. For example, the Twitch dataset contains the following 8 files:
>
> 1. "Twitch.in". This readable file contains the basic information of this dataset. The two numbers on the first line of the file represent the number of vertices and edges in the graph. Each of the following lines contains three numbers representing the two end vertices and the weight of an edge. For example, "16 14919 100" shows that there is an edge between vertex 16 and vertex 14919, with an edge weight of 100.
>
> 2. "Twitch_beg_pos.bin". This is a binary file. It contains V elements, each element representing the starting position of a vertex's adjacency list. Therefore, the position of a vertex can be obtained by subtracting the starting position of the next vertex from the starting position of that vertex.
>
> 3. "Twitch_csr.bin". This is a binary file. It contains E elements, and this file stores an adjacency list of vertices, where each element represents an endpoint of an edge.
>
> 4. "Twitch_weight.bin". This is a binary file. It contains E elements, which store the weights of edges, with each element representing the weight of an edge.
>
> 5. "Twitch.g". Each line of this file represents which vertices in the graph are included in a group. For example, "g7:2705 13464 16088 16341 22323" indicates that group 7 contains five vertices: 2705, 13464, 16088, 16341, and 22323.
>
> 6. "Twitch3.csv". Each line of this file represents a query of size 3. For example, "2475 2384 159" indicates that the return tree of this query must contain group 2475, 2384, and 159.
>
> 7. "Twitch5.csv". Each line of this file represents a query of size 5. For example, "1016 2941 1613 1105 2228" indicates that the return tree of this query must contain group 1016, 2941, 1613, 1105, 2228.
>
> 8. "Twitch7.csv". Each line of this file represents a query of size 7. For example, "3137 393 742 25 2125 2122 727" indicates that the return tree of this query must contain group 3137, 393, 742, 25, 2125, 2122, 727.

### Test whether the dataset preparation is sufficient

**In the root directory of the project, run the test_datasets.sh**

```
sh sh/test_datasets.sh
```

This command verifies the integrity and proper placement of all required datasets within the data directory to prevent data availability errors during execution. If both the environment configuration and dataset validation pass successfully, you may proceed with subsequent testing phases.

## Experiments

**The result will store into "data/result/". Before running the scripts, you should keep the directory clean.**

### Without diameter constraints

#### PrunedDP++

> This is the basic PrunedDP++ version code of GST without diameter constraints.

- "PrunedDP++/src/main.cpp" contains codes for conducting experiments for PrunedDP++. 
- "PrunedDP++/include/CPUNONHOP.h" contains codes of PrunedDP++.

The command to run this experiment is:

```
sh sh/exp_prunedDP++.sh
```

#### TrimCDP-WB

> This is the TrimCDP-WB version code without diameter constraint for GST.

- "TrimCDP-WB/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB. 
- "TrimCDP-WB/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB.
- "TrimCDP-WB/include/mapper.cuh" contains codes for performing specific operations on vertices, such as grow and merge operations.
- "TrimCDP-WB/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:

```
sh sh/exp_TrimCDP-WB.sh
```

#### TrimCDP-WB-no_virtual_split

> This is the TrimCDP-WB version code without virtual splitting optimization.

- "TrimCDP-WB-no_virtual_split/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB without virtual splitting optimization.
- "TrimCDP-WB-no_virtual_split/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/mapper.cuh" contains codes for performing specific operations on vertices without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:

```
sh sh/exp_TrimCDP-WB-no_virtual_split.sh
```

### With diameter constraints

#### D-PrunedDP++

> This is the PrunedDP++ version code with diameter constraints for GST

- "PrunedDP++/src/main.cpp" contains codes for conducting experiments for PrunedDP++. 
- "PrunedDP++/include/CPUNONHOP.h" contains codes of PrunedDP++.

The command to run this experiment is:

 ```
sh sh/exp_prunedDP++.sh
 ```

#### D-TrimCDP-WB

> This is the TrimCDP-WB version code with diameter constraints for GST.

- "D-TrimCDP-WB/src/GPUHop.cu" contains codes for conducting experiments for D-TrimCDP-WB. 
- "D-TrimCDP-WB/include/mapper_enactor.cuh" contains the overall framework of D-TrimCDP-WB.
- "D-TrimCDP-WB/include/mapper.cuh" contains codes for performing specific operations on vertices, such as grow and merge operations.
- "D-TrimCDP-WB/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:

 ```
sh sh/exp_D-TrimCDP-WB.sh
 ```

#### D-TrimCDP-WB-no_virtual_split

> This is the TrimCDP-WB version code without virtual splitting optimization, with diameter constraints.

- "TrimCDP-WB-no_virtual_split/src/GSTnonHop.cu" contains codes for conducting experiments for TrimCDP-WB without virtual splitting optimization.
- "TrimCDP-WB-no_virtual_split/include/mapper_enactor.cuh" contains the overall framework of TrimCDP-WB without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/mapper.cuh" contains codes for performing specific operations on vertices without virtual splitting.
- "TrimCDP-WB-no_virtual_split/include/reducer.cuh" contains codes for organizing and allocating work after completing vertices operations.

The command to run this experiment is:

 ```
sh sh/exp_TrimCDP-WB-no_virtual_split.sh
 ```

## Notes

### How to generate the binary file for a new dataset?

Take the generation of the binary file for the Twitch dataset as an example. If you need to generate a corresponding binary file for a new dataset, use the following command:

```
cd data
g++ tuple_text_to_bin.cpp -o tuple_text_to_bin
./tuple_text_to_bin Twitch 1 1
```

The explanation of the last line of instructions is as follows:

| Parameter             | Description                                                  |
| --------------------- | ------------------------------------------------------------ |
| `./tuple_text_to_bin` | Execute binary files                                         |
| `Twitch`           | The name of the dataset to be converted. The corresponding filename is `Twitch.in`                 |
| `1`                   | 0 indicates that the graph is a directed graph, while 1 indicates that the graph is an undirected graph. |
| `1`                   | Indicates to ignore the first line. If you need to ignore more lines, you can modify this parameter. |

Regarding how the program reads binary files, you can refer to the code located at "code/TrimCDP-WB/include/graph.hpp".

### Detailed explanation of the bash scripts

In exp_D-TrimCDP-WB.sh:

```cmd
./bin/D-TrimCDP-WB 2 ../../../data/ Musae 3 5 0 299
```

This command will execute 300 queries (from index 0 to 299) of size 3 on the Musae dataset using the D-TrimCDP-WB algorithm with a diameter constraint of 5.

The detailed explanation for this line is as follows:

| Parameter            | Description                                                  |
| -------------------- | ------------------------------------------------------------ |
| `./bin/D-TrimCDP-WB` | Execute binary files                                         |
| `2`                  | Number of GPU threads to use for computation                 |
| `../../../data/`     | Directory path where the dataset files are stored            |
| `Musae`              | Name of the dataset to be used for the experiment            |
| `3`                  | Size of each query (number of groups to connect)             |
| `5`                  | Upper bound for the diameter constraint of the solution tree |
| `0`                  | Starting index of queries to execute                         |
| `299`                | Ending index of queries to execute                           |

