// Copyright Tripo3D. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ITripoWebSocket.h"

// IXWebSocket includes - need full definition for callback signatures
#include "IXWebSocketServer.h"
#include "IXWebSocket.h"
#include "IXConnectionState.h"

/**
 * IXWebSocket 连接实现
 */
class FTripoWebSocketConnectionImpl : public ITripoWebSocketConnection, public TSharedFromThis<FTripoWebSocketConnectionImpl>
{
public:
	FTripoWebSocketConnectionImpl(ix::WebSocket* InWebSocket, const FString& InRemoteIp);
	virtual ~FTripoWebSocketConnectionImpl() override;

	// ITripoWebSocketConnection interface
	virtual void SendText(const FString& Message) override;
	virtual void SendBinary(const TArray<uint8>& Data) override;
	virtual void SendBinary(const FString& JsonMessage, const TArray<uint8>& BinaryData) override;
	virtual FString GetRemoteIp() const override;
	virtual void Close() override;

	/** 检查连接是否有效 */
	bool IsValid() const { return WebSocket != nullptr; }

	/** 使连接无效（当连接关闭时调用） */
	void Invalidate() { WebSocket = nullptr; }

private:
	ix::WebSocket* WebSocket;
	FString RemoteIp;
};

/**
 * IXWebSocket 服务器实现
 */
class FTripoWebSocketServerImpl : public ITripoWebSocketServer
{
public:
	FTripoWebSocketServerImpl();
	virtual ~FTripoWebSocketServerImpl() override;

	// ITripoWebSocketServer interface
	virtual bool Start(int32 Port, const FString& Host = TEXT("127.0.0.1")) override;
	virtual void Stop() override;
	virtual bool IsRunning() const override;
	virtual void SetOnMessage(FOnTripoWebSocketMessage Callback) override;
	virtual int32 GetPort() const override { return ListenPort; }
	virtual FString GetHost() const override { return ListenHost; }

private:
	/** 内部消息处理回调 */
	void OnClientMessage(
		std::shared_ptr<ix::ConnectionState> ConnectionState,
		ix::WebSocket& WebSocket,
		const ix::WebSocketMessagePtr& Msg);

	/** 查找或创建连接包装 */
	TSharedPtr<FTripoWebSocketConnectionImpl> GetOrCreateConnection(ix::WebSocket* WebSocket, const FString& RemoteIp);

	/** 移除连接包装 */
	void RemoveConnection(ix::WebSocket* WebSocket);

	/** 生成客户端 ID */
	FString GenerateClientId(const FString& RemoteIp);

private:
	ix::WebSocketServer* Server;
	int32 ListenPort;
	FString ListenHost;
	bool bIsRunning;

	/** 消息回调 */
	FOnTripoWebSocketMessage OnMessageCallback;

	/** 活跃连接映射 (WebSocket 指针 -> 连接包装) */
	TMap<ix::WebSocket*, TSharedPtr<FTripoWebSocketConnectionImpl>> Connections;

	/** 客户端 ID 映射 */
	TMap<ix::WebSocket*, FString> ClientIds;

	/** 线程安全锁 */
	FCriticalSection ConnectionsLock;
};
