#include <assert.h>
#include <dirent.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define __AVX__ 1
#define __AVX2__ 1
#define __SSE__ 1
#define __SSE2__ 1
#define __SSE2_MATH__ 1
#define __SSE3__ 1
#define __SSE4_1__ 1
#define __SSE4_2__ 1
#define __SSE_MATH__ 1
#define __SSSE3__ 1

#pragma GCC target(                                                            \
    "sse,sse2,sse3,ssse3,sse4.1,sse4.2,avx,avx2,popcnt,tune=native")

typedef __uint128_t uint128;
typedef unsigned long long uint64;

long long min64(long long x, long long y) { return x < y ? x : y; }

#define mod_p(x) (((x) < 0) ? ((x) + p) : (x))
#define at(a, i) a[i >> 2][i & 3]

struct stat get_file_stat(const char *file_name) {
  struct stat file_stat;
  stat(file_name, &file_stat);
  return file_stat;
}

/**
 * @brief 创建二进制文件 file_name。
 * 会一并创造其祖先文件夹（如果不存在）。
 * @param file_name 文件名
 * @return NULL
 * @example file_create("disk_3/subdir/something");
 */
void file_create(const char *file_name) {
  int n = strlen(file_name);
  char str[n + 1];
  struct stat tmp_stat;

  for (int i = 0; i < n; i++) {
    str[i] = file_name[i];
    if (str[i] == '/') {
      str[i] = '\0';
      if (access(str, 0) == -1)
        mkdir(str, 0777);
      str[i] = '/';
    }
  }
  str[n] = '\0';
  fclose(fopen(str, "wb"));
}

const int MAX_IO_BUFFER_SIZE_SUM =
    1 << 28; // 函数内 IO 缓存区大小最大字节数（不严格），防止空间过大
const int MAX_PER_IO_BUFFER_SIZE =
    1 << 20; // 单个 IO 缓存区大小最大字节数，防止缓存过大影响速度
const int MAX_FILE_NAME_LENGTH = 260; // 文件名的最大长度
const int MAX_P = 100;                // p 的最大值
/**
 * @brief 用于进行二进制文件输入的结构体（带缓存区）。
 *
 * @example
 * // 新建一个 Input 类型结构体
 * struct Input input;
 *
 * // 初始化 input，缓存字节数为 1000，从文件 "testfile" 里读入数据
 * init_input(input, 1000, "testfile");
 *
 * // 从输入文件里读入 35 个 bool 并输出
 * // 当输入文件字节数不够时，会令读入的 bool 为 0
 * uint128 x = read_bits(input, 35);
 * printf("%u", (unsigned)x);
 *
 * // 销毁 input，释放空间并关闭文件
 * del_input(input);
 */
struct Input {
  uint64 *st, *ed, *p;
  FILE *file;
  int remain;
};

/**
 * @brief 初始化 Input。
 * 为 (*buffer) 申请 size 字节的空间，设置其输入文件名为 file_name。
 * @param buffer 指向 Input 的指针
 * @param size 申请字节大小（会自动变为 ceil(size / 8) * 8）
 * @param file_name 文件名
 * @return NULL
 */
void init_input(struct Input *buffer, long long size, int n,
                const char *file_name) {
  size = min64(size, get_file_stat(file_name).st_size);
  size = min64(size, MAX_PER_IO_BUFFER_SIZE);
  size = ((size >> 3) / n + 1) * n;
  buffer->st = (uint64 *)malloc(size << 3);
  buffer->ed = buffer->st + size;
  buffer->p = buffer->ed;
  buffer->file = fopen(file_name, "rb");
}

/**
 * @brief 从文件读入足量字符填充缓存区。需要保证缓存区已全部用尽。
 * 一般不需要手动调用此函数，当缓存区用完时会自动调用。
 * @param buffer 指向 Input 的指针
 * @return NULL
 */
void flush_input(struct Input *buffer) {
  assert(buffer->p == buffer->ed);
  memset(buffer->st, 0, (buffer->ed - buffer->st) << 3);
  fread(buffer->st, 1, (buffer->ed - buffer->st) << 3, buffer->file);
  buffer->p = buffer->st;
}

