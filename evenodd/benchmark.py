#!/usr/bin/python3
import matplotlib.pyplot as plt
import time
from os import system
import datetime
from pathlib import Path


T = 5
N = 200 * 1024 * 1024


def timing(command):
    cur = time.time()
    system(command)
    return time.time() - cur


def gen(file_bytes, file_name, seed):
    system(f'./gendata {file_bytes} {file_name} {seed}')


def write(file_name, p):
    x = 0
    for t in range(T):
        x += timing(f'./evenodd write {file_name} {p}')
    x /= T
    print(f'# write: {x:.3f}s')
    return x


def read(file_name, save_as):
    x = 0
    for t in range(T):
        x += timing(f'./evenodd read {file_name} {save_as}')
    x /= T
    print(f'# read: {x:.3f}s')
    return x


def repair(idx):
    x = 0
    for t in range(T):
        for i in idx:
            system(f'rm -r disk_{i}')
        x += timing(f'./evenodd repair {len(idx)} {" ".join(map(str, idx))}')
    x /= T
    print(f'# repair: {x:.3f}s')
    return x



primes = [x for x in range(3, 101) if [
    y for y in range(2, x) if x % y == 0] == []]

x = primes


if __name__ == '__main__':
    system('mkdir benchmark')

    n = N
    gen(n, 'tmptest', 0)

    y1 = []
    y2 = []
    y3 = []

    for p in x:
        print(f'# 测试 {p}')
        y1.append(write('tmptest', p))
        y2.append(repair([p, p + 1]))
        y3.append(read('tmptest', '/dev/null'))

    system('rm -r disk* tmptest')

    avg_time = (sum(y1) + sum(y2) + sum(y3)) / (3 * len(primes))

    info = f'''Number of primes: {len(primes)}
T: {T}
Size: {(N / 1024 / 1024):.2f} MB
Avg time: {(avg_time * 1000):.6f} ms
Avg speed: {(avg_time / (N / 1024 / 1024) * 1000):.6f} ms/MB'''

    print(info)
    with open(f'benchmark/{datetime.datetime.now().isoformat()}.log', 'w') as f:
        f.write(info)

    plt.plot(x, y1, label='write')
    plt.plot(x, y2, label='repair')
    plt.plot(x, y3, label='read')
    plt.legend()
    plt.savefig(f'benchmark/{datetime.datetime.now().isoformat()}.png')
    plt.show()

