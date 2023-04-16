#ifndef __APOLLO_CALLBACKS_H__
#define __APOLLO_CALLBACKS_H__

#include <functional>
#include <memory>

namespace apollo {

class TcpConnection;
class Buffer;
class Timestamp;

using TcpConnectionPtr      = std::shared_ptr<TcpConnection>;
using ConnectionCallback    = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback         = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback       = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
} // namespace apollo

#endif // __APOLLO_CALLBACKS_H__