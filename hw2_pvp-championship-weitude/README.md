# SP_HW2_release

**[SPEC Link](https://hackmd.io/@UTGhost/H1Nk8CpMi)**

**[Video Link](https://youtu.be/mNVUvgWQezA?t=0)**

**[Discussion Space Link](https://github.com/NTU-SP/SP_HW2_release/discussions)**

## How to exec sample execution
In order to testing your player with TA's battle or testing your battle with TA's player, we provide the compiled executable file for battle.c and player.c.

Put the **player_status.txt**, **battle** and **player** in your current working dir. After that, you can run TA’s battle and player with following command:  
`$ ./battle A 0`  
`A = root's [battle_id] `  
`0 = root's [parent_pid]`  
**Remember to remove all player[K].fifo(8<=K<=14) files before you execute** `$ ./battle A 0` **otherwise mkfifo() will return error**.  
It will generate 8 + 14 log files in your current working directory and output a  champion message from battle A’s stdout at the end of the championship.
Note that it does not mean you can get full points by successfully running with these two programs. We will check if you strictly follow the steps by checking the log files step by step.
