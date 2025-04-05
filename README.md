# HttpServer

## 项目简介
HttpServer 基于对 Nginx 事件模块的学习和 HTTP 协议栈的实践需求，从零构建了一个高性能、可扩展的 C++ 多线程高并发服务器框架，并在此框架基础上开发了自定义 HTTP 服务框架。框架底层采用 主+从多进程模式 与 epoll 事件驱动机制，配合用户连接池、线程池等池化技术，上层支持HTTP/HTTPS 协议、动态路由、会话管理等应用层功能，形成了从网络通信到业务逻辑的完整技术栈。用户可直接基于框架开发 Web 应用。

## 主要特性
- **多进程模型**：支持多个 worker 进程，充分利用多核 CPU 性能。
- **事件驱动**：基于 epoll 实现高效的事件处理机制，支持高并发连接。
- **线程池**：使用线程池处理接收到的消息，提高处理效率。
- **守护进程模式**：支持以守护进程方式运行，适合生产环境部署。
- **日志系统**：支持多级别日志输出，便于调试和监控。
。

## 配置文件说明
配置文件 `nginx.conf` 位于 `config` 目录下，主要配置项包括日志设置、进程设置、网络设置及网络安全设置。

## 框架运行流程
1. **浏览器连接**：浏览器发起连接请求。
2. **事件监听**：主线程通过 `epoll` 监听事件，调用 `ngx_event_accept` 接受连接并分配 `Conn` 结构体。
3. **消息入队**：主线程调用 `ngx_http_read_request_handler` 将消息放入 worker 的 `recv` 队列。
3. **消息解析**：线程池线程调用 `parseRequest` 解析消息，并将解析后的消息暂存在 `Conn` 中。
4. **消息处理**：线程池线程通过注册的路由和对应回调处理消息。
5. **消息发送**：处理后的消息放入 `send` 队列，发送线程从队列中取出数据并发送给浏览器。
6. **发送完成**：如果数据未发送完，注册到 `epoll` 上，当发送缓冲区可写时继续发送。

## 超时踢人
- **时间队列**：`ServerTimerQueueMonitorThread` 持续监听时间队列，判断连接是否超时。
- **超时处理**：如果连接超时，更新连接时间并重新入队；如果超过最大等待时间，则踢出连接。


## 线程类型
1. **线程池**：负责处理接收队列消息，并调用对应回调函数。
2. **发送线程**：负责将消息从发送队列中取出并发送给客户端。
3. **回收线程**：负责回收连接池中的连接。


## 队列类型
1. **接收数据队列**：存放待处理的消息。
2. **发送数据队列**：存放待发送的消息。
3. **回收连接队列**：存放待回收的连接。

## 惊群问题
通过 Linux 的地址重用和端口重用，将新连接分配交给操作系统处理，避免惊群问题。

## 连接池回收
1. **超时踢人回收**：将超时连接从时间队列中删除，加入回收队列，等待超时后释放连接。
2. **客户端断开回收**：`epoll` 调用 `EPOLLIN` 的回调函数，将连接加入回收队列，等待超时后释放连接。
3. **回收线程**：回收线程负责释放连接的接收和发送缓冲区，关闭文件描述符，并将连接放回连接池。

## 退出
通过 `mynginx-quit.sh` 脚本优雅退出整个程序。


## 线程池
- **线程封装**：`ThreadItem` 类封装线程，记录线程 ID。
- **线程管理**：使用 `std::vector<ThreadItem*>` 容器存储线程。
- **队列优化**： 用无锁队列代替原来的容器+锁的形式，提高并发能力

## work进程中主线程解包负载过大
- 增加work进程数量，减少每个work的连接量
- 如果recv队列滞留消息过多，或者某个连接待处理消息过多，后序的包就会丢弃不再解包。


## 待解决问题
- 能否通过配置文件热重载项目。
- 应用层视频在线播放卡顿
- ffmppeg解码分片后只有视频，没有音频
- 用sendfile上传数据