/**
 * @brief 销毁 Input。
 * @param buffer 指向 Input 的指针
 * @return NULL
 */
void del_input(struct Input *buffer) {
  fclose(buffer->file);
  free(buffer->st);
  buffer->st = buffer->ed = buffer->p = NULL;
  buffer->file = NULL;
}

uint64 read_uint64_direct(struct Input *buffer) {
  uint64 x;
  fread(&x, 1, 8, buffer->file);
  return x;
}
uint64 read_uint64_unsafe(struct Input *buffer) { return *(buffer->p++); }
void read_array_unsafe(void *a, struct Input *buffer, int n) {
  memcpy(a, buffer->p, n << 3);
  buffer->p += n;
}

/**
 * @brief 用于进行二进制文件输出的结构体（带缓存区）。
 *
 * @example
 * // 新建一个 Output 类型结构体
 * struct Output output;
 *
 * // 初始化 output，缓存字节数为 1000，向文件 "testfile" 写入数据
 * init_output(output, 1000, "testfile");
 *
 * // 向输出文件写入 (1101)_2
 * // 实际上会先写入缓存区，缓存区满时再将缓存区一次性写入文件
 * write_bits(output, 0b1101, 4);
 *
 * // 销毁 output（输出缓存区，释放空间并关闭文件）
 * del_input(output);
 */
struct Output {
  uint64 *st, *ed, *p;
  FILE *file;
};

/**
 * @brief 初始化 Output。
 * 为 Output 申请 size 字节大小的空间，设置输出文件名为 file_name。
 * @param buffer 指向 Output 的指针
 * @param size 申请字节大小
 * @param file_name 文件名
 * @return NULL
 */
void init_output(struct Output *buffer, long long size, int n,
                 const char *file_name) {
  size = min64(size, MAX_PER_IO_BUFFER_SIZE);
  size = ((size >> 3) / n + 1) * n;
  buffer->st = (uint64 *)malloc(size << 3);
  buffer->ed = buffer->st + size;
  buffer->p = buffer->st;

  file_create(file_name);
  buffer->file = fopen(file_name, "wb");
}

/**
 * @brief 输出 Output 缓存区内的内容并清空。
 * 若 bool 数不为 8 的倍数，则在末尾补零直至 bool 数为 8 的倍数。
 * @param buffer 指向 Output 的指针
 * @return NULL
 */
void flush_output(struct Output *buffer) {
  fwrite(buffer->st, 1, (buffer->p - buffer->st) << 3, buffer->file);
  buffer->p = buffer->st;
}

/**
 * @brief 销毁 Output（会自动输出缓存区）。
 * @param buffer 指向 Output 的指针
 * @return NULL
 */
void del_output(struct Output *buffer) {
  flush_output(buffer);
  fclose(buffer->file);
  free(buffer->st);
  buffer->st = buffer->ed = buffer->p = NULL;
  buffer->file = NULL;
}

void write_uint64_direct(struct Output *buffer, uint64 x) {
  fwrite(&x, 1, 8, buffer->file);
}
void write_bytes_direct(struct Output *buffer, uint64 x, int n) {
  while (n--) {
    fputc(x & 255, buffer->file);
    x >>= 8;
  }
}
void write_array_unsafe(struct Output *buffer, void *a, int n) {
  memcpy(buffer->p, a, n << 3);
  buffer->p += n;
}

void get_info(const char *file_path, long long *file_size, int *p) {
  FILE *file;
  uint64 x;

  file = fopen(file_path, "rb");
  fread(&x, 1, 8, file);
  *file_size = x >> 8;
  *p = x & 255;
  fclose(file);
}

