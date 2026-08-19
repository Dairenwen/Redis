# Redis
Redis learning

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
