
## .env Redis 配置

在运行 `src/test.cpp` 前，请在 `Redis` 目录下创建 `.env` 文件，示例：

```env
REDIS_HOST=127.0.0.1
REDIS_PORT=6379
REDIS_PASSWORD=
REDIS_DB=0
REDIS_CONNECT_TIMEOUT_MS=2000
REDIS_SOCKET_TIMEOUT_MS=2000
```

程序会读取这些配置并连接 Redis，然后执行 `PING` 验证连接。


## 1. RESP 协议

RESP（Redis Serialization Protocol）是 Redis 客户端与服务器之间进行通信所使用的**序列化协议**。它规定了 Redis 命令和返回值在网络中应该如何表示。

例如客户端执行：

```bash
SET name Tom
```

Redis 客户端会按照 RESP 协议将命令编码后发送给服务器，服务器执行完成后，再按照 RESP 格式返回结果。

RESP 的特点是**简单、易解析、效率较高**。

### RESP2 的基本数据类型

| 类型            | 前缀  | 作用               |
| ------------- | --- | ---------------- |
| Simple String | `+` | 表示简单字符串，例如 `+OK` |
| Error         | `-` | 表示错误信息           |
| Integer       | `:` | 表示整数             |
| Bulk String   | `$` | 表示字符串或二进制数据      |
| Array         | `*` | 表示多个元素组成的数组      |

例如：

```text
+OK\r\n
```

表示简单字符串 `OK`。

```text
:100\r\n
```

表示整数 `100`。

```text
$5\r\nhello\r\n
```

表示字符串 `hello`。

Redis 客户端发送命令时，通常会将命令表示为一个 **Array**，数组中的每个参数都是一个 Bulk String。例如：

```text
SET name Tom
```

会表示为：

```text
*3
$3
SET
$4
name
$3
Tom
```

因此可以简单理解为：

> **RESP 就是 Redis 客户端和服务器之间约定的一种通信格式，用来规定“命令怎么发送、结果怎么返回”。**

目前 Redis 还支持 **RESP3**，它在 RESP2 的基础上增加了 Map、Set、Boolean、Null 等数据类型。

