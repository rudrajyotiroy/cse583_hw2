git config --global user.name rudrajyotiroy
git config --global user.email rudrajyotiroy@gmail.com
mkdir -p build && cd build
cmake ..
make
echo "Compile done"
cd ../benchmarks/correctness
sh run.sh hw2correct1
echo "Correctness 1 done"
sh run.sh hw2correct2
echo "Correctness 2 done"
sh run.sh hw2correct3
echo "Correctness 3 done"
sh run.sh hw2correct4
echo "Correctness 4 done"
sh run.sh hw2correct5
echo "Correctness 5 done"
sh run.sh hw2correct6
echo "Correctness 6 done"
cd ../performance
sh run.sh hw2perf1
echo "Performance 1 done"