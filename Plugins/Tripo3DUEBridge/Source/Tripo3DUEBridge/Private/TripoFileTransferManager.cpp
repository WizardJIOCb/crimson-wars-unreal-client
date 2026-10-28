// Copyright Tripo3D. All Rights Reserved.

#include "TripoFileTransferManager.h"
#include "TripoWebSocketServer.h"
#include "TripoModelImporter.h"
#include "Async/Async.h"

FTripoFileTransferManager::FTripoFileTransferManager()
{
}

FTripoFileTransferManager::~FTripoFileTransferManager()
{
	ClearAllSessions();
}

bool FTripoFileTransferManager::ProcessChunk(
	const FString& FileId,
	const FString& FileName,
	const FString& FileType,
	int32 ChunkIndex,
	int32 ChunkTotal,
	const TArray<uint8>& ChunkData)
{
	FScopeLock Lock(&SessionLock);

	// Get or create session
	FFileTransferSession* Session = ActiveSessions.Find(FileId);
	if (!Session)
	{
		FFileTransferSession NewSession;
		NewSession.FileId = FileId;
		NewSession.FileName = FileName;
		NewSession.FileType = FileType;
		NewSession.TotalChunks = ChunkTotal;
		
		Session = &ActiveSessions.Add(FileId, NewSession);
		
		UE_LOG(LogTemp, Log, TEXT("Started new file transfer session: %s (%d chunks)"), 
			*FileName, ChunkTotal);
	}

	// Validate chunk index
	if (ChunkIndex < 0 || ChunkIndex >= ChunkTotal)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid chunk index %d for file %s (total: %d)"),
			ChunkIndex, *FileId, ChunkTotal);
		return false;
	}

	// Check if chunk already received
	if (Session->HasChunk(ChunkIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("Chunk %d already received for file %s"), 
			ChunkIndex, *FileId);
		return true; // Not an error, just duplicate
	}

	// Store chunk data
	Session->Chunks.Add(ChunkIndex, ChunkData);

	float Progress = ((ChunkIndex + 1) * 100.0f) / ChunkTotal;
	UE_LOG(LogTemp, Log, TEXT("[Tripo3D Server] Received chunk %d/%d for %s (%.2f MB) - Progress: %.1f%%"),
		ChunkIndex + 1, ChunkTotal, *FileName, ChunkData.Num() / 1024.0f / 1024.0f, Progress);

	// Check if all chunks received
	if (Session->IsComplete())
	{
		UE_LOG(LogTemp, Log, TEXT("[Tripo3D Server] ✓ All %d chunks received for %s, assembling file..."), ChunkTotal, *FileName);

		// Send file transfer complete message
		FTripoWebSocketServer::Get().SendFileTransferComplete(
			FileId,
			TEXT("importing"),
			FString::Printf(TEXT("File transfer complete, importing %s..."), *FileName)
		);

		// Assemble file
		TArray<uint8> FileData;
		if (AssembleFile(*Session, FileData))
		{
			// Trigger import on next game thread tick
			TriggerImport(FileId, FileName, FileType, FileData);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to assemble file %s"), *FileId);
			
			FTripoWebSocketServer::Get().SendImportComplete(
				FileId,
				false,
				TEXT("Failed to assemble file chunks")
			);
			
			ClearSession(FileId);
			return false;
		}
	}

	return true;
}

bool FTripoFileTransferManager::AssembleFile(FFileTransferSession& Session, TArray<uint8>& OutFileData)
{
	// Calculate total size
	int32 TotalSize = 0;
	for (const auto& ChunkPair : Session.Chunks)
	{
		TotalSize += ChunkPair.Value.Num();
	}

	UE_LOG(LogTemp, Log, TEXT("Assembling file %s: %d chunks, %.2f MB total"),
		*Session.FileName, Session.TotalChunks, TotalSize / 1024.0f / 1024.0f);

	// Reserve space
	OutFileData.Reset(TotalSize);
	OutFileData.AddUninitialized(TotalSize);

	// Copy chunks in order
	int32 Offset = 0;
	for (int32 i = 0; i < Session.TotalChunks; ++i)
	{
		const TArray<uint8>* ChunkData = Session.Chunks.Find(i);
		if (!ChunkData)
		{
			UE_LOG(LogTemp, Error, TEXT("Missing chunk %d/%d for file %s"),
				i, Session.TotalChunks, *Session.FileName);
			return false;
		}

		FMemory::Memcpy(OutFileData.GetData() + Offset, ChunkData->GetData(), ChunkData->Num());
		Offset += ChunkData->Num();
	}

	UE_LOG(LogTemp, Log, TEXT("Successfully assembled file %s"), *Session.FileName);
	return true;
}

void FTripoFileTransferManager::TriggerImport(
	const FString& FileId,
	const FString& FileName,
	const FString& FileType,
	const TArray<uint8>& FileData)
{
	// Marshal everything to the game thread — GEditor and UI delegates require it
	AsyncTask(ENamedThreads::GameThread, [FileId, FileName, FileType, FileData]()
	{
		// Notify UI that file has been received
		FTripoWebSocketServer::Get().OnMessageReceived.Broadcast(
			FString::Printf(TEXT("Received %s (%.2f MB), starting import..."), 
				*FileName, FileData.Num() / 1024.0f / 1024.0f)
		);

		// Use timer to execute import WITHOUT AsyncTask recursion
		// This avoids task graph recursion errors when ImportAssetTasks is called
		GEditor->GetTimerManager()->SetTimerForNextTick([FileId, FileName, FileType, FileData]()
		{
			UE_LOG(LogTemp, Log, TEXT("Starting import for %s on game thread (via Timer)"), *FileName);

			// Import the model
			bool bSuccess = FTripoModelImporter::ImportModel(FileId, FileName, FileType, FileData);

			// Send import complete message
			FString ImportResultMsg = bSuccess
				? FString::Printf(TEXT("Import complete: %s"), *FileName)
				: FString::Printf(TEXT("Import failed: %s"), *FileName);

			FTripoWebSocketServer::Get().OnMessageReceived.Broadcast(ImportResultMsg);
			FTripoWebSocketServer::Get().SendImportComplete(
				FileId,
				bSuccess,
				ImportResultMsg
			);

			// Clear session after import
			FTripoWebSocketServer::Get().FileTransferManager->ClearSession(FileId);
		});
	});
}

void FTripoFileTransferManager::ClearAllSessions()
{
	FScopeLock Lock(&SessionLock);
	ActiveSessions.Empty();
	UE_LOG(LogTemp, Log, TEXT("Cleared all file transfer sessions"));
}

void FTripoFileTransferManager::ClearSession(const FString& FileId)
{
	FScopeLock Lock(&SessionLock);
	
	if (ActiveSessions.Remove(FileId) > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Cleared file transfer session: %s"), *FileId);
	}
}
