// ConsoleApplication20.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string.h>
#include <stdlib.h>
#include "XNFS-SCE.h"

#define PRINTTYPEINFO "INFO:"
#define PRINTTYPEERROR "ERROR:"
#define PRINTTYPEWARNING "WARNING:"

bool bCombinerMode = 0;

XNFSSCE_API StreamInfo CreateStreamInfoBuffer(unsigned int InfoCount)
{
	return (StreamInfo)malloc(sizeof(StreamInfoStruct) * InfoCount);
}

// Search for aligned chunks by type, returns the size of the chunk, OffsetOut is the place where it finds it
XNFSSCE_API unsigned int SearchAlignedChunkByType(const char* Filename, unsigned int ChunkMagic, long &OffsetOut)
{
	FILE *fin = fopen(Filename, "rb");

	if (fin == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, Filename);
		perror("ERROR");
		return 0;
	}

	unsigned int ReadMagic = 0;
	unsigned int ReadSize = 0;
	while (!feof(fin))
	{
		fread(&ReadMagic, 4, 1, fin);
		fread(&ReadSize, 4, 1, fin);
		if (ReadSize && (ReadMagic == ChunkMagic))
		{
			OffsetOut = ftell(fin) - 8;
			fclose(fin);
			return ReadSize;
		}
		if (ReadSize)
			fseek(fin, ReadSize, SEEK_CUR);
	}
	fclose(fin);
	return ReadSize;
}

XNFSSCE_API unsigned int GetInfoCount(unsigned int InfoChunkSize)
{
	return InfoChunkSize / sizeof(StreamInfoStruct);
}

// Reads StreamInfo out of a LocBundle file
// TheStreamInfo - The output StreamInfo
// InfoCount - count of StreamInfos to read
// InfoChunkAddress - The address of the info chunk
// LocBundleFilename - The location bundle file path
XNFSSCE_API StreamInfo StreamInfoReader(unsigned int InfoCount, long InfoChunkAddress, const char* LocBundleFilename)
{
	FILE *fin = fopen(LocBundleFilename, "rb");
	char LocationName[255];
	const char* BundleName = strrchr(LocBundleFilename, '\\') + 1;
	StreamInfo TheStreamInfo = CreateStreamInfoBuffer(InfoCount);

	if (fin == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, LocBundleFilename);
		perror("ERROR");
		return 0;
	}

	strncpy(LocationName, BundleName, strrchr(BundleName, '.') + 1 - BundleName);
	LocationName[strrchr(BundleName, '.') - BundleName] = 0;

	fseek(fin, InfoChunkAddress + 8, SEEK_SET);

	if (!bCombinerMode)
	{
		char LogFileName[255];
		sprintf(LogFileName, "%s.txt", LocationName);
		FILE *logfile = fopen(LogFileName, "w");
		if (!logfile || logfile == NULL)
		{
			perror("ERROR");
			printf("%s Can't create log file, printing to stdout...", PRINTTYPEWARNING);
			logfile = stdout;
		}

		for (unsigned int i = 0; i < InfoCount; i++)
		{
			fread(&TheStreamInfo[i], sizeof(StreamInfoStruct), 1, fin);
			fprintf(logfile, "===INFO %d START===\n", i);
			fprintf(logfile, "Model group name: %s\n", TheStreamInfo[i].ModelGroupName);
			fprintf(logfile, "Stream chunk number: %d\n", TheStreamInfo[i].StreamChunkNumber);
			fprintf(logfile, "Unk2: %x\n", TheStreamInfo[i].Unk2);
			fprintf(logfile, "Master stream chunk number: %d\n", TheStreamInfo[i].MasterStreamChunkNumber);
			fprintf(logfile, "Master stream chunk offset: %x\n", TheStreamInfo[i].MasterStreamChunkOffset);
			fprintf(logfile, "Size1: %x\n", TheStreamInfo[i].Size1);
			fprintf(logfile, "Size2: %x\n", TheStreamInfo[i].Size2);
			fprintf(logfile, "Size3: %x\n", TheStreamInfo[i].Size3);
			fprintf(logfile, "Unk3: %x\n", TheStreamInfo[i].Unk3);
			fprintf(logfile, "X: %f\n", TheStreamInfo[i].X);
			fprintf(logfile, "Y: %f\n", TheStreamInfo[i].Y);
			fprintf(logfile, "Z: %f\n", TheStreamInfo[i].Z);
			fprintf(logfile, "StreamChunkHash: %x\n", TheStreamInfo[i].StreamChunkHash);
			fprintf(logfile, "====INFO %d END====\n\n", i);
		}
	}
	else
		for (unsigned int i = 0; i < InfoCount; i++)
			fread(&TheStreamInfo[i], sizeof(StreamInfoStruct), 1, fin);

	fclose(fin);
	return TheStreamInfo;
}