#define CALC_DIAG(res)                                                         \
  {                                                                            \
    __m256i _b1[4][2 * len4];                                                  \
    memset(_b1, 0, sizeof(_b1));                                               \
    memset(res, 0, sizeof(res));                                               \
    for (int _i = 0; _i < p; _i++) {                                           \
      __m256i *ptr = &_b1[_i & 3][_i >> 2];                                    \
      for (int _j = 0; _j < len4; _j++, ptr++)                                 \
        (*ptr) = _mm256_xor_si256(*ptr, a[_i][_j]);                            \
    }                                                                          \
    uint64 *_b064 = _b1[0];                                                    \
    for (int _k = 1; _k < 4; _k++) {                                           \
      uint64 *_b64 = _b1[_k];                                                  \
      for (int _i = _k; _i < 2 * (p - 1); _i++)                                \
        _b064[_i] ^= _b64[_i - _k];                                            \
    }                                                                          \
    uint64 *_res64 = res;                                                      \
    for (int _i = 0; _i <= p - 1; _i++)                                        \
      _res64[_i] = _b064[_i] ^ _b064[_i + p];                                  \
  }
#define CALC_PP1(res)                                                          \
  {                                                                            \
    CALC_DIAG(res)                                                             \
    uint64 *_res64 = res;                                                      \
    for (int _i = 0; _i <= p - 1; _i++)                                         \
      _res64[_i] ^= _res64[p - 1];                                             \
  }

#define CALC_P(res)                                                            \
  {                                                                            \
    memset(res, 0, sizeof(res));                                               \
    for (int _i = 0; _i < p; _i++)                                             \
      for (int _j = 0; _j < len4; _j++)                                        \
        res[_j] = _mm256_xor_si256(res[_j], a[_i][_j]);                        \
  }
#define CALC_I(res)                                                            \
  {                                                                            \
    memset(res, 0, sizeof(res));                                               \
    for (int _i = 0; _i < p + 1; _i++)                                         \
      if (check_disk[_i])                                                      \
        for (int _j = 0; _j < len4; _j++)                                      \
          res[_j] = _mm256_xor_si256(res[_j], a[_i][_j]);                      \
  }

/**
 * @brief 修复文件名为 file_name 的数据。
 * @param file_name 需要修复的文件名
 * @param size 该文件原来的大小
 * @param p 该文件对应的加密质数 p
 * @param content_only 若为 true，则当损坏加密数据数不超过 2 且
 * 编号为 0 ... (p - 1) 的加密数据完好时直接退出，不进行修复
 * @return 损坏加密数据数是否不超过 2
 * @example repair_work("testfile", size, 5, false);
 */
