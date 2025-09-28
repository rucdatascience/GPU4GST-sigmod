cd code/D-PrunedDP++
mkdir build
cd build
cmake ..
make 
./bin/D-PrunedDP++ 2 ../../../data/ Twitch 3 4 0 50

cd ../../../code/D-TrimCDP-WB
mkdir build
cd build
cmake ..
make 
./bin/D-TrimCDP-WB 2 ../../../data/ Twitch 3 4 0 50


cd ../../../code/D-TrimCDP-WB-no_virtual_split
mkdir build
cd build
cmake ..
make 
./bin/D-TrimCDP-WB 2 ../../../data/ Twitch 3 4 0 50

cd ../../../code/PrunedDP++
mkdir build
cd build
cmake ..
make 
./bin/PrunedDP++ 1 ../../../data/ Twitch 3 0 50

cd ../../../code/TrimCDP-WB
mkdir build
cd build
cmake ..
make 
./bin/TrimCDP-WB 2 ../../../data/ Twitch 3 0 50

cd ../../../code/TrimCDP-WB-no_virtual_split
mkdir build
cd build
cmake ..
make 
./bin/TrimCDP-WB 2 ../../../data/ Twitch 3 0 50