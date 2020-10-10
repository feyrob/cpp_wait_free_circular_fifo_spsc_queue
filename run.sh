
cd ${0%/*}

rm app 2>/dev/null
set -e
clang++ -std=c++11 ./main.cpp -o app -lboost_thread -lboost_system

# -ldl -lboost_system -lboost_filesystem
./app