bool repair_work(const char *file_name, const long long size, const int p,
                 bool content_only) {
  int number_erasures = 0;
  int idx[2], ok_id = 0;
  char disk_file_path[MAX_FILE_NAME_LENGTH];

  bool check_disk[p + 2]; // 为 true 表示完好，为 false 表示损坏
  for (int i = 0; i < p + 2; i++) {
    sprintf(disk_file_path, "disk_%d/%s", i, file_name);
    check_disk[i] = (access(disk_file_path, 0) == 0);
    if (!check_disk[i]) {
      if (number_erasures == 2)
        return false;
      idx[number_erasures] = i;
      number_erasures++;
    } else
      ok_id = i;
  }

  if (number_erasures == 0 || (content_only && idx[0] >= p))
    return true;

  struct Input input[p + 2];
  struct Output output[number_erasures];
  const int len4 = (p + 3) / 4;
  __m256i a[p + 2][len4]; // 0 ... (p + 1) 列的数据
  char disk_file_name[MAX_FILE_NAME_LENGTH];
  int now_output_id = 0;

  for (int i = 0; i < p + 2; i++) {
    sprintf(disk_file_name, "disk_%d/%s", i, file_name);
    if (check_disk[i]) {
      init_input(&input[i], MAX_IO_BUFFER_SIZE_SUM / (p + 2), p - 1,
                 disk_file_name);
      read_uint64_direct(&input[i]);
      flush_input(&input[i]);
    } else {
      init_output(&output[now_output_id],
                  min64(MAX_IO_BUFFER_SIZE_SUM / 2, size / p), p - 1,
                  disk_file_name);
      write_uint64_direct(&output[now_output_id], size << 8 | p);
      now_output_id++;
    }
  }

  for (long long t = 0; t < (size << 3); t += 64 * (p - 1) * p) {
    if (input[ok_id].p == input[ok_id].ed)
      for (int i = 0; i < p + 2; i++)
        if (check_disk[i])
          flush_input(&input[i]);

    memset(a, 0, sizeof(a));
    for (int i = 0; i < p + 2; i++)
      if (check_disk[i])
        read_array_unsafe(a[i], &input[i], p - 1);

    if (number_erasures == 1) { // 1 个文件损坏
      if (idx[0] == p)
        CALC_P(a[p])
      else if (idx[0] == p + 1)
        CALC_PP1(a[p + 1])
      else
        CALC_I(a[idx[0]])
      if (output[0].p == output[0].ed)
        flush_output(&output[0]);
      write_array_unsafe(&output[0], a[idx[0]], p - 1);

    } else { // 2 个文件损坏
      const int disk_i = idx[0], disk_j = idx[1];
      if (disk_i == p && disk_j == p + 1) { // 相当于重新加密
        CALC_P(a[p])
        CALC_PP1(a[p + 1])
      } else if (disk_i < p && disk_j == p) {
        __m256i S[len4]; // 对角线的 xor
        uint64 t;
        CALC_DIAG(S)

        for (int l = 0; l < len4; l++)
          S[l] = _mm256_xor_si256(S[l], a[p + 1][l]);

        uint64 *S64 = S, *ai64 = a[disk_i];
        t = S64[mod_p(disk_i - 1)];
        for (int k = 0; k < p - 1; k++)
          ai64[k] = S64[mod_p(disk_i + k - p)] ^ t;
        CALC_P(a[p])
      } else if (disk_i < p &&
                 disk_j == p + 1) { // 由 a[p] 可以修复 disk_i 然后再求解 a[p+1]
        CALC_I(a[disk_i])
        CALC_PP1(a[p + 1])
      } else if (disk_i < p && disk_j < p) {
        __m256i S0[len4], S1[len4], Sb = _mm256_set1_epi32(0);

        CALC_P(S0)
        for (int l = 0; l < len4; l++) {
          S0[l] = _mm256_xor_si256(S0[l], a[p][l]);
          Sb = _mm256_xor_si256(Sb, a[p][l]);
          Sb = _mm256_xor_si256(Sb, a[p + 1][l]);
        }
        Sb = _mm256_set1_epi64x(Sb[0] ^ Sb[1] ^ Sb[2] ^ Sb[3]);

        CALC_DIAG(S1)
        for (int l = 0; l < len4; l++)
          S1[l] = _mm256_xor_si256(S1[l], _mm256_xor_si256(Sb, a[p + 1][l]));

        const int ij = mod_p(disk_i - disk_j), ji = mod_p(disk_j - disk_i);
        int s = mod_p(ij - 1);

        uint64 *aj64 = a[disk_j], *ai64 = a[disk_i], *S064 = S0, *S164 = S1;

        do {
          aj64[s] = S164[mod_p(disk_j + s - p)] ^ ai64[mod_p(s - ij)];
          ai64[s] = S064[s] ^ aj64[s];
          s = mod_p(s - ji);
        } while (s != p - 1);
      }
      if (output[0].p == output[0].ed) {
        flush_output(&output[0]);
        flush_output(&output[1]);
      }
      write_array_unsafe(&output[0], a[disk_i], p - 1);
      write_array_unsafe(&output[1], a[disk_j], p - 1);
    }
  }

  for (int i = 0; i < p + 2; i++)
    if (check_disk[i])
      del_input(&input[i]);
  for (int i = 0; i < number_erasures; i++)
    del_output(&output[i]);
  return true;
}

/**
 * @brief 读入文件 file_name，经 EVENODD 加密后储存。
 * 从文件 file_name 读入数据并编码，然后将 p + 2 个数据块储存在
 * "disk_0", "disk_1", ..., "disk_{p + 1}" 文件夹下。
 * @param file_name 文件名，长度不超过 100
 * @param p 用于 EVENODD 加密的质数，应当为不超过 100 的整数
 * @return NULL
 * @example write("testfile", 5);
 */
