cd code/TrimCDP-WB-no_virtual_split
mkdir build
cd build
cmake .. 
make
#exe type path data_name T task_start_num(from 0) task_end_num

./bin/TrimCDP-WB 2 ../../../data/ Musae 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Musae 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Musae 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ Twitch 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Twitch 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Twitch 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ Github 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Github 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Github 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ Youtube 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Youtube 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Youtube 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ DBLP 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ DBLP 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ DBLP 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ Reddit 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Reddit 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Reddit 7 0 299

./bin/TrimCDP-WB 2 ../../../data/ Orkut 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ Orkut 5 0 299
./bin/TrimCDP-WB 2 ../../../data/ Orkut 7 0 299


./bin/TrimCDP-WB 2 ../../../data/ LiveJournal 3 0 299
./bin/TrimCDP-WB 2 ../../../data/ LiveJournal  5 0 299
./bin/TrimCDP-WB 2 ../../../data/ LiveJournal 7 0 299