XNFSSCE_API bool ExtractAllStreamParts(StreamInfo TheStreamInfo, unsigned int InfoCount, const char* StreamFilename, const char* OutFolderPath)
{
	void *partbuffer;

	char* StreamPartName = (char*)malloc(strlen(strrchr(StreamFilename, '\\') + 1) + 5);
	char* OutFilename = (char*)malloc(strlen(OutFolderPath) + strlen(strrchr(StreamFilename, '\\') + 1) + 5);

	FILE *fout;
	printf("%s Opening file: %s\n", PRINTTYPEINFO, StreamFilename);
	FILE *fin = fopen(StreamFilename, "rb");
	if (fin == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, StreamFilename);
		perror("ERROR");
		return 0;
	}
	for (unsigned int i = 0; i < InfoCount; i++)
	{
		strcpy(StreamPartName, strrchr(StreamFilename, '\\') + 1);
		sprintf(strrchr(StreamPartName, '.'), "_%d.BUN", TheStreamInfo[i].StreamChunkNumber);
		sprintf(OutFilename, "%s\\%s", OutFolderPath, StreamPartName);

		fout = fopen(OutFilename, "wb");
		if (fout == NULL)
		{
			printf("%s Can't open file: %s\n", PRINTTYPEERROR, OutFilename);
			perror("ERROR");
			return 0;
		}

		printf("%s Writing: %s\n", PRINTTYPEINFO, OutFilename);

		partbuffer = malloc(TheStreamInfo[i].Size1);
		fseek(fin, TheStreamInfo[i].MasterStreamChunkOffset, SEEK_SET);
		fread(partbuffer, 1, TheStreamInfo[i].Size1, fin);
		fwrite(partbuffer, 1, TheStreamInfo[i].Size1, fout);
		free(partbuffer);
		fclose(fout);
	}
	fclose(fin);
	return 1;
}

XNFSSCE_API bool ExtractStreamPartByNumber(unsigned int StreamPartNumber, StreamInfo TheStreamInfo, unsigned int InfoCount, const char* StreamFilename, const char* OutFolderPath)
{
	void *partbuffer;

	char* StreamPartName = (char*)malloc(strlen(strrchr(StreamFilename, '\\') + 1) + 5);
	char* OutFilename = (char*)malloc(strlen(OutFolderPath) + strlen(strrchr(StreamFilename, '\\') + 1) + 5);

	FILE *fin = fopen(StreamFilename, "rb");
	printf("%s Opening file: %s\n", PRINTTYPEINFO, StreamFilename);
	if (fin == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, StreamFilename);
		perror("ERROR");
		return 0;
	}

	strcpy(StreamPartName, strrchr(StreamFilename, '\\') + 1);
	sprintf(strrchr(StreamPartName, '.'), "_%d.BUN", StreamPartNumber);
	sprintf(OutFilename, "%s\\%s", OutFolderPath, StreamPartName);

	FILE *fout = fopen(OutFilename, "wb");
	if (fout == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, OutFilename);
		perror("ERROR");
		return 0;
	}

	for (unsigned int i = 0; i < InfoCount; i++)
	{
		if (TheStreamInfo[i].StreamChunkNumber == StreamPartNumber)
		{
			partbuffer = malloc(TheStreamInfo[i].Size1);
			fseek(fin, TheStreamInfo[i].MasterStreamChunkOffset, SEEK_SET);
			fread(partbuffer, 1, TheStreamInfo[i].Size1, fin);
			fwrite(partbuffer, 1, TheStreamInfo[i].Size1, fout);
			free(partbuffer);
			fclose(fout);
			fclose(fin);
			return 1;
		}
	}
	fclose(fout);
	fclose(fin);
	return 0;
}