void write(const char *file_name, const int p) {
  struct Input input;
  struct Output output[p + 2];
  long long file_size;
  const int len4 = (p + 3) / 4;
  __m256i a[p + 2][len4];

  file_size = get_file_stat(file_name).st_size;

  init_input(&input, file_size, p * (p - 1), file_name);
  flush_input(&input);

  for (int i = 0; i < p + 2; i++) {
    char disk_file_name[MAX_FILE_NAME_LENGTH];

    sprintf(disk_file_name, "disk_%d/%s", i, file_name);
    init_output(&output[i],
                min64(MAX_IO_BUFFER_SIZE_SUM / (p + 2), file_size / p), p - 1,
                disk_file_name);

    // 先将文件大小和 p 的值输出
    write_uint64_direct(&output[i], file_size << 8 | p);
  }

  // 开始加密
  for (long long t = 0; t < (file_size << 3); t += 64 * (p - 1) * p) {
    memset(a, 0, sizeof(a));
    if (input.p == input.ed)
      flush_input(&input);
    for (int i = 0; i < p; i++)
      read_array_unsafe(a[i], &input, p - 1);

    CALC_P(a[p])
    CALC_PP1(a[p + 1])

    if (output[0].p == output[0].ed)
      for (int i = 0; i < p + 2; i++)
        flush_output(&output[i]);
    for (int i = 0; i < p + 2; i++)
      write_array_unsafe(&output[i], a[i], p - 1);
  }

  del_input(&input);
  for (int i = 0; i < p + 2; i++)
    del_output(&output[i]);
}

void read(const char *file_name, const char *save_as) {
  long long file_size;
  int p;
  struct Output output;
  char disk_file_path[MAX_FILE_NAME_LENGTH];
  int disk_id;
  int r, c;

  disk_id = -1;
  do {
    disk_id++;
    sprintf(disk_file_path, "disk_%d/%s", disk_id, file_name);
  } while (disk_id < MAX_P + 1 && access(disk_file_path, 0) == -1);
  if (access(disk_file_path, 0) == -1) {
    printf("File does not exist!\n");
    return;
  } else if (disk_id >= 3) {
    printf("File corrupted!\n");
    return;
  }

  get_info(disk_file_path, &file_size, &p);

  if (!repair_work(file_name, file_size, p, true)) {
    printf("File corrupted!\n");
    return;
  }

  struct Input input[p];

  for (int i = 0; i < p; i++) {
    sprintf(disk_file_path, "disk_%d/%s", i, file_name);
    init_input(&input[i], MAX_IO_BUFFER_SIZE_SUM / p, p - 1, disk_file_path);
    read_uint64_direct(&input[i]);
    flush_input(&input[i]);
  }
  init_output(&output, file_size, p - 1, save_as);

  int i = 0;
  while (file_size >= 8 * (p - 1)) {
    if (input[0].ed == input[0].p)
      for (i = 0; i < p; i++)
        flush_input(&input[i]);
    for (i = 0; i < p && file_size >= 8 * (p - 1); i++) {
      if (output.ed == output.p)
        flush_output(&output);
      write_array_unsafe(&output, input[i].p, p - 1);
      input[i].p += p - 1;
      file_size -= 8 * (p - 1);
    }
  }
  i = i % p;
  if (output.ed == output.p)
    flush_output(&output);
  if (input[i].ed == input[i].p)
    flush_input(&input[i]);
  write_array_unsafe(&output, input[i].p, file_size >> 3);
  input[i].p += file_size >> 3;
  flush_output(&output);
  write_bytes_direct(&output, read_uint64_unsafe(&input[i]), file_size & 7);

  for (int i = 0; i < p; i++)
    del_input(&input[i]);
  del_output(&output);
}

/**
 * @brief 修复加密数据文件夹
 * @param dir_path 要修复的文件夹路径
 * @return 是否成功修复
 * @example repair_directory("disk_1");
 */
