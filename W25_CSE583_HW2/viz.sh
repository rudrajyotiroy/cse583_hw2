git config --global user.name rudrajyotiroy
git config --global user.email rudrajyotiroy@gmail.com
mkdir -p build && cd build
cmake ..
make
echo "Compile done"
cd ../benchmarks/correctness
rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
sh run.sh hw2correct1
sh viz.sh hw2correct1
echo "Correctness 1 done"

# For later
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2correct2
# sh viz.sh hw2correct2
# echo "Correctness 2 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2correct3
# sh viz.sh hw2correct3
# echo "Correctness 3 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2correct4
# sh viz.sh hw2correct4
# echo "Correctness 4 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2correct5
# sh viz.sh hw2correct5
# echo "Correctness 5 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2correct6
# sh viz.sh hw2correct6
# echo "Correctness 6 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# cd ../performance
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll
# sh run.sh hw2perf1
# sh viz.sh hw2perf1
# echo "Performance 1 done"
# rm -f default.profraw *_prof *_fplicm *.bc *.profdata *_output *.ll