#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>

typedef __uint128_t uint128;

#define max(x, y)                                                              \
  ({                                                                           \
    typeof(x) _x = (x);                                                        \
    typeof(y) _y = (y);                                                        \
    _x > _y ? _x : _y;                                                         \
  })
#define min(x, y)                                                              \
  ({                                                                           \
    typeof(x) _x = (x);                                                        \
    typeof(y) _y = (y);                                                        \
    _x < _y ? _x : _y;                                                         \
  })
#define swap(x, y)                                                             \
  ({                                                                           \
    typeof(x) t = x;                                                           \
    x = y;                                                                     \
    y = t;                                                                     \
  })                                                                           \

#define mod_id(x,p) (((x) >= (p)) ? ((x) - (p)) : (((x) < 0) ? ((x) + (p)) : (x)))
#define ALL1(n) (((uint128)1 << (n)) - 1)



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
 * @example file_create("disk3/subdir/something");
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
    1 << 24; // 函数内 IO 缓存区大小最大字节数（不严格），防止空间过大
const int MAX_PER_IO_BUFFER_SIZE =
    1 << 20; // 单个 IO 缓存区大小最大字节数，防止缓存过大影响速度
const int MAX_FILE_NAME_LENGTH =
    120;     // 文件名的最大长度
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
  char *st, *ed, *p;
  FILE *file;
  int remain;
};

/**
 * @brief 初始化 Input。
 * 为 (*buffer) 申请 size 字节的空间，设置其输入文件名为 file_name。
 * @param buffer 指向 Input 的指针
 * @param size 申请字节大小
 * @param file_name 文件名
 * @return NULL
 */