XNFSSCE_API int ExtractChunks(const char* LocBundle, const char* OutPath)
{
	const char* BundleName = strrchr(LocBundle, '\\') + 1;
	
	char* InputPath;

	char* FullStreamPath;
	char StreamName[255];
	char LocationName[255];

	unsigned int SizeOfInfoChunk = 0;
	unsigned int InfoCount = 0;
	long InfoChunkOffset = 0;

	StreamInfo ExtractorStreamInfo;

	printf("%s Opening %s\n", PRINTTYPEINFO, LocBundle);
	FILE *fin = fopen(LocBundle, "rb");
	if (fin == NULL)
	{
		printf("%s Can't open file: %s\n", PRINTTYPEERROR, LocBundle);
		perror("ERROR");
		return -1;
	}
	fclose(fin);

	printf("%s Bundle name: %s\n", PRINTTYPEINFO, BundleName);
	strncpy(LocationName, BundleName, strrchr(BundleName, '.') + 1 - BundleName);
	LocationName[strrchr(BundleName, '.') - BundleName] = 0;
	printf("%s Location name: %s\n", PRINTTYPEINFO, LocationName);

	SizeOfInfoChunk = SearchAlignedChunkByType(LocBundle, STREAMINFOCHUNK, InfoChunkOffset);
	printf("%s Found at %X, size %X\n", PRINTTYPEINFO, InfoChunkOffset, SizeOfInfoChunk);
	InfoCount = GetInfoCount(SizeOfInfoChunk);
	printf("Total info count = %d\n", InfoCount);
	//ExtractorStreamInfo = CreateStreamInfoBuffer(InfoCount);
	ExtractorStreamInfo = StreamInfoReader(InfoCount, InfoChunkOffset, LocBundle);
	
	if (OutPath != NULL)
	{
		sprintf(StreamName, "STREAM%s.BUN", LocationName);

		InputPath = (char*)malloc(BundleName - LocBundle + 1);
		InputPath[BundleName - LocBundle] = 0;
		strncpy(InputPath, LocBundle, BundleName - LocBundle);

		FullStreamPath = (char*)malloc(strlen(StreamName) + strlen(InputPath) - 2);
		sprintf(FullStreamPath, "%s%s", InputPath, StreamName);

		free(InputPath);

		printf("%s Extracting stream chunks to %s\n", PRINTTYPEINFO, OutPath);

		if (ExtractAllStreamParts(ExtractorStreamInfo, InfoCount, FullStreamPath, OutPath))
			printf("%s Extraction finished successfuly!\n", PRINTTYPEINFO);
		else
			printf("%s Extraction finished with errors.\n", PRINTTYPEWARNING);
	}

	return 0;
}

XNFSSCE_API int WriteChunkTypeAndSize(FILE *fout, unsigned int ChunkMagic, unsigned int ChunkSize)
{
	fwrite(&ChunkMagic, 4, 1, fout);
	fwrite(&ChunkSize, 4, 1, fout);
	return 1;
}

XNFSSCE_API int ZeroChunkWriter(FILE *fout, unsigned int ChunkSize)
{
	if (ChunkSize)
	{
		WriteChunkTypeAndSize(fout, 0, ChunkSize);
		printf("%s Zero chunk: writing %X bytes\n", PRINTTYPEINFO, ChunkSize);

		for (unsigned int i = 0; i <= ChunkSize - 1; i++)
			fputc(0, fout);
	}
	return 1;
}

