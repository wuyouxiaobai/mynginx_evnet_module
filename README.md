# MyNginx Event-Driven Server

## 项目简介
MyNginx 是一个基于事件驱动的高性能服务器，使用 C/C++ 编写，旨在提供一个轻量级、高效的网络服务器解决方案。该项目支持多进程、多线程处理，并提供了丰富的配置选项，以满足不同场景下的需求。

## 主要特性
- **多进程模型**：支持多个 worker 进程，充分利用多核 CPU 性能。
- **事件驱动**：基于 epoll 实现高效的事件处理机制，支持高并发连接。
- **线程池**：使用线程池处理接收到的消息，提高处理效率。
- **守护进程模式**：支持以守护进程方式运行，适合生产环境部署。
- **日志系统**：支持多级别日志输出，便于调试和监控。
- **网络安全**：内置 Flood 攻击检测机制，增强服务器安全性。

## 配置文件说明
配置文件 `nginx.conf` 位于 `config` 目录下，主要配置项包括日志设置、进程设置、网络设置及网络安全设置。

## 框架运行流程
client -> server  
- 对应 worker 通过 epoll 得知哪些事件触发，主线程调用 `ngx_event_accept`（接受连接，并分配 Conn）和 `ngx_read_request_handler`（解析消息，用 Conn 的 precvbuf 来暂存接收的数据，即解析的消息会暂存在连接池分配的连接缓存中。 psendMemPointer 用来释放缓存）。
- 将相应事件的消息放入 worker 拥有的 recv 队列。
- thread pool 从 recv 队列中取出数据，解析后调用对应的回调函数。
- 回调函数将数据放入 send 队列（m_MsgSendQueue）。
- worker 中的发送线程将 send 队列中的数据记录在 Conn 的 psendbuf 中（psendMemPointer 用来释放分配的缓存），然后发送给 client。
- （如果没有发送完，会注册到 epoll 上，使用默认的LT（避免漏掉一些数据），当发送缓冲区可写时，主线程调用 `ngx_write_response_handler` 继续发送。）

## 心跳检测和超时踢人
时间队列（时间，LPSTRUC_MSG_HEADER）
- ServerTimerQueueMonitorThread持续监听。
- 超时后会更新通过判断（Sock_TimeOutKick）是否超时就踢，不是就会更新时间（currTime + m_iWaitTime）并重新入时间队列，同时统计超时队列。
- 通过procPingTimeOutChecking检测超时队列中的Conn具体多久没有收到心跳包了，如果超过m_iWaitTime*3+10，就会踢人，并更新时间队列。
- GetOneToUse分配连接时会统计一次lastPingTime，之后通过注册的业务函数HandlePing来继续统计lastPingTime。


## flood检测


## 线程类型
1、线程池，负责处理接收队列消息，并调用对应回调函数。
2、发送线程，负责将消息从发送队列中取出，并发送给客户端。
3、回收线程，负责回收连接池中的连接。
4、心跳检测和超时踢人线程，负责检测连接是否超时，并踢人。（ServerTimerQueueMonitorThread）
5、

## 队列类型
1、接受数据队列
2、发送数据队列
3、回收连接队列

## 惊群问题
用linux的地址重用和端口重用，将新链接分配交给操作系统。

## 连接池回收
- 1、超时踢人回收 DeleteFromTimerQueue删除时间队列中的连接，把回收连接加入回收队列inRecyConnectQueue，等待Sock_RecyConnectionWaitTime超时后释放连接

- 2、客户端断开连接后回收？？？？？？？？？？？

- ServerRecyConnectionThread连接池回收线程回收连接，首先++iCurrsequence控制连接序号，然后通过psendMemPointer和precvMemPointer释放接受和发送数据，关闭fd，然后放回连接池。


## 退出
kill 主进程pid


## 问题？？？？？
1、多连接时出错，会中断其他连接
2、客户端断开连接后回收
3、iThrowsendCount用来控制客户端断开？，
4、iThrowsendCount的作用
nginx: ------------------------------------begin--------------------------------------
nginx: 当前在线人数/总人数(2/2048)。
nginx: 连接池中空闲连接/总连接/要释放的连接(2046/2048/0)。
nginx: 当前时间队列大小(1)。
nginx: 当前收消息队列/发消息队列大小分别为(0/0)，丢弃的待发送数据包数量为0。
nginx: ------------------------------------end--------------------------------------