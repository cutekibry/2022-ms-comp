# 2022-ms-comp
2022 Massive Storage 第一届大学生信息存储技术竞赛 · 挑战赛 - WHUACM2022新生群-team026

## 提交方式
略。

**注意：本仓库使用 `main` 作为主分支而不是 `master`**。

## 代码规范
**此章节规范暂定，有修改意见可以提出**。

### 一些规范
* 变量 / 函数命名用下划线命名法（如 `disk_dir_name` 等），默认全小写。
  * 结构体首字母大写（如 `Disk_dir`）。
  * 常量全大写（如 `MAX_RANGE`）。
* 不使用 `not`、`and` 和 `or`，请使用 `!`、`&&` 和 `||` 代替。

### Commit 信息规范
参考 [Git 提交的正确姿势：Commit message 编写指南](https://developer.aliyun.com/article/441408)。

Commit message 可以只写简要概括，但请不要写个 `git update` 或不写就交上来。

其中 `type`（`feat` 等）写英文，其余部分写中文。我英语下手，请见谅。🙏

### 注释规范
* 使用 <code>"str"</code> 表示字符串 str。
* 写中文。我英语下手，请见谅。🙏

### 函数说明规范
参考：

```c
/**
 * @brief 读入文件 `file_name`，经 EVENODD 加密后储存
 * 从文件 `file_name` 读入数据并编码，然后将 ``p + 2`` 个数据块储存在 "disk_0",
 * "disk_1", ..., "disk_``p + 1``" 文件夹下。
 *
 * @param file_name 文件名
 * @param p 用于 EVENODD 加密的质数，应当保证 ``p <= 100``。
 *
 * @return NULL
 *
 * @example write("testfile", 5);
 *
 * @todo(Tsukimaru): 需要讨论如何完善该函数的具体实现。
 */
void write(const char *file_name, const int p) {
  // do something
}
```

* `@brief`：函数简介，通常不超过一行。
* 具体描述：写在 `@brief` 正下方（可空一行）。
* `@param`：参数描述，可以有多个 `@param`。
* `@return`：返回值描述。
* `@example`：使用样例，可以写注释。应当只有一个 `@example`，多个样例写多行即可。
* `@todo(user)`：需要之后做的事情，可以有多个 `@todo`。
* `@bug`：用于提示存在的 BUG，可以有多个 `@bug`。

其中 `@brief`、`@param` 和 `@return` 必填，其余选填。需要的话可以写多行。

`@see` 等别的关键字也可以用，可以试着在注释里打 `@` 查看补全列表，只要大家能看懂就行。

`type`（`@brief` 等）写英文，其余部分写中文。我英语下手，请见谅。🙏

### 代码格式化
本项目使用 `LLVM` 代码标准，可使用 `clang-format` 等工具进行代码格式化。