// Combines stream parts back
// LocBundle - The original location bundle path (e.g. L5RA.BUN)
// StreamChunkPaths - Stream parts folder path
// OutPath - Output folder of the combined stream and new LocBundle
XNFSSCE_API int CombineChunks(const char* LocBundle, const char* StreamChunkPaths, const char* OutPath)
{
	// fin = The original LocBundle
	// fout1 = The master stream
	// fout2 = The new LocBundle

	char BundleName[255];
	char LocationName[255];
	unsigned int InfoCount = 0;

	unsigned int SizeOfInfoChunk = 0;
	long InfoChunkOffset = 0;
	char TotalStreamChunkPath[2048];
	char TotalOutPath[2048];
	char StreamName[512];
	void *chunkbuffer;
	StreamInfo CombinerStreamInfo;
	struct stat st;

	bCombinerMode = 1;
	printf("%s Opening %s\n", PRINTTYPEINFO, LocBundle);
	FILE *fin = fopen(LocBundle, "rb");
	if (fin == NULL)
	{
		printf("Error opening %s\n", LocBundle);
		perror("ERROR");
		bCombinerMode = 0;
		return -1;
	}

	strcpy(BundleName, strrchr(LocBundle, '\\') + 1);
	printf("%s Bundle name: %s\n", PRINTTYPEINFO, BundleName);
	strncpy(LocationName, BundleName, strrchr(BundleName, '.') + 1 - BundleName);
	LocationName[strrchr(BundleName, '.') - BundleName] = 0;
	printf("%s Location name: %s\n", PRINTTYPEINFO, LocationName);

	stat(LocBundle, &st);

	strcpy(TotalOutPath, OutPath);
	strcat(TotalOutPath, "\\");
	strcat(TotalOutPath, strrchr(LocBundle, '\\'));
	FILE *fout2 = fopen(TotalOutPath, "wb");
	if (fout2 == NULL)
	{
		printf("Error opening %s\n", TotalOutPath);
		perror("ERROR");
		bCombinerMode = 0;
		return -1;
	}

	// copy the main locbundle file
	chunkbuffer = malloc(st.st_size);
	fread(chunkbuffer, 1, st.st_size, fin);
	fwrite(chunkbuffer, 1, st.st_size, fout2);
	free(chunkbuffer);
	fclose(fin);

	SizeOfInfoChunk = SearchAlignedChunkByType(LocBundle, STREAMINFOCHUNK, InfoChunkOffset);
	printf("%s Found at %X, size %X\n", PRINTTYPEINFO, InfoChunkOffset, SizeOfInfoChunk);
	InfoCount = GetInfoCount(SizeOfInfoChunk);
	printf("Total info count = %d\n", InfoCount);
	//CombinerStreamInfo = CreateStreamInfoBuffer(InfoCount);
	CombinerStreamInfo = StreamInfoReader(InfoCount, InfoChunkOffset, LocBundle);

	if (OutPath != NULL)
	{
		FILE *fin;
		
		unsigned int ChunkAlignSize;

		sprintf(StreamName, "STREAM%s.BUN", LocationName);
		strcpy(TotalOutPath, OutPath);
		strcat(TotalOutPath, "\\");
		strcat(TotalOutPath, StreamName);
		FILE *fout1 = fopen(TotalOutPath, "wb");
		if (fout1 == NULL)
		{
			printf("Error opening %s\n", TotalOutPath);
			perror("ERROR");
			bCombinerMode = 0;
			return -1;
		}

		for (unsigned int i = 0; i < InfoCount; i++)
		{
			// set the offset and chunk number
			CombinerStreamInfo[i].MasterStreamChunkNumber = 1;
			CombinerStreamInfo[i].MasterStreamChunkOffset = ftell(fout1);
			printf("%s Offset: %X\n", PRINTTYPEINFO, ftell(fout1));

			// generate a filename to search for
			sprintf(StreamName, "STREAM%s_%d.BUN", LocationName, CombinerStreamInfo[i].StreamChunkNumber);
			printf("%s Reading: %s\n", PRINTTYPEINFO, StreamName);
			strcpy(TotalStreamChunkPath, StreamChunkPaths);
			strcat(TotalStreamChunkPath, "\\");
			strcat(TotalStreamChunkPath, StreamName);

			// open the file
			fin = fopen(TotalStreamChunkPath, "rb");
			if (fin == NULL)
			{
				printf("Error opening %s\n", LocBundle);
				perror("ERROR");
				bCombinerMode = 0;
				return -1;
			}
			stat(TotalStreamChunkPath, &st);

			printf("%s Chunk size: %X\n", PRINTTYPEINFO, st.st_size);

			// size3 unknown, so don't touch unless it's identical
			//if (StreamInfo[i].Size3 == StreamInfo[i].Size1)
			CombinerStreamInfo[i].Size3 = st.st_size;

			CombinerStreamInfo[i].Size1 = st.st_size;
			CombinerStreamInfo[i].Size2 = st.st_size;

			// allocate memory, copy to master stream, free memory, close input file
			chunkbuffer = malloc(CombinerStreamInfo[i].Size1);
			fread(chunkbuffer, 1, CombinerStreamInfo[i].Size1, fin);
			fwrite(chunkbuffer, 1, CombinerStreamInfo[i].Size1, fout1);
			free(chunkbuffer);
			fclose(fin);

			if (i != (InfoCount - 1))
			{
				// calculate and write alignment chunk, align by 0x800 bytes
				ChunkAlignSize = (((CombinerStreamInfo[i].Size1 + 8)) - ((CombinerStreamInfo[i].Size1 + 8) % 0x800)) + 0x1000;

				ChunkAlignSize -= CombinerStreamInfo[i].Size1 + 8;

				ZeroChunkWriter(fout1, ChunkAlignSize);
			}
		}
		fclose(fout1);

		InfoChunkOffset += 8;
		fseek(fout2, InfoChunkOffset, SEEK_SET);

		for (unsigned int i = 0; i < InfoCount; i++)
			fwrite(&CombinerStreamInfo[i], 1, sizeof(StreamInfoStruct), fout2);

		fclose(fout2);
		bCombinerMode = 0;
		return 0;
	}
	bCombinerMode = 0;
	return -1;
}
