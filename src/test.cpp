#include <sw/redis++/redis++.h>
#include <jsoncpp/json/json.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <thread>
#include <string>

using namespace std;

// 读取 .env 文件，并把 KEY=VALUE 形式的配置保存到 Json::Value 中。
Json::Value loadEnvConfig(const string &envPath)
{
    Json::Value root;
    ifstream file(envPath);
    if (!file.is_open())
    {
        cerr << "Failed to open env file: " << envPath << endl;
        return root;
    }
    string line;
    while (getline(file, line))
    {
        // 空行和注释行不参与配置解析。
        if (line.empty() || line[0] == '#')
            continue;

        // 每行只按第一个 '=' 分割，左边是配置名，右边是配置值。
        auto pos = line.find('=');
        if (pos != string::npos)
        {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);

            // 去除配置名和配置值两端的空白，避免 "REDIS_HOST = 127.0.0.1"
            // 这类写法导致 key/value 多出空格。
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            root[key] = value;
        }
    }

    // 保留 Json::StreamWriterBuilder，方便后续需要调试输出配置时复用。
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "   ";
    Json::writeString(builder, root);
    return root;
}

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

    // GET 命令与类型错误异常
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
        printf("APPEND 命令\n\n");
        redis.flushdb();
        long long c = redis.append("mykey", "Hello");
        printf("redis < APPEND mykey \"Hello\"\n");
        printf("redis > %lld\n", c);
        c = redis.append("mykey", " World");
        printf("redis < APPEND mykey \" World\"\n");
        printf("redis > %lld\n", c);
        printf("\n");

        printf("GETRANGE 命令\n\n");
        redis.set("mykey", "This is a string");
        printf("redis < GETRANGE mykey 0 3 > %s\n", redis.getrange("mykey", 0, 3).c_str());
        printf("redis < GETRANGE mykey -3 -1 > %s\n", redis.getrange("mykey", -3, -1).c_str());
        printf("=============================================\n");
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

    // 批量操作命令
    {
        printf("MSET / MGET 命令\n\n");
        redis.flushdb();
        std::vector<std::pair<std::string, std::string>> kvs = {
            {"key1", "Hello"}, {"key2", "World"}};
        redis.mset(kvs.begin(), kvs.end());
        printf("redis < MSET key1 Hello key2 World\n");

        std::vector<sw::redis::OptionalString> values;
        redis.mget({"key1", "key2", "nonexisting"}, std::back_inserter(values));
        int n = 1;
        for (auto &val : values)
        {
            printf("redis > %d) %s\n", n++, val ? val->c_str() : "(nil)");
        }
        printf("\n");
    }
}

int main()
{
    string envPath = "../.env";
    Json::Value config = loadEnvConfig(envPath);

    // 从配置中提取 Redis 连接参数；缺省值保证 .env 缺失部分字段时尝试本地连接。
    string host = config.get("REDIS_HOST", "127.0.0.1").asString();
    int port = config.get("REDIS_PORT", "6379").asString().empty() ? 6379 : stoi(config["REDIS_PORT"].asString());
    string password = config.get("REDIS_PASSWORD", "").asString();
    int db = config.get("REDIS_DB", "0").asString().empty() ? 0 : stoi(config["REDIS_DB"].asString());

    // redis++ 的连接选项对象，负责描述目标 Redis 服务地址、认证信息和数据库编号。
    sw::redis::ConnectionOptions options;
    options.host = host;
    options.port = port;
    if (!password.empty())
        options.password = password;
    options.db = db;

    // 建立连接并发送 PING；成功时 Redis 会返回 PONG。
    sw::redis::Redis redis(options);
    std::string result = redis.ping();
    std::cout << result << std::endl;

    testGenericCommands(redis);
    testStringCommands(redis);
    return 0;
}
