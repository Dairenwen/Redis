# 生成前9个节点配置
for port in $(seq 1 9); \
do \
mkdir -p redis${port}/; \
touch redis${port}/redis.conf
cat << EOF > redis${port}/redis.conf
# Redis 监听端口
port 6379
# 监听所有网卡，允许其他节点连接
bind 0.0.0.0
# 关闭保护模式
protected-mode no
# 开启 AOF 持久化
appendonly yes
# 开启 Redis Cluster 模式
cluster-enabled yes
# 保存集群节点信息的配置文件
cluster-config-file nodes.conf
# 集群节点故障判断超时时间，5000ms = 5秒
cluster-node-timeout 5000
# 当前节点对外公布的 IP 地址
cluster-announce-ip 172.30.0.10${port}
# Redis 服务端口
cluster-announce-port 6379
# Cluster Bus 集群内部通信端口，客户端口和访问端口区分开来
cluster-announce-bus-port 16379
EOF
done

# 生成后2个扩容节点配置
for port in $(seq 10 11); \
do \
mkdir -p redis${port}/; \
touch redis${port}/redis.conf
cat << EOF > redis${port}/redis.conf
port 6379
bind 0.0.0.0
protected-mode no
appendonly yes
cluster-enabled yes
cluster-config-file nodes.conf
cluster-node-timeout 5000
cluster-announce-ip 172.30.0.1${port}
cluster-announce-port 6379
cluster-announce-bus-port 16379
EOF
done
