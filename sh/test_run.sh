# cd /home/sunyahui/lijiayu/open/code/D-TrimCDP-WB
# mkdir build
# cd build
# cmake ..
# make
#  ./bin/D-TrimCDP-WB 2 ../../../data/  Github  5 3 0 10

cd /home/sunyahui/lijiayu/open/code/TrimCDP-WB
mkdir build
cd build
cmake .. 
make
#exe type path data_name T task_start_num(from 0) task_end_num


#./bin/TrimCDP-WB 2 ../../../data/ Github 3 0 299
# ./bin/TrimCDP-WB 2 ../../../data/ Github 5 0 299
# ./bin/TrimCDP-WB 2 ../../../data/ Github 7 0 299
#./bin/TrimCDP-WB 2 ../../../data/ DBLP 3 0 299
 ./bin/TrimCDP-WB 2 ../../../data/ DBLP 5 0 10
# ./bin/TrimCDP-WB 2 ../../../data/ DBLP 7 0 10