#!/usr/bin/python3

from os import system
from pathlib import Path
import time
import random
import hashlib

data_id = 0
data_size = 0
total_time = 0
max_size = 0
cur_seed = 0

MAX_SIZE_LIMIT = 1 << 30 # 1GB
SHOW_TIME = False # 不显示命令时间


def sha256(path):
    if Path(path).is_file():
        with open(path, 'rb') as f:
            sha256obj = hashlib.sha256()
            sha256obj.update(f.read())
            return sha256obj.hexdigest()
    else:
        sha256obj = hashlib.sha256()
        sha256obj.update(' '.join([sha256(x) for x in sorted(Path(path).iterdir())]).encode())
        return sha256obj.hexdigest()
        


def add_time(command):
    global total_time
    
    start = time.time()
    system(command)
    used = time.time() - start
    if SHOW_TIME:
        print(f'# `{command}` 用时 {used:.3f}s')
    total_time += used

write = lambda file_name, p: add_time(f'./evenodd write {file_name} {p}')
read = lambda file_name, save_as: add_time(f'./evenodd read {file_name} {save_as}')
repair = lambda idx: add_time(f'./evenodd repair {len(idx)} {" ".join(map(str, idx))}')

def gen(file_bytes, file_name, seed):
    global data_size, max_size
    data_size += file_bytes * 3
    max_size = max(max_size, data_size)
    if data_size >= MAX_SIZE_LIMIT:
        raise RuntimeError("data size is over the limit")
    system(f'./gendata {file_bytes} {file_name} {seed}')
    

primes = [x for x in range(3, 101) if [y for y in range(2, x) if x % y == 0] == []]

def reset():
    global data_id, data_size
    system('rm -r disk* testfile* savefile* 2> /dev/null')
    data_id = data_size = 0


def plain_rw_test(n, p):
    global data_id, cur_seed
    
    data_id += 1
    cur_seed += 1
    
    testfile = f'testfile/test{data_id}'
    savefile = f'savefile/save{data_id}'
    
    print(f'# 测试 {data_id}：n = {n}, p = {p}, seed = {cur_seed}')
    gen(n, testfile, cur_seed)
    write(testfile, p)
    read(testfile, savefile)
    return_code = system(f'diff -q {testfile} {savefile}')
    if return_code == 0:
        print('# 测试通过')
    else:
        print(f'# 测试不通过，diff 返回值为 {return_code}')
        exit(-1)

def broken_rw_test(n, p, idx, broke_type):
    global data_id, cur_seed
    
    data_id += 1
    cur_seed += 1
    
    testfile = f'testfile/test{data_id}'
    savefile = f'savefile/save{data_id}'

    print(f'# 测试 {data_id}：n = {n}, p = {p}, idx = {idx}, seed = {cur_seed}')
    gen(n, testfile, cur_seed)
    write(testfile, p)

    if broke_type:
        for x in idx:
            system(f'rm disk{x}/testfile/test{data_id}')
    else:
        for x in idx:
            system(f'rm -r disk{x}')

    read(testfile, savefile)
    return_code = system(f'diff -q {testfile} {savefile}')
    if return_code == 0:
        print('# 测试通过')
    else:
        print(f'# 测试不通过，diff 返回值为 {return_code}')
        exit(-1)

repair_test_cnt = 0
def repair_test(size, n, p, idx):
    global cur_seed, repair_test_cnt

    reset()

    repair_test_cnt += 1

    print(f'# 测试 {repair_test_cnt}：size = {size}, n = {n}, p = {p}, idx = {idx}, seed = {cur_seed}')

    for i in range(1, n + 1):
        cur_seed += 1
        testfile = f'testfile/test{i}'
        gen(size, testfile, cur_seed)
        write(testfile, p[i - 1])

    hashes = []
    for x in idx:
        hashes.append(sha256(f'disk{x}'))
        system(f'rm -r disk{x}')

    repair(idx)

    for i in range(len(idx)):
        if sha256(f'disk{idx[i]}') != hashes[i]:
            print(f'# 测试不通过，disk{idx[i]} 未正确修复')
            exit(-1)
    print(f'# 测试通过')
    reset()

huge_test_cnt = 0
def huge_test(n, p):
    global cur_seed, huge_test_cnt, SHOW_TIME

    SHOW_TIME = True

    cur_seed += 1
    huge_test_cnt += 1
    
    reset()

    testfile = f'testfile/test1'
    savefile = f'savefile/save1'

    print(f'# 测试 {huge_test_cnt}：n = {n}, p = {p}, seed = {cur_seed}')
    gen(n, testfile, cur_seed)
    write(testfile, p)

    system(f'rm disk{p}/{testfile}')
    system(f'rm disk{p + 1}/{testfile}')

    repair([p, p + 1])
    read(testfile, savefile)
    return_code = system(f'diff -q {testfile} {savefile}')
    if return_code == 0:
        print('# 测试通过')
    else:
        print(f'# 测试不通过，diff 返回值为 {return_code}')
        exit(-1)
    reset()
    SHOW_TIME = False


def subtask_plain_rw():
    reset()

    print('# 测试：无损 read/write')

    for p in primes[::2]:
        for i in range(2, 8):
            plain_rw_test(10 ** i, p)
    reset()
    print()

def subtask_broken_rw():
    reset()

    print('# 测试：带损 read/write')
    p = 11
    broken_rw_test(400, p, [0], False)
    broken_rw_test(400, p, [1], True)
    broken_rw_test(400, p, [0, 1], False)
    broken_rw_test(400, p, [2, 4], True)
 
    broken_rw_test(400, p, [p], False)
    broken_rw_test(400, p, [p + 1], True)
    broken_rw_test(400, p, [p, p + 1], False)
    broken_rw_test(400, p, [3, p], True)
    broken_rw_test(400, p, [0, p + 1], False)

    reset()

    for n in [10**5, 10**6, 10**7]:
        for p in [3, 5, 83, 89]:
            broken_rw_test(n, p, [p], cur_seed & 1)
            broken_rw_test(n, p, [p, p + 1], cur_seed & 1)
            broken_rw_test(n, p, [p + 1], cur_seed & 1)
            broken_rw_test(n, p, [0, p + 1], cur_seed & 1)
            broken_rw_test(n, p, [0, p], cur_seed & 1)
            broken_rw_test(n, p, [0, 1], cur_seed & 1)
            broken_rw_test(n, p, [0], cur_seed & 1)
            reset()
    print()


def subtask_repair():
    print('# 测试：repair')
    for size in [10**4, 10**5, 10**6, 10 ** 7]:
        n = min(20, 10 ** 7 // size)
        repair_test(size, n, random.choices(primes, k=n), [0, 1])
        repair_test(size, n, random.choices(primes, k=n), [0])
        p = random.choice(primes)
        repair_test(size, n, [p] * n, [p, p + 1])
        repair_test(size, n, [p] * n, [p])
        repair_test(size, n, [p] * n, [p + 1])
    print()

def subtask_huge():
    print('# 测试：大数据 write / repair / read')
    n = 10 ** 8
    for p in primes:
        huge_test(n, p)
    print()


if __name__ == '__main__':
    random.seed(0)

    subtask_plain_rw()
    subtask_broken_rw()
    subtask_repair()
    subtask_huge()

print(f'总用时：{total_time:.3f}s')
print(f'瞬时最大占用磁盘空间（预计）：{(max_size / 1048576):.3f}MB')