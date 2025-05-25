
#g++ -std=c++11 microbench_ideal.cpp -g -O3 -lnuma -fopenmp -o microbench_ideal -DAUTONUMA
g++ -std=c++14 microbench_ideal.cpp -g -O3 -lnuma -fopenmp -o microbench_ideal -DMANUAL
#g++ -std=c++14 -I/ssd1/songxin8/thesis/bigmembench_common/tinylfu/ microbench_ideal.cpp -g -O3 -lnuma -fopenmp -o microbench_ideal -DLFU 


#g++ test2.cpp -g -O0 -fopenmp -o damo_test2
#g++ -std=c++11 microbench_ideal.cpp -I/ssd1/songxin8/thesis/folly -g -O0 -lnuma -fopenmp -o microbench_ideal
#g++ damo_test_working2.cpp -g -fopenmp -o damo_working


