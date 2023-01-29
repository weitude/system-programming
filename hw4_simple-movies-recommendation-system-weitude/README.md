# SP_HW4_release

**[SPEC Link](https://hackmd.io/@claire2222/SkLyBSnIo)**

**[Video Link](https://www.youtube.com/watch?v=ZpgwOeGoqBA)**

**[Discussion Space Link](https://github.com/NTU-SP/SP_HW4_release/discussions)**

## How to exec?
In order not to let the test result of the running time of the program be affected by the current workload, we will run your program 3-5 times and take the test result with the shortest running time for each testcase. Meanwhile, we will check the consistency of your results among the tests. If the results are inconsistent, or once the program is crashed, then you will not get points of the testcase. 

Your `Makefile` should produce executable file `tserver` for using multithread to do merge sort  and `pserver` for using multiprocess to do merge sort. The output results `{}.out` of your program should place in your current working dir. TA will pull your repository and replace the `lib.c` library and run with following command to test your program in **linux1.csie.ntu.edu.tw**:  
```
make 
./pserver < /path/to/input/data  
./tserver < /path/to/input/data
```

Remember `make clean` to remove all {}.out and executable file.  
Note that it does not mean you can get full points by successfully running with these two programs in the time limit. We will check about if you have completed the requirements, for instance, using provided sort library, multiprocess programming, i.e., `fork()`.

### Tips
You can quickly test the execution time of your program using following command (note that the variable `real time` in bash using `time` command is counted as time uses):
```
for i in {1..5};do time ./tserver < /path/to/input$i.txt;echo "";done
```
