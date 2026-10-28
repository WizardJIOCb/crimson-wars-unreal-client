// Copyright Tripo3D. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * WebSocket 消息类型
 */
enum class ETripoWebSocketMessageType : uint8
{
	Open,       // 连接已建立
	Close,      // 连接已关闭
	Message,    // 收到消息（文本或二进制）
	Error       // 发生错误
};

/**
 * WebSocket 关闭信息
 */
struct FTripoWebSocketCloseInfo
{
	int32 Code = 0;
	FString Reason;
};

/**
 * WebSocket 消息数据
 */
struct FTripoWebSocketMessage
{
	ETripoWebSocketMessageType Type;
	TArray<uint8> Data;           // 消息数据（文本或二进制）
	bool bIsBinary = false;       // 是否为二进制消息
	FTripoWebSocketCloseInfo CloseInfo;  // 关闭时的信息
	FString ErrorMessage;         // 错误信息
};

/**
 * WebSocket 连接接口
 * 抽象单个客户端连接
 */
class TRIPO3DUEBRIDGE_API ITripoWebSocketConnection
{
public:
	virtual ~ITripoWebSocketConnection() = default;

	/** 发送文本消息 */
	virtual void SendText(const FString& Message) = 0;

	/** 发送二进制消息 */
	virtual void SendBinary(const TArray<uint8>& Data) = 0;

	/** 发送组合消息（JSON + 二进制数据） */
	virtual void SendBinary(const FString& JsonMessage, const TArray<uint8>& BinaryData) = 0;

	/** 获取远程 IP 地址 */
	virtual FString GetRemoteIp() const = 0;

	/** 关闭连接 */
	virtual void Close() = 0;
};

/**
 * WebSocket 服务器回调委托
 */
DECLARE_DELEGATE_ThreeParams(FOnTripoWebSocketMessage, 
	const FString& /* ClientId */, 
	TSharedPtr<ITripoWebSocketConnection> /* Connection */,
	const FTripoWebSocketMessage& /* Message */);

/**
 * WebSocket 服务器接口
 * 抽象 WebSocket 服务器实现，便于替换底层库
 */
class TRIPO3DUEBRIDGE_API ITripoWebSocketServer
{
public:
	virtual ~ITripoWebSocketServer() = default;

	/** 启动服务器监听 */
	virtual bool Start(int32 Port, const FString& Host = TEXT("127.0.0.1")) = 0;

	/** 停止服务器 */
	virtual void Stop() = 0;

	/** 检查服务器是否正在运行 */
	virtual bool IsRunning() const = 0;

	/** 设置消息回调 */
	virtual void SetOnMessage(FOnTripoWebSocketMessage Callback) = 0;

	/** 获取监听端口 */
	virtual int32 GetPort() const = 0;

	/** 获取监听主机 */
	virtual FString GetHost() const = 0;
};

/**
 * WebSocket 工厂类
 * 用于创建 WebSocket 服务器实例，隔离实现细节
 */
class TRIPO3DUEBRIDGE_API FTripoWebSocketFactory
{
public:
	/** 创建 WebSocket 服务器实例 */
	static TSharedPtr<ITripoWebSocketServer> CreateServer();
};
