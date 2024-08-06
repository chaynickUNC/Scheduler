import os
import time
import random
import multiprocessing as mp
from statistics import mean
import pickle
import numpy as np
# from send import send

# In order to reserve cores, go into /media/boot/boot.ini and append "nohz_full=<list of cores comma separated> isolcpus=<list of cores comma separated>" to the setenv bootargs string
# For example: if I wanted to reserve cores 5,6 and 7, I would append "nohz_full=5,6,7 isolcpus=5,6,7" to the setenv bootargs string
# Make sure to reboot after changing boot.ini
# The code below assumes you have reserved cores 3-7


def matrix_multiply(dim):
    """Basic matrix multiplication stress test, returns the time in seconds it took to complete"""

    # could easily do this in a couple lines with numpy but installing numpy gave an error I didn't really want to spend time looking into so here we are
    t0 = time.time()

    V1 = []
    V2 = []
    for i in range(0, dim):
        v1 = []
        v2 = []
        for j in range(0, dim):
            v1.append(random.random())
            v2.append(random.random())
        V1.append(v1)
        V2.append(v2)
    Z = []

    for i in range(0, dim):
        z = []

        for j in range(0, dim):
            sum = 0
            for k in range(0, dim):
                sum += V1[i][k] * V2[k][j]
            z.append(sum)

        Z.append(z)   

    # V1 = np.random.random((dim, dim))
    # V2 = np.random.random((dim, dim))

    # Z = np.matmul(V1, V2)

    return time.time() - t0

def get_temps():
    """Reads temps of cores 4-7 on the A15 and returns them as an array"""

    temp4 = open(r"/sys/class/thermal/thermal_zone0/temp")
    temp5 = open(r"/sys/class/thermal/thermal_zone3/temp")
    temp6 = open(r"/sys/class/thermal/thermal_zone2/temp")
    temp7 = open(r"/sys/class/thermal/thermal_zone1/temp")

    temps = [int(temp4.readline())/1000, int(temp5.readline())/1000, int(temp6.readline())/1000, int(temp7.readline())/1000]

    temp4.close()
    temp5.close()
    temp6.close()
    temp7.close()

    return temps


def scheduler(mode="c"):
    """Runs processes concurrently (c) or sequentially (s)"""

    # puts the scheduler function on core 3
    os.system(f"taskset -p -c 3 {os.getpid()}")

    # creates four processes, each one a 512x512 matrix multiplicatin stress test
    p1 = mp.Process(target=matrix_multiply, args=(300, ))
    p2 = mp.Process(target=matrix_multiply, args=(300, ))
    p3 = mp.Process(target=matrix_multiply, args=(300, ))
    p4 = mp.Process(target=matrix_multiply, args=(300, ))

    # runs processes on all cores concurrently
    if mode == "c":

        t0 = time.time()

        p1.start()
        p2.start()
        p3.start()
        p4.start()

        os.system(f"taskset -p -c 4 {p1.pid}")
        os.system(f"taskset -p -c 5 {p2.pid}")
        os.system(f"taskset -p -c 6 {p3.pid}")
        os.system(f"taskset -p -c 7 {p4.pid}")

        log = []
        while p1.is_alive() and p2.is_alive() and p3.is_alive() and p4.is_alive():
            temps = get_temps()
            log.append((time.time() - t0, temps))

        p1.join()
        p2.join()
        p3.join()
        p4.join()

    # runs processes on core 4 one after the other
    if mode == "s":

        t0 = time.time()

        p1.start()
        os.system(f"taskset -p -c 4 {p1.pid}")
        p1.join()

        p2.start()
        os.system(f"taskset -p -c 4 {p2.pid}")
        p2.join()

        p3.start()
        os.system(f"taskset -p -c 4 {p3.pid}")
        p3.join()

        p4.start()
        os.system(f"taskset -p -c 4 {p4.pid}")
        p4.join()

        return time.time() - t0

    else: 
        print("Mode must be either \"c\" or \"s\"")
        exit()

    with open(f"{mode}_log.pickle", "wb") as file: pickle.dump(log, file)
    send(f"{mode}_log.pickle")


def hybrid(t_min, t_max, mode="single", dim=512, connected=False):

    os.system(f"taskset -p -c 3 {os.getpid()}")

    log = []
    p1 = mp.Process(target=matrix_multiply, args=(dim, ))
    p2 = mp.Process(target=matrix_multiply, args=(dim, ))
    p3 = mp.Process(target=matrix_multiply, args=(dim, ))
    p4 = mp.Process(target=matrix_multiply, args=(dim, ))

    t0 = time.time()
    
    p1.start()
    p2.start()
    p3.start()
    p4.start()

    hot_exec = False
    cool_exec = False
    while p1.is_alive() or p2.is_alive() or p3.is_alive() or p4.is_alive():
        temps = get_temps()
        max_temp = max(temps)
        min_temp = min(temps)
        avg_temp = mean(temps)
        log.append((time.time()-t0, temps))
        print(temps)
        
        if avg_temp <= t_min and not cool_exec:
            os.system(f"taskset -p -c 4 {p1.pid}")
            os.system(f"taskset -p -c 5 {p2.pid}")
            os.system(f"taskset -p -c 6 {p3.pid}")
            os.system(f"taskset -p -c 7 {p4.pid}")
            cool_exec = True
            hot_exec = False
        if max_temp >= t_max and not hot_exec:
            if mode == "single":
                cool_core = temps.index(min_temp)+4
                os.system(f"taskset -p -c {cool_core} {p1.pid}")
                os.system(f"taskset -p -c {cool_core} {p2.pid}")
                os.system(f"taskset -p -c {cool_core} {p3.pid}")
                os.system(f"taskset -p -c {cool_core} {p4.pid}")
            if mode == "double":
                cool_cores = sorted(range(len(temps)), key=lambda sub: temps[sub])[:2]
                os.system(f"taskset -p -c {cool_cores[0]+4} {p1.pid}")
                os.system(f"taskset -p -c {cool_cores[0]+4} {p2.pid}")
                os.system(f"taskset -p -c {cool_cores[1]+4} {p3.pid}")
                os.system(f"taskset -p -c {cool_cores[1]+4} {p4.pid}")
            hot_exec = True
            cool_exec = False

        time.sleep(0.04)
        os.system("clear")

    p1.join()
    p2.join()
    p3.join()
    p4.join()

    with open(f"h_{mode[0]}_{dim}_log.pickle", "wb") as file: pickle.dump(log, file)
    # if connected == True: send("h_log.pickle")


def main():

    hybrid(t_min=66, t_max=80, mode="double", dim=400)

main()