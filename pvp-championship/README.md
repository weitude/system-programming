# PvP Championship

## Tech Stack

+ pid, fork
+ pipe / fifo
+ dup2
+ execl
+ wait

## Description

![Championship Graph](https://i.imgur.com/ecBJOow.png)

There are 8 players (P0 - P7) in this championship.
I use a simpler Double Elimination Tournament for this championship.
Each battle has only two players, so there are 14 battles(A - N) in total to generate a champion.

For battles in Zone A, players who lose in the battle still have a chance to be the champion.
However, players who lose in the battle in Zone B are eliminated.

## Player Status Structured Message (PSSM)

A structure to pass between processes.

```C
typedef enum {
    FIRE,
    GRASS,
    WATER
} Attribute;

typedef struct {
    int real_player_id; // ID of the Real Player, an integer from 0 - 7
    int HP; // Healthy Point
    int ATK; // Attack Power
    Attribure attr; // Player's attribute
    char current_battle_id; //current battle ID
    int battle_ended_flag; // a flag for validate if the battle is the last round or not, 1 for yes, 0 for no
} Status;
```

## battle.c

+ There will be 14 battles (from A to N) in the game,
  a battle starts whenever two players both send the first PSSM to the battle,
  so different battles may run parallelly.

+ For non-root battles (B - N), it would read from its parent via stdin, and write to its parent via stdout.

+ For the root battle (A), it would print the Champion Message via stdout when the battle ends.

+ For battles (A - N), it would communicate with its two children via pipes.
  Two pipes should be created for each child.
  One for writing to the child. The other for reading from the child.
  So a battle will create four pipes in total.

+ Four modes in a battle’s lifecycle:
    + Init Mode
        + In this mode, battle creates its log file, create pipes, fork children…, etc. Then it enters Waiting Mode.
    + Waiting Mode
        + After the battle initializes itself, it enters Waiting Mode.
          It is blocked by the pipes until it receives PSSMs from both children.
    + Playing Mode
        + In Playing Mode, the battle receives children’s PSSM, updates them,
          then sends them back to each children repeatedly until one of the player’s HP ≤ 0.
    + Passing Mode
        + In Passing Mode, the battle is responsible for passing PSSM between its parent and child.
          The passing mode ends when all its descendant players’ HP ≤ 0.
          After that, the battle should terminate itself immediately.

## player.c

+ For each player, it should read from its parent via stdin, and write to its parent via stdout.
+ Two types of players:
    + Real Player (P0 - P7)
        + It reads initial status from **player_status.txt** and join battles first.
          When the Real Player lose in a battle,
          it will transfer its PSSM to the corresponding Agent Player via fifo.
    + Agent Player (P8 - P14)
        + It waits for a PSSM via fifo in the beginning.
          After receiving loser’s PSSM from the corresponding battle, the Agent Player then works like a Real Player.

## sample execution

Put the **player_status.txt**, **battle** and **player** in your current working dir.
After that, you can run battle and player with following command:

```
./battle A 0
```

`A = root's [battle_id]`

`0 = root's [parent_pid]`