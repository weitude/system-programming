# csieBooking

+ file locking
+ select multiplexing
+ socket communication

---

## Description

The csieBooking system is comprised of read servers and write servers.
Both can access a file `bookingRecord` that records information about users’ bookings.
When a server gets a request from a client, it will respond according to the file’s content.
A read server tells the client how many items it has booked.
A write server can modify the file to book more or less items.

## Read Server

A client can read an user’s booking state.
Once it connects to a read server, the terminal will show the following:

```
Please enter your id (to check your booking state):
```

If the client types an valid id (for example, 902001 ) on the client side, the server shall reply:

```
Food: 1 booked
Concert: 3 booked
Electronics: 0 booked

(Type Exit to leave...)
```

Now, if the client types Exit, server should close the connection from the client
(If typing other input, ignore it).

Booking state of the same id could be read simultaneously.
However, if someone else is writing the booking state of the same id, the server shall reply:

```
Locked.
```

and close the connection from client.

## Write Server

Once the client connects to a write server, the terminal will show the following

```
Please enter your id (to check your booking state):
```

If you type an valid id (for example, 902001 ) on the client side,
the server shall reply the booking state of the id, following by a prompt:

```
Food: 1 booked
Concert: 3 booked
Electronics: 0 booked

Please input your booking command. (Food, Concert, Electronics. Positive/negative value increases/decreases the booking amount.):
```

A valid booking command consists of 3 integers, separated by a single space.
If the client types a command (for example, `3 -2 4`),
the server shall modify the `bookingRecord` file and reply:

```
Bookings for user 902001 are updated, the new booking state is:
Food: 4 booked
Concert: 1 booked
Electronics: 4 booked
```

If someone else is reading/writing the booking state of the same id, the server shall reply:

```
Locked.
```

and close the connection from client.

## Sample Judge

You can test this program with a single command `python sample_judge.py`.

### Arguments

`sample_judge.py` accepts following optional arguments:

+ `-v`, `--verbose`: verbose. if you set this argument, `sample_judge.py` will print messages delivered between server and client, which may help debugging.
+ `-t TASK [TASK ...]`, `--task TASK [TASK ...]`, Specify which tasks you want to run. If you didn't set this argument, `sample_judge.py` will run all tasks by default.
  + Valid `TASK` are `["1_1", "1_2", "2", "3", "4", "5_1", "5_2"]`.

E.g, `python sample_judge.py -t 1_1 3 5_1 -v` set `sample_judge.py` to be verbose, and runs only task 1-1, task 3 and task 5-1.