void init_input(struct Input *buffer, int size, const char *file_name) {
  size = min(size, get_file_stat(file_name).st_size);
  buffer->st = (char *)malloc(size);
  buffer->ed = buffer->st + size;
  buffer->p = buffer->ed;
  buffer->file = fopen(file_name, "rb");
  buffer->remain = 8;
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

/**
 * @brief 从文件读入足量字符填充缓存区。需要保证缓存区已全部用尽。
 * 一般不需要手动调用此函数，当缓存区用完时会自动调用。
 * @param buffer 指向 Input 的指针
 * @return NULL
 */
void flush_input(struct Input *buffer) {
  assert(buffer->p == buffer->ed);
  memset(buffer->st, 0, buffer->ed - buffer->st);
  fread(buffer->st, 1, buffer->ed - buffer->st, buffer->file);
  buffer->remain = 8;
  buffer->p = buffer->st;
}

/**
 * @brief 从 Input 读入 n 个 bool 并返回。
 * 若读入的 bool 分别为 x_0, x_1, ..., x_n，则返回的值为 (x_0x_1...x_n)_2。
 * @param buffer 指向 Input 的指针
 * @param n 读入 bool 个数，不超过 128
 * @return 由读入的 n 个 bool 构成的整数
 */
uint128 read_bits(struct Input *buffer, int n) {
  uint128 result = 0;

  if (buffer->p == buffer->ed)
    flush_input(buffer);

  if (n < buffer->remain) {
    buffer->remain -= n;
    result = *buffer->p >> buffer->remain & ALL1(n);
  } else {
    result = *buffer->p & ALL1(buffer->remain);
    n -= buffer->remain;
    buffer->p++;

    while (n >= 8) {
      if (buffer->p == buffer->ed)
        flush_input(buffer);
      result = result << 8 | *buffer->p;
      n -= 8;
      buffer->p++;
    }

    if (buffer->p == buffer->ed)
      flush_input(buffer);
    buffer->remain = 8 - n;
    result = result << n | (*buffer->p >> buffer->remain & ALL1(n));
  }
  return result;
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
  char *st, *ed, *p;
  FILE *file;
  int remain;
};

/**
 * @brief 初始化 Output。
 * 为 Output 申请 size 字节大小的空间，设置输出文件名为 file_name。
 * @param buffer 指向 Output 的指针
 * @param size 申请字节大小
 * @param file_name 文件名
 * @return NULL
 */
void init_output(struct Output *buffer, int size, const char *file_name) {
  buffer->st = (char *)malloc(size);
  buffer->ed = buffer->st + size;
  buffer->p = buffer->st;

  file_create(file_name);
  buffer->file = fopen(file_name, "wb");
  buffer->remain = 8;
  memset(buffer->st, 0, size);
}

/**
 * @brief 输出 Output 缓存区内的内容并清空。
 * 若 bool 数不为 8 的倍数，则在末尾补零直至 bool 数为 8 的倍数。
 * @param buffer 指向 Output 的指针
 * @return NULL
 */
void flush_output(struct Output *buffer) {
  int bytes = buffer->p - buffer->st + (buffer->remain < 8);

  fwrite(buffer->st, 1, bytes, buffer->file);
  memset(buffer->st, 0, bytes);

  buffer->p = buffer->st;
  buffer->remain = 8;
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

/**
 * @brief 向 Output 输出 n 个 bool。
 * 令 x = (y_0y_1...y_{n - 1})_2，输出 y_0y_1...y_{n - 1}。
 * @param buffer 指向 Output 的指针
 * @param x 输出 bool 的值
 * @param n 输出 bool 的个数
 * @return NULL
 */
void write_bits(struct Output *buffer, uint128 x, int n) {
  x &= ALL1(n);
  if (n < buffer->remain) {
    buffer->remain -= n;
    *buffer->p |= x << buffer->remain;
  } else {
    n -= buffer->remain;
    *buffer->p |= x >> n;
    buffer->p++;

    while (n >= 8) {
      if (buffer->p == buffer->ed)
        flush_output(buffer);
      n -= 8;
      *buffer->p = x >> n;
      buffer->p++;
    }

    if (buffer->p == buffer->ed)
      flush_output(buffer);
    buffer->remain = 8 - n;
    *buffer->p = x << buffer->remain;
  }
}

/**
 * @brief 修复文件名为file_name 在 idx 磁盘的数据
 * @param number_erasures 损坏文件夹个数 <=2
 * @param idx 损坏文件夹的编号
 * @param file_name 需要修复的文件名
 * @param size 该文件原来的大小
 * @param p 该文件对应的加密质数p
 * @return NULL
 * @example repair_work(2, [0,1], "testfile", size , 5);
 */
void repair_work(int number_erasures, int *idx, const char *file_name, 
                 const long long size, const int p){
              
  bool check_disk[p + 2];//为 1 表示完好， 为 0 表示损坏
  for (int i = 0; i < p + 2 ; i++)
    check_disk[i] = true;
  if (number_erasures == 2){ 
    if (idx[1] > p + 1) number_erasures--; //去除对该文件没有影响的磁盘
  }  
  for (int i = 0; i < number_erasures; i++)
    check_disk[idx[i]] = false;
  
  struct Input input[p + 2];
  struct Output output[number_erasures];
  uint128 a[p + 2];   // 0 ... (p + 1) 列的数据
  char disk_file_name[MAX_FILE_NAME_LENGTH];
  int now_output_id=0;

  for (int i = 0 ;i < p + 2; i++){
    sprintf(disk_file_name, "disk%d/%s", i, file_name);
    if (check_disk[i] == true) {
    
      init_input(
          &input[i],
          min(MAX_PER_IO_BUFFER_SIZE, MAX_IO_BUFFER_SIZE_SUM),
          disk_file_name);   //printf("%llu\n",a[i]);

      read_bits(&input[i], 48);
    }else {
      init_output(
          &output[now_output_id],
          min(min(MAX_PER_IO_BUFFER_SIZE, MAX_IO_BUFFER_SIZE_SUM / (p + 2)),
              size ),
          disk_file_name);

      write_bits(&output[now_output_id], size << 8 | p, 48);
      now_output_id++;
    }
  }

  for (long long t = 0; t < (size << 3); t += (p - 1) * p) {

    for (int i = 0; i < p + 2; i++)
    if (check_disk[i] == true) {
      a[i] = read_bits(&input[i], p - 1);
    }else {
      a[i] = 0;
    }
    if (number_erasures == 1){ // 1 个文件损坏
      
      if (idx[0] == p){
        for (int i = 0; i < p; i++)
        a[p] ^= a[i];

      }else
      if (idx[0] == p + 1){
        for (int i = 0; i < p; i++)
          a[p + 1] ^= (a[i] << i) ^ (a[i] >> (p - i));
        if (a[p + 1] >> (p - 1) & 1)
          a[p + 1] = ~a[p+1];
        a[p + 1] &= ALL1(p - 1);
        
      }else{
        for (int i=0; i < p + 1; i++)
        if (check_disk[i] == true) a[idx[0]] ^=a[i];
      }
      write_bits(&output[0], a[idx[0]], p - 1);
    
    }else {  // 2 个文件损坏
      const int disk_i = idx[0], disk_j = idx[1];
      if (disk_i == p && disk_j == p + 1){ //相当于重新加密
        for (int i = 0; i < p; i++)
        a[p] ^= a[i];
        for (int i = 0; i < p; i++)
          a[p + 1] ^= (a[i] << i) ^ (a[i] >> (p - i));
        if (a[p + 1] >> (p - 1) & 1)
          a[p + 1] = ~a[p+1];
        a[p + 1] &= ALL1(p - 1);
      }else
      if (disk_i < p && disk_j == p){
        uint128 S=0;  //对角线的 xor 和
        for (int i = 0; i < p; i++)
          S ^= (a[i] << i) ^ (a[i] >> (p - i));
        
        S &=ALL1(p);
        
        a[disk_i]= (S >> disk_i) ^ (a[p + 1] >> disk_i ) ^ (S << (p - disk_i)) ^ (a[p + 1] << (p - disk_i));
        
        if (((a[p + 1] >> mod_id(disk_i - 1, p)) & 1) ^ ((S >> mod_id(disk_i - 1, p)) & 1)) 
          a[disk_i] = ~a[disk_i];
        a[disk_i] &= ALL1(p - 1);
        for (int i = 0; i < p; i++)
        a[disk_j] ^=a[i];

      }else
      if (disk_i < p && disk_j == p + 1){ //由 a[p] 可以修复 disk_i 然后再求解a[p+1]
        for (int i = 0; i < p + 1; i++)
        if (check_disk[i] == true) a[disk_i] ^=a[i];
        for (int i = 0; i < p; i++)
          a[p + 1] ^= (a[i] << i) ^ (a[i] >> (p - i));
        if (a[p + 1] >> (p - 1) & 1)
          a[p + 1] = ~a[p+1];
        a[p + 1] &= ALL1(p - 1);
      }else
      if (disk_i < p && disk_j < p){    
        uint128 S = 0;  //对角线的 xor 和
        bool syndromes = 0;

        for (int i = 0; i < p; i++){
          S ^= (a[i] << i) ^ (a[i] >> (p - i));
          syndromes ^=((a[p] >> i) & 1) ^ ((a[p + 1] >> i) & 1);
        }
        S ^=a[p+1];
        if (syndromes)
          S = ~S;
        S &= ALL1(p);
        for (int i = 0; i < p; i++)
          a[p] ^= a[i];
        // 此时 a[p] 表示当前行丢失的两个数的 xor 值
        // S 表示对角线上丢失的两个数的 xor 值
        int now_diagonal_id = disk_j - 1, now_x = disk_j - disk_i -1;
        for (int i=0; i < p; i++){
          a[disk_i] ^= ((S >> now_diagonal_id) & 1) << now_x;
          a[disk_j] ^= (((a[p] ^ a[disk_i]) >> now_x) &1) << now_x;

          now_diagonal_id = mod_id(now_diagonal_id + disk_j - disk_i, p);
          S ^= ((((a[p] ^ a[disk_i]) >> now_x) &1)) << now_diagonal_id;
          now_x = mod_id(now_x + disk_j - disk_i, p);
        } 
      }
      write_bits(&output[0], a[disk_i], p-1);
      write_bits(&output[1], a[disk_j], p-1);
    }
  }

  for (int i =0 ;i < p + 2; i++)
  if (check_disk[i] == true) del_input(&input[i]);
  for (int i = 0; i < number_erasures; i++)
    del_output(&output[i]);
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
  uint128 a[p];   // 0 ... (p - 1) 列的数据
  uint128 b0, b1; // p 列的数据和 (p + 1) 列的数据

  file_size = get_file_stat(file_name).st_size;

  init_input(
      &input,
      min(min(MAX_PER_IO_BUFFER_SIZE, MAX_IO_BUFFER_SIZE_SUM), file_size),
      file_name);

  for (int i = 0; i < p + 2; i++) {
    char disk_file_name[MAX_FILE_NAME_LENGTH];

    sprintf(disk_file_name, "disk%d/%s", i, file_name);
    init_output(
        &output[i],
        min(min(MAX_PER_IO_BUFFER_SIZE, MAX_IO_BUFFER_SIZE_SUM / (p + 2)),
            file_size),
        disk_file_name);

    // 先将文件大小和 p 的值输出
    write_bits(&output[i], file_size << 8 | p, 48);
  }

  // 开始加密
  for (long long t = 0; t < (file_size << 3); t += (p - 1) * p) {
    for (int i = 0; i < p; i++)
      a[i] = read_bits(&input, p - 1);

    b0 = b1 = 0;

    for (int i = 0; i < p; i++)
      b0 ^= a[i];

    for (int i = 0; i < p; i++)
      b1 ^= (a[i] << i) ^ (a[i] >> (p - i));
    if (b1 >> (p - 1) & 1)
      b1 = ~b1;
    b1 &= ALL1(p - 1);

    for (int i = 0; i < p; i++)
      write_bits(&output[i], a[i], p - 1);
    write_bits(&output[p], b0, p - 1);
    write_bits(&output[p + 1], b1, p - 1);
  }

  del_input(&input);
  for (int i = 0; i < p + 2; i++)
    del_output(&output[i]);
}

void read(const char *file_name, const char *save_as) {}


/**
 * @brief 已知损坏的文件夹个数 number_erasures 以及具体损坏文件夹的编号 idx, 修复磁盘
 * @param number_erasures 损坏的文件夹个数，大于 2 时无法修复
 * @param idx 具体损坏文件夹的编号
 * @return NULL
 * @example repair(2, [0,1]);
 */

void repair(const int number_erasures, int *idx) {
  // 损坏文件夹个数大于 2 时无法修复
  
  if (number_erasures > 2){
    printf("Too many corruptions!");
    return;
  }
  if (number_erasures == 2){
    if (idx[0]>idx[1]) swap(idx[0],idx[1]); //排序
  }
  
  int disk_ok_id = 0; //找到一个完整磁盘，以获取需要修复的文件名

  // 这部分是求mex 
  for (int i = 0;i < number_erasures; i++)
  if (idx[i]==disk_ok_id) disk_ok_id++;

  char disk_ok_name[MAX_FILE_NAME_LENGTH];
  sprintf(disk_ok_name, "disk%d", disk_ok_id);

  DIR *disk_dir = opendir(disk_ok_name);
  if (!disk_dir) { //未能成功打开此文件夹
    printf("can't open a unbroken disk\n"); 
    return;
  }
  struct dirent *file;
  char file_name[MAX_FILE_NAME_LENGTH];
  while ((file = readdir(disk_dir)) != NULL){

    if (file->d_type == DT_REG) { //是普通文件时进行修复,否则可能找到 本级目录. 和 上级目录..
      struct Input input;
      long long file_size;

      sprintf(file_name, "disk%d/%s", disk_ok_id, file->d_name);
      
      init_input(&input, 6, file_name);

      file_size =read_bits(&input, 5 << 3);
      int p=read_bits(&input, 1 << 3);

      del_input(&input);
      
      repair_work(number_erasures, idx, file->d_name , file_size, p);
    }
    
  }
  closedir(disk_dir);

}


void usage() {
  printf("./evenodd write <file_name> <p>\n");
  printf("./evenodd read <file_name> <save_as>\n");
  printf("./evenodd repair <number_erasures> <idx0> ...\n");
}

int main(int argc, char **argv) {
  //write("testfile", 3);
  //return 0;
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
