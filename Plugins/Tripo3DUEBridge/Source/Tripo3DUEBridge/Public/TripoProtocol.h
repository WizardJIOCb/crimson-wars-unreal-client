// Copyright Tripo3D. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TripoProtocol.generated.h"

/**
 * Protocol constants for Tripo3D WebSocket communication
 */
namespace TripoProtocol
{
	// WebSocket connection settings
	static const FString SERVER_HOST = TEXT("127.0.0.1");
	static const int32 SERVER_PORT = 60620;
	static const FString PROTOCOL_VERSION = TEXT("1.0.0");

	// Message types
	namespace MessageType
	{
		static const FString MSG_HANDSHAKE = TEXT("handshake");
		static const FString MSG_HANDSHAKE_ACK = TEXT("handshake_ack");
		static const FString MSG_PING = TEXT("ping");
		static const FString MSG_PONG = TEXT("pong");
		static const FString MSG_FILE_TRANSFER = TEXT("file_transfer");
		static const FString MSG_FILE_TRANSFER_ACK = TEXT("file_transfer_ack");
		static const FString MSG_FILE_TRANSFER_COMPLETE = TEXT("file_transfer_complete");
		static const FString MSG_IMPORT_COMPLETE = TEXT("import_complete");
	}

	// Error codes
	namespace ErrorCode
	{
		static const int32 SUCCESS = 0;
		static const int32 ERROR_INVALID_JSON = 1001;
		static const int32 ERROR_PROCESSING = 1002;
	}

	// File transfer settings
	static const int32 CHUNK_SIZE = 5 * 1024 * 1024; // 5MB
	static const int32 MAX_RETRIES = 5;
	static const float RETRY_DELAY = 0.1f; // 100ms

	// Asset paths
	static const FString ASSET_IMPORT_ROOT = TEXT("/Game/TripoModels");
	static const FString TEMP_DIR_NAME = TEXT("Tripo3D");
}

/**
 * Message structures for JSON serialization
 */

// Handshake message from client
USTRUCT()
struct FTripoHandshakePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString clientName;

	UPROPERTY()
	FString protocolVersion;
};

// Handshake acknowledgment from server
USTRUCT()
struct FTripoHandshakeAckPayload
{
	GENERATED_BODY()

	UPROPERTY()
	bool success = false;

	UPROPERTY()
	FString clientName;

	UPROPERTY()
	FString dccVersion;

	UPROPERTY()
	FString pluginVersion;

	UPROPERTY()
	FString protocolVersion;
};

// File transfer message payload
USTRUCT()
struct FTripoFileTransferPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString fileId;

	UPROPERTY()
	FString fileName;

	UPROPERTY()
	FString fileType;

	UPROPERTY()
	int32 chunkIndex = 0;

	UPROPERTY()
	int32 chunkTotal = 0;

	UPROPERTY()
	int32 chunkSize = 0;
};

// File transfer acknowledgment payload
USTRUCT()
struct FTripoFileTransferAckPayload
{
	GENERATED_BODY()

	UPROPERTY()
	bool success = false;

	UPROPERTY()
	FString fileId;

	UPROPERTY()
	int32 fileIndex = 0;

	UPROPERTY()
	int32 code = 0;
};

// File transfer complete payload
USTRUCT()
struct FTripoFileTransferCompletePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString fileId;

	UPROPERTY()
	FString status;

	UPROPERTY()
	FString message;
};

// Import complete payload
USTRUCT()
struct FTripoImportCompletePayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString fileId;

	UPROPERTY()
	bool success = false;

	UPROPERTY()
	FString message;
};