[Redis 官方 RESP 协议文档](https://redis.io/docs/latest/develop/reference/protocol-spec/?utm_source=chatgpt.com)




## 2. C++客户端 redis++安装

### 1. 依赖说明

`redis-plus-plus` 基于 C 语言官方客户端 `hiredis` 封装实现，需先安装 hiredis 开发库。

### 2. 安装 hiredis

- **Ubuntu/Debian**

```
apt install libhiredis-dev
```

- **CentOS**

```
yum install hiredis-devel.x86_64
```

### 3. 编译安装 redis-plus-plus

下载源码：

```
git clone https://github.com/sewenew/redis-plus-plus.git
```

#### Ubuntu 编译安装

```
cd redis-plus-plus
mkdir build
cd build
cmake ..
make
sudo make install
```

#### CentOS 编译安装

CentOS 自带 cmake 版本较低，需先安装 cmake3：

```
yum install cmake3
```

再执行编译安装：

```
cd redis-plus-plus
mkdir build
cd build
cmake3 ..
make
sudo make install
```

#### 安装产物

- 头文件路径：`/usr/local/include/sw/redis++/`
- 静态库路径：`/usr/local/lib/`（CentOS 为 `/usr/local/lib64/`）

## 3. cmake编写

```cpp
cmake_minimum_required(VERSION 3.10)
project(RedisClient LANGUAGES CXX)

# redis-plus-plus 当前安装版本使用 std::string_view/std::optional/std::variant 因此必须启用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# PkgConfig 用于读取系统库的 .pc 文件；本项目中的 cmake/Findhiredis.cmake 会通过 hiredis.pc 补齐 hiredis 的 CMake 查找能力。
find_package(PkgConfig REQUIRED)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# hiredis 是 redis++ 底层依赖，jsoncpp 用于解析 .env 后保存为 Json::Value。
find_package(hiredis REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(redis++ REQUIRED)

# redis_test 是当前示例程序：读取 .env，连接 Redis，并执行 PING。
add_executable(redis_test src/test.cpp)

# 使用导入目标链接第三方库，可以自动携带 include 路径和链接参数。
target_link_libraries(redis_test PRIVATE
    redis++::redis++
    JsonCpp::JsonCpp
)

```

## 4. 通用命令

覆盖 EXISTS、DEL、KEYS、EXPIRE、TTL、PTTL、TYPE 等全局键操作命令。

### GET

* **参数类型**：`StringView key`，对于字符串可读不能写
* **返回值类型**：`std::optional<std::string>`，可能存在值也可能为空
* **含义**：Key 存在返回 value，不存在返回 `std::nullopt`。

### SET

* **参数类型**：`StringView key, StringView value`
* **返回值类型**：`bool`
* **含义**：设置成功返回 `true`，失败或条件不满足返回 `false`。


### EXISTS

* **参数类型**：`StringView key` （也支持传入多个 key）
* **返回值类型**：`long long`
* **含义**：返回**存在的 key 的数量**。

### DEL

* **参数类型**：`StringView key`（也支持传入多个 key）
* **返回值类型**：`long long`
* **含义**：返回**实际删除的 key 的数量**。


### KEYS

* **参数类型**：`StringView pattern`
* **返回值类型**：`void`
* **含义**：将**匹配 `pattern` 的所有 key** 写入你提供的输出迭代器中。Redis++ 的 `keys` 本身不直接返回一个容器。


这里强调一下三个特殊迭代器：
| 类型                           | 创建方式                     | 作用        | 插入位置        | 对应容器操作         | 常见容器                    |
| ---------------------------- | ------------------------ | --------- | ----------- | -------------- | ----------------------- |
| `std::insert_iterator`       | `std::inserter(c, pos)`  | 向指定位置插入元素 | `pos` 指定的位置 | `insert()`     | `vector`、`list`、`set` 等 |
| `std::back_insert_iterator`  | `std::back_inserter(c)`  | 向容器末尾插入元素 | 尾部          | `push_back()`  | `vector`、`deque`、`list` |
| `std::front_insert_iterator` | `std::front_inserter(c)` | 向容器头部插入元素 | 头部          | `push_front()` | `deque`、`list`          |

### EXPIRE

* **参数类型**：`StringView key`、`long long timeout`
* **返回值类型**：`bool`
* **含义**：给指定 key 设置过期时间，设置成功返回 `true`，key 不存在返回 `false`。
### TTL

* **参数类型**：`StringView key`
* **返回值类型**：`long long`
* **含义**：返回 key 的剩余过期时间，单位是秒；`-1` 表示永不过期，`-2` 表示 key 不存在。


### TYPE

* **参数类型**：`StringView key`
* **返回值类型**：`std::string`
* **含义**：返回指定 key 的数据类型，例如 `string`、`list`、`set`、`zset`、`hash`、`stream`；key 不存在时返回 `none`。

测试用函数：
```cpp
void testGenericCommands(sw::redis::Redis &redis)
{
    printf("Generic 系列命令:\n");
    printf("=============================================\n");

    // getset命令
    {
        printf("GetSet 命令\n\n");
        redis.flushdb();
        redis.set("key1", "111");
        auto ret = redis.get("key1");
        if (ret)
            cout << ret.value() << endl; // 由于返回类型是optional，需要判断真假
    }

    // EXISTS 命令
    {
        printf("EXISTS 命令\n\n");
        redis.flushdb();

        bool b;
        b = redis.set("key1", "Hello");
        printf("redis < SET key1 \"Hello\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        long long e;
        e = redis.exists("key1");
        printf("redis < EXISTS key1\n");
        printf("redis > %lld\n", e);

        e = redis.exists("nosuchkey");
        printf("redis < EXISTS nosuchkey\n");
        printf("redis > %lld\n", e);

        b = redis.set("key2", "World");
        printf("redis < SET key2 \"World\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        e = redis.exists({"key1", "key2", "nosuchkey"});
        printf("redis < EXISTS key1 key2 nosuchkey\n");
        printf("redis > %lld\n", e);
        printf("\n");
    }

    // DEL 命令
    {
        printf("DEL 命令\n\n");
        redis.flushdb();
        bool b;
        b = redis.set("key1", "Hello");
        printf("redis < SET key1 \"Hello\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        b = redis.set("key2", "World");
        printf("redis < SET key2 \"World\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        int d;
        d = redis.del({"key1", "key2", "key3"});
        printf("redis < DEL key1 key2 key3\n");
        printf("redis > %d\n", d);
        printf("\n");
    }

    // KEYS 命令
    {
        printf("KEYS 命令\n\n");
        redis.flushdb();
        std::unordered_map<std::string, std::string> kvs1 = {
            {"firstname", "Jack"},
            {"lastname", "Stuntman"},
            {"age", "35"}};
        redis.mset(kvs1.begin(), kvs1.end()); // 一次设置三个key
        printf("redis < MSET firstname Jack lastname Stuntman age 35\n");
        printf("redis > OK\n");

        std::vector<std::string> keys;
        std::insert_iterator<std::vector<std::string>> ins = std::inserter(keys, keys.begin()); // 创建一个“输出位置”，让 redis.keys() 把找到的 key 放进 keys 里面
        redis.keys("*name*", ins);                                                              // 前面可以有任意字符，后面也可以有任意字符，但中间必须出现 name
        printf("redis < KEYS *name*\n");
        int n = 1;
        for (auto it = keys.begin(); it != keys.end(); ++it)
        {
            printf("redis > %d) %s\n", n++, it->c_str());
        }
    }

    // EXPIRE & TTL 命令
    {
        printf("EXPIRE 命令\n");
        printf("TTL 命令\n\n");
        redis.flushdb();
        bool b;
        b = redis.set("mykey", "Hello");
        printf("redis < SET mykey \"Hello\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        b = redis.expire("mykey", std::chrono::seconds(10)); // Redis 中的键 mykey 设置 10 秒的过期时间
        printf("redis < EXPIRE mykey 10\n");
        printf("redis > %d\n", b ? 1 : 0);

        long long ttl = redis.ttl("mykey");
        printf("redis < TTL mykey\n");
        printf("redis > %lld\n", ttl);
        printf("\n");
    }

    // TYPE 命令
    {
        printf("TYPE 命令\n\n");
        redis.flushdb();
        bool b;
        b = redis.set("key1", "value");
        printf("redis < SET key1 \"value\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        long long c = redis.lpush("key2", "value");
        printf("redis < LPUSH key2 \"value\"\n");
        printf("redis > %lld\n", c);

        c = redis.sadd("key3", "value");
        printf("redis < SADD key3 \"value\"\n");
        printf("redis > %lld\n", c);

        std::string type = redis.type("key1");
        printf("redis < TYPE key1\n");
        printf("redis > \"%s\"\n", type.c_str());

        type = redis.type("key2");
        printf("redis < TYPE key2\n");
        printf("redis > \"%s\"\n", type.c_str());

        type = redis.type("key3");
        printf("redis < TYPE key3\n");
        printf("redis > \"%s\"\n", type.c_str());

        printf("=============================================\n");
    }
}
```


## 5. 字符串命令

覆盖 SET、GET、APPEND、GETRANGE、SETEX、INCR/DECR、MSET/MGET 等核心字符串操作。

### 完整代码

```cpp
void testStringCommands(sw::redis::Redis &redis)
{
    printf("String 系列命令:\n");
    printf("=============================================\n");

    // SET 命令（含 NX/XX/过期时间选项）
    {
        printf("SET 命令\n\n");
        redis.flushdb();

        long long e = redis.exists("mykey");
        printf("redis < EXISTS mykey\n");
        printf("redis > %lld\n", e);

        bool b = redis.set("mykey", "Hello");
        printf("redis < SET mykey \"Hello\"\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        auto v = redis.get("mykey");
        printf("redis < GET mykey\n");
        if (v)
        {
            printf("redis > \"%s\"\n", v->c_str());
        }
        else
        {
            printf("redis > (nil)\n");
        }

        // NX：键不存在时才设置
        b = redis.set("mykey", "World", std::chrono::milliseconds(0),
                      sw::redis::UpdateType::NOT_EXIST);
        printf("redis < SET mykey \"World\" NX\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        long long d = redis.del("mykey");
        printf("redis < DEL mykey\n");
        printf("redis > %lld\n", d);

        // XX：键存在时才设置
        b = redis.set("mykey", "World", std::chrono::seconds(0),
                      sw::redis::UpdateType::EXIST);
        printf("redis < SET mykey \"World\" XX\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        v = redis.get("mykey");
        printf("redis < GET mykey\n");
        if (bool(v))
        {
            printf("redis > \"%s\"\n", v->c_str());
        }
        else
        {
            printf("redis > (nil)\n");
        }

        // 设置带过期时间的键
        b = redis.set("mykey", "Will expire in 10s", std::chrono::seconds(10));
        printf("redis < SET mykey \"Will expire in 10s\" EX 10\n");
        printf("redis > %s\n", b ? "OK" : "(nil)");

        v = redis.get("mykey");
        printf("redis < GET mykey\n");
        if (v)
        {
            printf("redis > \"%s\"\n", v->c_str());
        }
        else
        {
            printf("redis > (nil)\n");
        }

        // 等待过期
        for (int i = 10; i > 0; --i)
        {
            printf("Waiting %ds\n", i);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        v = redis.get("mykey");
        printf("redis < GET mykey\n");
        if (v)
        {
            printf("redis > \"%s\"\n", v->c_str());
        }
        else
        {
            printf("redis > (nil)\n");
        }
        printf("\n");
    }

    // GET 命令与批量操作命令
    {
        printf("GET 命令\n\n");
        redis.flushdb();

        auto v = redis.get("nonexisting");
        printf("redis < GET nonexisting\n");
        if (v)
        {
            printf("redis > \"%s\"\n", v->c_str());
        }
        else
        {
            printf("redis > (nil)\n");
        }

        std::unordered_map<std::string, std::string> kvs1 = {
            {"firstname", "Jack"},
            {"lastname", "Stuntman"},
            {"age", "35"}};
        redis.mset(kvs1.begin(), kvs1.end()); // 一次设置多个key
        std::vector<std::string> keys;
        std::insert_iterator<std::vector<std::string>> ins = std::inserter(keys, keys.begin()); // 还是将查询的value插入数组
        redis.mget({"firstname", "lastname"}, ins);                                             // 一次查询多个value
        for (auto it = keys.begin(); it != keys.end(); ++it)
        {
            printf("redis > %d) %s\n", it->c_str());
        }
    }

    // APPEND、GETRANGE 命令
    {
        printf("APPEND 命令\n\n"); // 返回追加字符串后，整个 value 的长度
        redis.flushdb();
        long long c = redis.append("mykey", "Hello");
        printf("redis < APPEND mykey \"Hello\"\n");
        printf("redis > %lld\n", c);
        c = redis.append("mykey", " World");
        printf("redis < APPEND mykey \" World\"\n");
        printf("redis > %lld\n", c);
        printf("\n");

        printf("SETRANGE 命令\n\n");
        redis.set("mykey", "This is a string");
        printf("redis < SETRANGE mykey 0 Redis > %lld\n", redis.setrange("mykey", 0, "Redis"));
        printf("redis < GET mykey > %s\n", redis.get("mykey").value().c_str());

        printf("GETRANGE 命令\n\n"); // 返回指定范围内的字符串，如果不存在则返回空字符串
        redis.set("mykey", "This is a string");
        printf("redis < GETRANGE mykey 0 3 > %s\n", redis.getrange("mykey", 0, 3).c_str());
        printf("redis < GETRANGE mykey -3 -1 > %s\n", redis.getrange("mykey", -3, -1).c_str());
    }

    // 数值增减命令
    {
        printf("INCR / DECR 命令\n\n");
        redis.flushdb();
        printf("redis < INCR mykey > %lld\n", redis.incr("mykey"));
        redis.set("mykey", "10");
        printf("redis < INCR mykey > %lld\n", redis.incr("mykey"));
        printf("redis < DECR mykey > %lld\n", redis.decr("mykey"));
        printf("redis < INCRBY mykey 5 > %lld\n", redis.incrby("mykey", 5));
        printf("redis < INCRBYFLOAT mykey 0.5 > %.2f\n", redis.incrbyfloat("mykey", 0.5));
        printf("=============================================\n");
    }
}
```

## 6. 列表命令

覆盖 LPUSH/RPUSH、LPOP/RPOP、LRANGE、阻塞弹出、索引查询等列表操作。

### 完整代码

```cpp
void testListCommands(sw::redis::Redis &redis)
{
    printf("List 系列命令:\n");
    printf("=============================================\n");

    // 左右压入命令
    {
        printf("LPUSH / RPUSH 命令\n\n");
        redis.flushdb();
        redis.lpush("mylist", "world");            // 左压入一个元素
        redis.lpush("mylist", {"hello", "Redis"}); // 支持一次性压入多个元素

        std::vector<std::string> elements;
        redis.lrange("mylist", 0, -1, std::inserter(elements, elements.begin())); // 获取指定下标范围的元素
        printf("redis < LRANGE mylist 0 -1\n");
        int n = 1;
        for (auto &e : elements)
        {
            printf("redis > %d) %s\n", n++, e.c_str());
        }
    }

    // 弹出命令
    {
        printf("LPOP / RPOP 命令\n\n");
        redis.flushdb();
        redis.rpush("mylist", {"one", "two", "three", "four", "five"}); // 先压入五个元素

        auto left = redis.lpop("mylist"); // 删除并返回 List 左侧的第一个元素；List 不存在或为空时返回 std::nullopt
        printf("redis < LPOP mylist > %s\n", left ? left->c_str() : "(nil)");
        auto right = redis.rpop("mylist");
        printf("redis < RPOP mylist > %s\n", right ? right->c_str() : "(nil)");
        printf("\n");
    }

    // 阻塞弹出 BLPOP/BRPOP
    {
        printf("BLPOP 阻塞弹出命令\n\n");
        redis.flushdb();
        redis.rpush("list1", {"a", "b", "c"});

        auto res = redis.blpop({"list1", "list2"}, std::chrono::seconds(0)); // 成功弹出时，返回 {key, value}；超时没有弹出元素时返回 std::nullopt
        if (res)
        {
            printf("redis > 来自键: %s, 值: %s\n", res->first.c_str(), res->second.c_str());
        }
        printf("\n");
    }

    // 索引与长度
    {
        printf("LINDEX / LLEN 命令\n\n");
        redis.flushdb();
        redis.rpush("mylist", {"Hello", "World"});
        printf("redis < LLEN mylist > %lld\n", redis.llen("mylist"));

        auto val = redis.lindex("mylist", 0); // 类似于get，返回指定下标的元素；不存在时返回 std::nullopt
        printf("redis < LINDEX mylist 0 > %s\n", val ? val->c_str() : "(nil)");
        printf("=============================================\n");
    }
}
```

## 7. 集合命令

覆盖增删查、集合运算（交集、并集、差集）等无序集合操作。

### 完整代码

```cpp
void testSetCommands(sw::redis::Redis &redis)
{
    printf("Set 系列命令:\n");
    printf("=============================================\n");

    // 基础增删查
    {
        printf("SADD / SMEMBERS / SISMEMBER 命令\n\n");
        redis.flushdb();
        redis.sadd("myset", "Hello");
        redis.sadd("myset", "World");
        redis.sadd("myset", "World"); // 重复元素自动去重

        std::unordered_set<std::string> elements;
        redis.smembers("myset", std::inserter(elements, elements.begin()));
        printf("集合元素数量: %zu\n", elements.size());
        printf("是否包含 Hello: %d\n", redis.sismember("myset", "Hello"));

        printf("SISMEMBER 和 SPOP 命令\n\n");
        redis.sadd("myset", {"Hello", "World", "Redis"});
        // SREM：删除集合中的指定元素
        printf("SREM 删除 World: %lld\n", redis.srem("myset", "World"));
        // SPOP：随机删除并返回一个元素
        auto element = redis.spop("myset");
        if (element)
        {
            printf("SPOP 弹出元素: %s\n", element->c_str());
        }
    }

    // 集合运算
    {
        printf("交集 / 并集 / 差集 命令\n\n");
        redis.flushdb();
        redis.sadd("key1", {"a", "b", "c"});
        redis.sadd("key2", {"c", "d", "e"});

        std::unordered_set<std::string> inter;
        redis.sinter({"key1", "key2"}, std::inserter(inter, inter.begin()));
        printf("交集元素数量: %zu\n", inter.size());

        long long count = redis.sinterstore("key3", {"key1", "key2"});

        std::unordered_set<std::string> union_set;
        redis.sunion({"key1", "key2"}, std::inserter(union_set, union_set.begin()));
        printf("并集元素数量: %zu\n", union_set.size());

        std::unordered_set<std::string> diff;
        redis.sdiff({"key1", "key2"}, std::inserter(diff, diff.begin()));
        printf("差集元素数量: %zu\n", diff.size());
    }
}
```

## 8. 哈希命令

覆盖字段读写、批量操作、数值增减等哈希结构操作。

### 完整代码

```cpp
void testHashCommand(sw::redis::Redis &redis)
{
    printf("Hash 系列命令:\n");
    printf("=============================================\n");

    // 基础读写
    {
        printf("HSET / HGET 命令\n\n");
        redis.flushdb();

        redis.hset("key", "f1", "111");
        redis.hset("key", std::make_pair("f2", "222"));
        // hset 能够一次传入多个 field-value 对!!
        redis.hset("key", {std::make_pair("f3", "333"),
                           std::make_pair("f4", "444")});
        vector<std::pair<string, string>> fields = {
            std::make_pair("f5", "555"),
            std::make_pair("f6", "666")};
        redis.hset("key", fields.begin(), fields.end());

        redis.hset("user", "name", "zhangsan");
        redis.hset("user", "age", "20");

        auto name = redis.hget("user", "name");
        auto age = redis.hget("user", "age");
        if (name && age)
        {
            printf("name: %s, age: %s\n", name->c_str(), age->c_str());
        }
    }

    // 批量操作
    {
        printf("HKEYS / HVALS / HMGET 命令\n\n");
        redis.flushdb();
        redis.hset("user", "name", "zhangsan");
        redis.hset("user", "age", "20");

        // HKEYS：获取 Hash 中所有 field
        std::vector<string> keys;
        redis.hkeys("user", std::back_inserter(keys));
        printf("字段数量: %zu\n", keys.size());

        for (const auto &key : keys)
        {
            printf("field: %s\n", key.c_str());
        }

        // HVALS：获取 Hash 中所有 value
        std::vector<string> values;
        redis.hvals("user", std::back_inserter(values));

        printf("值数量: %zu\n", values.size());

        for (const auto &value : values)
        {
            printf("value: %s\n", value.c_str());
        }

        // HMGET：根据多个 field 获取对应的 value
        std::vector<sw::redis::OptionalString> hmget_values;
        vector<string> fields = {"name", "age"};
        redis.hmget("user", fields.begin(), fields.end(), std::back_inserter(hmget_values));
    }

    // 数值增减
    {
        printf("HINCRBY 命令\n\n");
        redis.flushdb();
        redis.hset("user", "age", "20");
        long long new_age = redis.hincrby("user", "age", 2);
        printf("自增后年龄: %lld\n", new_age);
        printf("=============================================\n");
    }
}
```

## 9. 有序集合命令

覆盖带权值的增删查、排名查询、集合运算等有序集合操作。

### 完整代码

```cpp
void testZsetCommand(sw::redis::Redis &redis)
{
    printf("Zset 系列命令:\n");
    printf("=============================================\n");

    // 基础增删与范围查询
    {
        printf("ZADD / ZRANGE 命令\n\n");
        redis.flushdb();
        // 这里其实支持很多插入方式，但是
        redis.zadd("ranking", "吕布", 100);
        redis.zadd("ranking", "赵云", 98);
        redis.zadd("ranking", "典韦", 95);

        vector<std::pair<string, double>> members;
        redis.zrange("ranking", 0, -1, std::inserter(members, members.begin()));
        for (auto &item : members)
        {
            printf("%s: %.0f\n", item.first.c_str(), item.second);
        }
    }

    // 排名与分数查询
    {
        printf("ZRANK / ZSCORE / ZREVRANK / ZCARD 命令\n\n");

        auto rank = redis.zrank("ranking", "吕布"); // 某个成员从小到大排第几
        auto score = redis.zscore("ranking", "吕布");

        printf("吕布的排名(从小到大): %lld, 分数: %.0f\n", rank.value(), score.value());

        rank = redis.zrevrank("ranking", "吕布"); // 某个成员从大到小排第几

        printf("吕布的排名(从大到小): %lld\n", rank.value());

        auto count = redis.zcard("ranking");
        printf("ranking中的成员数量: %lld\n", count);

        auto result = redis.zrem("ranking", "吕布");
        printf("删除了 %lld 个成员\n", result);
    }

    // 交集运算
    {
        printf("ZINTERSTORE 命令\n\n");
        redis.flushdb();
        redis.zadd("key1", "吕布", 100);
        redis.zadd("key1", "赵云", 98);
        redis.zadd("key2", "吕布", 100);
        redis.zadd("key2", "关羽", 92);

        long long count = redis.zinterstore("result", {"key1", "key2"});
        printf("交集结果数量: %lld\n", count);
    }
}
```