bool repair_directory(const char *dir_path) {
  DIR *root;
  struct dirent *sub_dir;
  char sub_dir_path[MAX_FILE_NAME_LENGTH];
  struct Input input;
  long long file_size;
  int p;

  root = opendir(dir_path);
  while ((sub_dir = readdir(root)) != NULL) {
    if (strcmp(sub_dir->d_name, ".") == 0 || strcmp(sub_dir->d_name, "..") == 0)
      continue;

    sprintf(sub_dir_path, "%s/%s", dir_path, sub_dir->d_name);

    if (sub_dir->d_type == DT_DIR) {
      if (!repair_directory(sub_dir_path))
        return false;
    } else {
      int pos = 0; // 找到第一个斜杆出现的下标
      while (sub_dir_path[pos] != '/')
        pos++;

      get_info(sub_dir_path, &file_size, &p);

      if (!repair_work(sub_dir_path + pos + 1, file_size, p,
                       false)) // 此处 sub_dir_path + pos + 1 即为原文件路径
        return false;
    }
  }
  closedir(root);
  return true;
}

/**
 * @brief 已知损坏的文件夹个数 number_erasures 以及具体损坏文件夹的编号 idx,
 * 修复磁盘
 * @param number_erasures 损坏的文件夹个数，大于 2 时无法修复
 * @param idx 具体损坏文件夹的编号
 * @return NULL
 * @example repair(2, [0, 1]);
 */
void repair(const int number_erasures, const int *idx) {
  if (number_erasures == 0)
    return;
  if (number_erasures > 2) {
    printf("Too many corruptions!\n");
    return;
  }

  int disk_ok_id = 0; // 找到一个完整磁盘，以获取需要修复的文件名

  // 这部分是求 mex
  while (disk_ok_id == idx[0] || (number_erasures == 2 && disk_ok_id == idx[1]))
    disk_ok_id++;

  char disk_ok_name[MAX_FILE_NAME_LENGTH];
  sprintf(disk_ok_name, "disk_%d", disk_ok_id);
  if (!repair_directory(disk_ok_name)) {
    printf("Too many corruptions!\n");
    return;
  }
}

void usage() {
  printf("./evenodd write <file_name> <p>\n");
  printf("./evenodd read <file_name> <save_as>\n");
  printf("./evenodd repair <number_erasures> <idx0> ...\n");
}

int main(int argc, char **argv) {
  // int id[]= {0};
  // repair(1, id);
  // return 0;
  if (argc < 2) {
    usage();
    return -1;
  }

  char *op = argv[1];
  if (strcmp(op, "write") == 0) {
    /*
     * Please encode the input file with EVENODD code
     * and store the erasure-coded splits into corresponding disks
     * For example: Suppose "file_name" is "testfile", and "p" is 5. After
     * your encoding logic, there should be 7 splits, "testfile_0",
     * "testfile_1",
     * ..., "testfile_6", stored in 7 diffrent disk folders from "disk_0" to
     * "disk_6".
     * "p" is considered to be less or equal to 100.
     */
    write(argv[2], atoi(argv[3]));
  } else if (strcmp(op, "read") == 0) {
    /*
     * Please read the file specified by "file_name", and store it as a file
     * named "save_as" in the local file system.
     * For example: Suppose "file_name" is "testfile" (which we have encoded
     * before), and "save_as" is "tmp_file". After the read operation, there
     * should be a file named "tmp_file", which is the same as "testfile".
     */
    read(argv[2], argv[3]);
  } else if (strcmp(op, "repair") == 0) {
    /*
     * Please repair failed disks. The number of failures is specified by
     * "num_erasures", and the index of disks are provided in the command
     * line parameters.
     * For example: Suppose "number_erasures" is 2, and the indices of
     * failed disks are "0" and "1". After the repair operation, the data
     * splits in folder "disk_0" and "disk_1" should be repaired.
     */
    int number_erasures = atoi(argv[2]);
    int idx[number_erasures];
    for (int i = 0; i < number_erasures; i++)
      idx[i] = atoi(argv[i + 3]);
    repair(number_erasures, idx);
  } else {
    printf("Non-supported operations!\n");
  }
  return 0;
}
