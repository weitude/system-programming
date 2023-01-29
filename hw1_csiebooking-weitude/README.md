# SP-Hw1-release

## Reminders
You should not modify content under `.github`. If caught, you will receive a 0 for this assignment.  
If you use some advanced features like branch, make sure your code is merged into `main` branch (the default branch). TA will use the last commit before deadline in `main` branch for judging. 

## Sample Judge
In addtion to testing your server with `./read_server`, `./write_server` and `telnet`, we provide `sample_judge.py` and a `bookingRecord` so that you can test your program with a single command `python sample_judge.py`.  

### Arguments
`sample_judge.py` accepts following optional arguments:  
* `-v`, `--verbose`: verbose. if you set this argument, `sample_judge.py` will print messages delivered between server and client, which may help debugging.
* `-t TASK [TASK ...]`, `--task TASK [TASK ...]`, Specify which tasks you want to run. If you didn't set this argument, `sample_judge.py` will run all tasks by default.
  * Valid `TASK` are `["1_1", "1_2", "2", "3", "4", "5_1", "5_2"]`.  

E.g, `python sample_judge.py -t 1_1 3 5_1 -v` set `sample_judge.py` to be verbose, and runs only task 1-1, task 3 and task 5-1.

### Notice on Sample Judge
Score printed by `sample_judge.py` is just for your reference. Real judge is much more complicated than `sample_judge.py`.  
Besides, autograding also used `sample_judge.py` for scoring.  
Thus, you should still test some complex cases by yourselves, maybe by `./read_server`, `./write_server` and `telnet`. Of course, if you can understand `sample_judge.py`, feel free to modify it.
