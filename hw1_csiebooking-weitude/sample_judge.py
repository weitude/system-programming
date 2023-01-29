import subprocess
import traceback
import telnetlib
import functools
import argparse
import socket
import typing
import copy
import time
import enum
import sys
import os


class TelnetClient(telnetlib.Telnet):
    def __init__(self, verbose, cid = 0, sid = (0, 0), *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.verbose = verbose
        self.client_idx = cid
        self.server_type = "Read server" if sid[0] == 0 else "Write server"
        self.server_idx = sid[1]
        if self.verbose:
            print(f"Client {self.client_idx} connects to {self.server_type.lower()} {self.server_idx}")

    def check_output(self, expected):
        msg = self.read_until(b"\n", timeout=0.2).decode()
        if msg[-1] != "\n":
            if self.verbose:
                print(f"\33[31m{self.server_type} {self.server_idx} respond to client {self.client_idx}: Timeout\33[0m")
            return False
        msg = msg.strip("\n")
        if self.verbose:
            if msg == expected:
                print(f"{self.server_type} {self.server_idx} send to client {self.client_idx}: " + msg)
            elif expected is not None:
                print(f"\33[31mClient {self.client_idx} expects \"{expected}\", but received \"{msg}\" from {self.server_type.lower()} {self.server_idx}\33[0m")
        return msg == expected if expected is not None else True
    
    def check_EOF(self):
        try:
            self.read_until(b"\n", timeout=0.2)
        except EOFError:
            return True
        else:
            return False

    def write(self, buffer):
        if self.verbose:
            print(f"Client {self.client_idx} send: " + buffer.decode().strip("\r\n"))
        super().write(buffer)


class Scorer:
    def __init__(self, dir_, student_id=None, port=10123, verbose=False):
        self.dir_ = dir_
        self.default_port = port
        self.punishment = 0
        self.verbose = verbose
        self.score = [0.5, 0.5, 0.25, 1.5, 1.5, 1.75, 1]
        self.record = {
            902001: [3, 0, 3], 902002: [2, 4, 1], 902003: [2, 4, 4], 902004: [4, 1, 1], 902005: [1, 2, 3], 
            902006: [0, 0, 0], 902007: [0, 3, 2], 902008: [2, 1, 2], 902009: [1, 2, 0], 902010: [0, 2, 1],
            902011: [2, 3, 1], 902012: [3, 2, 2], 902013: [1, 0, 3], 902014: [2, 1, 1], 902015: [3, 2, 3],
            902016: [1, 3, 0], 902017: [4, 0, 1], 902018: [1, 4, 4], 902019: [1, 2, 2], 902020: [3, 2, 1]
        }
        self.record_backup = copy.deepcopy(self.record)
        self.FOOD = 0
        self.CONCERT = 1
        self.ELECS = 2
        self.ID = 3
        self.COMMAND = 4
        self.EXIT = 5
        self.LOCKED = 6
        self.UPDATE = 7
        self.FAILED = 8
        self.EXCEED = 9
        self.NEGATIVE = 10
        self.strs = [
            "Food: {num} booked",
            "Concert: {num} booked",
            "Electronics: {num} booked",
            "Please enter your id (to check your booking state):",
            "Please input your booking command. (Food, Concert, Electronics. Positive/negative value increases/decreases the booking amount.):",
            "(Type Exit to leave...)",
            "Locked.",
            "Bookings for user {id} are updated, the new booking state is:",
            "[Error] Operation failed. Please try again.",
            "[Error] Sorry, but you cannot book more than 15 items in total.",
            "[Error] Sorry, but you cannot book less than 0 items."
        ]
        self.read_server = [None, None]
        self.write_server = [None, None]

    def bold(self, str_):
        return "\33[1m" + str_ + "\33[0m"

    def grey(self, str_):
        return "\33[2m" + str_ + "\33[0m"

    def underscore(self, str_):
        return "\33[4m" + str_ + "\33[0m"

    def red(self, str_):
        return "\33[31m" + str_ + "\33[0m"

    def bright_red(self, str_):
        return "\33[1;31m" + str_ + "\33[0m"

    def green(self, str_):
        return "\33[32m" + str_ + "\33[0m"
    
    def cyan(self, str_):
        return "\33[36m" + str_ + "\33[0m"
    
    def white(self, str_):
        return "\33[37m" + str_ + "\33[0m"

    def find_empty_port(self, start):
        for i in range(start, 65536):
            if not self.check_port_in_use(i):
                return i

    def check_port_in_use(self, port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            return s.connect_ex(("localhost", port)) == 0;

    def start_server(self, server_type, port):
        # server_type: either "read_server" or "write_server"
        server = subprocess.Popen([os.path.join(self.dir_, server_type), str(port)], stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL)
        # server = subprocess.Popen([os.path.join(self.dir_, server_type), str(port)])
        time.sleep(0.2)
        return server
    
    def execute_and_check_ret(self, cmd, exit_on_fail=True):
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        cmd_str = ' '.join(cmd)
        stdout, stderr = proc.communicate()
        if proc.returncode:
            print(self.red(f"Error when executing {' '.join(cmd)}"))
            print(self.red(f"stdout output: {stdout}"))
            print(self.red(f"stderr output: {stderr}"))
            if exit_on_fail:
                exit(1)
        return stdout, stderr

    def execute(self, cmd):
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = proc.communicate()
        return proc.returncode, stdout, stderr
    
    def read_record(self):
        rec = {}
        with open("./bookingRecord", "rb") as f:
            content = f.read()
            order = sys.byteorder
            for i in range(0, len(content), 16):
                id_ = int.from_bytes(content[i:i+4], order)
                food = int.from_bytes(content[i+4:i+8], order)
                concert = int.from_bytes(content[i+8:i+12], order)
                elecs = int.from_bytes(content[i+12:i+16], order)
                rec[id_] = [food, concert, elecs]
        return rec

    def print_record(self):
        for id_ in self.record:
            print(f"id: {id_}, Food: {self.record[id_][self.FOOD]}, Concert: {self.record[id_][self.CONCERT]}, Electronics: {self.record[id_][self.ELECS]}")

    def update_record(self, id_, food, concert, elecs):
        self.record[id_][self.FOOD] = food
        self.record[id_][self.CONCERT] = concert
        self.record[id_][self.ELECS] = elecs

    def restore_record(self):
        self.execute_and_check_ret(["cp", "bookingRecord.backup", "bookingRecord"])
        self.record = copy.deepcopy(self.record_backup)

    def file_exists(self, path):
        try:
            with open(path, "rb") as f:
                pass
        except OSError:
            return False
        else:
            return True

    def init_test(self):
        print(self.bold("Initializing for tests..."))

        print("Checking existence of read_server and write_server...")
        if self.file_exists("./read_server") or self.file_exists("./write_server"):
            self.punishment += 1
            print(self.red("Find read_server/write_server, lose 1 point.\nRemoving read_server and write_server..."))
            self.execute_and_check_ret(["rm", "-f", "read_server", "write_server"])

        print("Copying bookingState...")
        self.execute_and_check_ret(["cp", "bookingRecord", "bookingRecord.backup"])

        print("Testing command \"make clean\"...")
        self.execute_and_check_ret(["touch", "read_server", "write_server"])
        ret, stdout, stderr = self.execute(["make", "clean"])
        if ret:
            self.punishment += 0.25
            print(self.red(f"Command \"make clean\" failed, lose 0.25 point.\nstdout: {stdout}\nstderr: {stderr}"))
            self.execute_and_check_ret(["rm", "-f", "read_server", "write_server"])
        elif self.file_exists("./read_server") or self.file_exists("./write_server"):
            self.punishment += 0.25
            print(self.red("Command \"make clean\" did not remove files correctly, lose 0.25 point."))
            self.execute_and_check_ret(["rm", "-f", "read_server", "write_server"])

        print("Compiling source code...")
        ret, stdout, stderr = self.execute(["make"])
        if ret:
            print(self.bright_red(f"Make failed, 0 points.\nstdout: {stdout}\nstderr: {stderr}"))
            exit(1)
        elif not self.file_exists("./read_server") or not self.file_exists("./write_server"):
            print(self.bright_red("Make did not generate read_server and write_server, 0 points."))
            exit(1)
        print(self.green("Init test done!"))
    
    def check_state(self, client, id_, read_newline = True, state = None):
        passed = True
        for idx in [self.FOOD, self.CONCERT, self.ELECS]:
            if state is None:
                passed = passed and client.check_output(expected=self.strs[idx].format(num=self.record[id_][idx]))
            else:
                passed = passed and client.check_output(expected=self.strs[idx].format(num=state[idx]))
        if passed and read_newline:
            # read empty line
            client.check_output(expected=None)
        return passed
    
    def single_valid_read(self, port, id_):
        passed = True
        with TelnetClient(host="localhost", port=port, verbose=self.verbose) as client:
            passed = client.check_output(expected=self.strs[self.ID])
            if passed:
                payload = str(id_) + "\r\n"
                client.write(payload.encode())
                passed = self.check_state(client, id_) and client.check_output(expected=self.strs[self.EXIT])
            if passed:
                client.write(b"Exit\r\n")
                passed = client.check_EOF()
        return passed

    def single_valid_write(self, port, id_, cmd, case = "normal"):
        passed = True
        food, concert, elecs = 0, 0, 0
        with TelnetClient(host="localhost", port=port, verbose=self.verbose, sid=(1, 0)) as client:
            passed = client.check_output(expected=self.strs[self.ID])
            if passed:
                payload = str(id_) + "\r\n"
                client.write(payload.encode())
                food, concert, elecs = self.record[id_][self.FOOD], self.record[id_][self.CONCERT], self.record[id_][self.ELECS]
                passed = self.check_state(client, id_) and client.check_output(expected=self.strs[self.COMMAND])
            if passed:
                payload = " ".join([str(i) for i in cmd]) + "\r\n"
                client.write(payload.encode())
                if case == "upper":
                    passed = client.check_output(expected=self.strs[self.EXCEED])
                elif case == "lower":
                    passed = client.check_output(expected=self.strs[self.NEGATIVE])
                else:
                    food, concert, elecs = food + cmd[self.FOOD], concert + cmd[self.CONCERT], elecs + cmd[self.ELECS]
                    passed = client.check_output(expected=self.strs[self.UPDATE].format(id=id_)) and \
                             client.check_output(expected=self.strs[self.FOOD].format(num=food)) and \
                             client.check_output(expected=self.strs[self.CONCERT].format(num=concert)) and \
                             client.check_output(expected=self.strs[self.ELECS].format(num=elecs))
                    if passed:
                        self.update_record(id_, food, concert, elecs)
            if passed:
                passed = client.check_EOF()
        return passed

    def print_result(self, stat):
        for i in range(len(stat)):
            if stat[i]:
                print(self.green(f"Testdata {i+1}: Passed"))
            else:
                print(self.red(f"Testdata {i+1}: Failed"))

    def run_and_handle_exception(self, idx, task, f, *args, **kwargs):
        try:
            if self.verbose:
                print(self.cyan(f"Messages between client and server in testdata {idx}:"))
            passed = f(*args, **kwargs)
        except Exception as e:
            exc_type, exc_obj, exc_tb = sys.exc_info()
            print(self.bright_red(f"Testdata {idx}: Exception encountered"))
            print(self.bright_red(f"{traceback.format_exc()}"))
            self.clean()
            self.score[task] = 0
            raise e
        else:
            if passed:
                print(self.green(f"Testdata {idx}: Passed"))
                return 1
            else:
                print(self.red(f"Testdata {idx}: Failed"))
                self.score[task] = 0
                return 0

    def task_1_1(self):
        print(self.bold("=====Task 1-1: Read server====="))
        task = 0
        passed = []
        read_port = self.find_empty_port(start=self.default_port)
        self.read_server[0] = self.start_server("read_server", read_port)
        
        test_read_server = functools.partial(self.single_valid_read, read_port)
        
        try:
            passed.append(self.run_and_handle_exception(1, task, test_read_server, id_=902001))
        except Exception as e:
            return False
        else:
            self.clean()
            return False if not all(passed) else True


    def task_1_2(self):
        print(self.bold("=====Task 1-2: Write server====="))
        task = 1
        passed = []
        write_port = self.find_empty_port(start=self.default_port)
        self.write_server[0] = self.start_server("write_server", write_port)
        
        test_write_server = functools.partial(self.single_valid_write, write_port, id_=902002)

        try:
            passed.append(self.run_and_handle_exception(1, task, test_write_server, cmd=[0, 0, 0]))
            passed.append(self.run_and_handle_exception(2, task, test_write_server, cmd=[1, 1, -2], case="lower"))
        except Exception as e:
            return False
        else:
            self.clean()
            return False if not all(passed) else True
    
    def task_2(self):
        print(self.bold("=====Task 2: Handle valid and invalid requests on 1 server, 1 simultaneous connection====="))
        task = 2
        passed = []
        read_port = self.find_empty_port(start=self.default_port)
        self.read_server[0] = self.start_server("read_server", read_port)

        def invalid_id(port, id_, sid):
            passed = True
            with TelnetClient(host="localhost", port=port, verbose=self.verbose, sid=sid) as client:
                passed = client.check_output(expected=self.strs[self.ID])
                if passed:
                    payload = str(id_) + "\r\n"
                    client.write(payload.encode())
                    passed = client.check_output(expected=self.strs[self.FAILED]) and client.check_EOF()
            return passed

        def invalid_cmd(port, id_, cmd, sid):
            passed = True
            with TelnetClient(host="localhost", port=port, verbose=self.verbose, sid=sid) as client:
                client.check_output(expected=None)
                payload = str(id_) + "\r\n"
                client.write(payload.encode())
                for _ in range(5):
                    client.check_output(expected=None)
                payload = cmd + "\r\n"
                client.write(payload.encode())
                passed = client.check_output(expected=self.strs[self.FAILED]) and client.check_EOF()
            return passed

        test_invalid_id = functools.partial(invalid_id, read_port, sid=(0, 0))
        
        try:
            passed.append(self.run_and_handle_exception(1, task, test_invalid_id, id_=902))
            passed.append(self.run_and_handle_exception(2, task, test_invalid_id, id_=1000000))
        except Exception as e:
            return False
        else:
            self.clean()

        write_port = self.find_empty_port(start=self.default_port)
        self.write_server[0] = self.start_server("write_server", write_port)
        
        test_invalid_id = functools.partial(invalid_id, write_port, sid=(1, 0))
        test_invalid_cmd = functools.partial(invalid_cmd, write_port, sid=(1, 0))

        try:
            passed.append(self.run_and_handle_exception(3, task, test_invalid_cmd, id_=902014, cmd="1 abcde 0"))
            passed.append(self.run_and_handle_exception(4, task, test_invalid_cmd, id_=902014, cmd="1 -1 2-"))
        except Exception as e:
            return False
        else:
            self.clean()
            return False if not all(passed) else True

    def task_3(self):
        print(self.bold("=====Task 3, Handle valid requests on 1 server, multiple simultaneous connections====="))
        task = 3
        passed = []
        write_port = self.find_empty_port(start=self.default_port)
        self.write_server[0] = self.start_server("write_server", write_port)

        def test_3_1():
            passed = True
            with TelnetClient(host="localhost", port=write_port, verbose=self.verbose, timeout=0.2, sid=(1, 0), cid=0) as client1, \
                 TelnetClient(host="localhost", port=write_port, verbose=self.verbose, timeout=0.2, sid=(1, 0), cid=1) as client2:
                passed = client1.check_output(expected=self.strs[self.ID]) and client2.check_output(expected=self.strs[self.ID])
                if passed:
                    client2.write(b"902001\r\n")
                    time.sleep(0.2)
                    client1.write(b"902001\r\n")
                    passed = client1.check_output(expected=self.strs[self.LOCKED]) and client1.check_EOF() and \
                             self.check_state(client2, 902001) and client2.check_output(expected=self.strs[self.COMMAND])
                if passed:
                    client2.write(b"-1 2 -1\r\n")
                    passed = client2.check_output(expected=self.strs[self.UPDATE].format(id=902001)) and \
                             self.check_state(client2, 902001, read_newline=False, state=[2, 2, 2]) and \
                             client2.check_EOF()
            return passed
       
        test_1 = functools.partial(test_3_1)
        try:
            passed.append(self.run_and_handle_exception(1, task, test_1))
        except Exception as e:
            return False
        else:
            self.clean()
            return False if not all(passed) else True
                    
    def task_4(self):
        print(self.bold("=====Task 4, Handle valid requests on multiple servers, 1 simultaneous connection on each server====="))
        task = 4
        passed = []
        read_port, write_port = [], []
        write_port.append(self.find_empty_port(start=self.default_port))
        self.write_server[0] = self.start_server("write_server", port=write_port[0])
        write_port.append(self.find_empty_port(start=write_port[0]+1))
        self.write_server[1] = self.start_server("write_server", port=write_port[1])

        def test_4_1():
            passed = True
            with TelnetClient(host="localhost", port=write_port[0], verbose=self.verbose, timeout=0.2, sid=(1, 0), cid=0) as wr_client1, \
                 TelnetClient(host="localhost", port=write_port[1], verbose=self.verbose, timeout=0.2, sid=(1, 1), cid=1) as wr_client2:
                passed = wr_client1.check_output(expected=self.strs[self.ID]) and wr_client2.check_output(expected=self.strs[self.ID])
                if passed:
                    wr_client2.write(b"902001\r\n")
                    time.sleep(0.2)
                    wr_client1.write(b"902001\r\n")
                    passed = self.check_state(wr_client2, 902001) and wr_client2.check_output(expected=self.strs[self.COMMAND]) and \
                             wr_client1.check_output(expected=self.strs[self.LOCKED]) and wr_client1.check_EOF()
                if passed:
                    wr_client2.write(b"-1 2 -1\r\n")
                    passed = wr_client2.check_output(expected=self.strs[self.UPDATE].format(id=902001)) and \
                             self.check_state(wr_client2, 902001, read_newline=False, state=[2, 2, 2]) and \
                             wr_client2.check_EOF()
                return passed
        
        test_1 = functools.partial(test_4_1)

        try:
            passed.append(self.run_and_handle_exception(1, task, test_1))
        except Exception as e:
            return False
        else:
            self.clean()
            return False if not all(passed) else True

    def task_5_1(self):
        print(self.bold("=====Task 5-1, Client will not disconnect arbitrarily====="))
        print(self.underscore("This is not provided in sample judge!"))
        return True

    def task_5_2(self):
        print(self.bold("=====Task 5-2, Client may disconnect arbitrarily====="))
        print(self.underscore("This is not provided in sample judge!"))
        return True

    def clean(self):
        for i in range(2):
            if self.read_server[i] is not None:
                self.read_server[i].kill()
                self.read_server[i].wait()
                self.read_server[i] = None
            if self.write_server[i] is not None:
                self.write_server[i].kill()
                self.write_server[i].wait()
                self.write_server[i] = None
        self.restore_record()

    def print_score(self):
        total = sum(self.score) - self.punishment
        if total < 0:
            total = 0
        print(self.bold(f"Final score: {total}"))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", "--verbose", action="store_true", required=False, help="If set, message received from server will be printed.")
    parser.add_argument(
        "-t", "--task", nargs="+", required=False, 
        help="Specify which tasks to run. E.g, python judge.py -t 1_1 3 5_1 will only run task 1-1, task 3 and task 5-1."
    )
    args = parser.parse_args()
    scorer = Scorer(dir_=os.path.dirname(os.path.abspath(__file__)), student_id=None, verbose=args.verbose)
    scorer.init_test()
    assert scorer.read_record() == scorer.record, "Content of bookingRecord is not consistent with record of the class."
    print(scorer.bold("Start testing!"))
    # print(args.task)
    if args.task is None:
        print(scorer.bold("=====Task 1: Handle requests on 1 server, 1 simultaneous connection====="))
        scorer.task_1_1()
        scorer.task_1_2()
        scorer.task_2()
        scorer.task_3()
        scorer.task_4()
        print(scorer.bold("=====Task 5, Handle valid requests on multiple servers, multiple simultaneous connection====="))
        scorer.task_5_1()
        scorer.task_5_2()
        scorer.print_score()
    else:
        passed = []
        tasks = {
            "1_1": scorer.task_1_1, "1_2": scorer.task_1_2, "2": scorer.task_2, "3": scorer.task_3,
            "4": scorer.task_4, "5_1": scorer.task_5_1, "5_2": scorer.task_5_2
        }
        index = {"1_1": 0, "1_2": 1, "2": 2, "3": 3, "4": 4, "5_1": 5, "5_2": 6}
        if not all([t in tasks for t in args.task]):
            print(f"Error: task should be 1_1, 1_2, 2, 3, 4, 5_1 or 5_2.")
            exit(0)
        for key in tasks:
            if key in args.task:
                passed.append(tasks[key]())
            else:
                scorer.score[index[key]] = 0
        scorer.print_score()
        if not all(passed):
            exit(1)

if __name__ == '__main__':
    main()

