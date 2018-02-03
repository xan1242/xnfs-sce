#pragma once
#define XNFSSCE_API __declspec(dllexport)

#define STREAMINFOCHUNK 0x00034110 // TODO: integrate with some BCHUNK list

typedef struct StreamInfoStruct
{
	unsigned int ModelGroupName;
	unsigned int Unk1;
	unsigned int StreamChunkNumber;
	unsigned int Unk2;
	unsigned int MasterStreamChunkNumber;
	unsigned int MasterStreamChunkOffset;
	unsigned int Size1;
	unsigned int Size2;
	unsigned int Size3;
	unsigned int Unk3;
	float X;
	float Y;
	float Z;
	unsigned int StreamChunkHash;
	unsigned char RestOfData[0x24]; // 0x24 for MW, 0x1C for Carbon TODO: add game detection or something idk
} *StreamInfo;

#ifdef __cplusplus
extern "C"
{
#endif

// Creates InfoCount number of StreamInfo slots
// example: StreamInfo ExtractorStreamInfo = CreateStreamInfoBuffer(962);
// makes 962 ExtractorStreamInfo slots
XNFSSCE_API StreamInfo CreateStreamInfoBuffer(unsigned int InfoCount);

// Searches for aligned chunks by chunk's type
// return = chunk size, 0 on failure
// Filename - File to search a chunk for
// ChunkMagic - The chunk magic number
// OffsetOut - (out) Writes the offset the chunk was found at
XNFSSCE_API unsigned int SearchAlignedChunkByType(const char* Filename, unsigned int ChunkMagic, long &OffsetOut);

// Automatically calculates the count of stream infos based on the chunk's size
// InfoChunkSize - Stream section info chunk size, get it by using SearchAlignedChunkByType
XNFSSCE_API unsigned int GetInfoCount(unsigned int InfoChunkSize);

// Reads a StreamInfo out of a LocBundle file
// return = The output StreamInfo
// InfoCount - count of StreamInfos to read
// InfoChunkAddress - The address of the info chunk
// LocBundleFilename - The location bundle file path
XNFSSCE_API StreamInfo StreamInfoReader(unsigned int InfoCount, long InfoChunkAddress, const char* LocBundleFilename);

// Extracts all stream parts out of a stream bundle
// return = true on success, false on anything else
// TheStreamInfo - StreamInfo of a LocBundle (generate with StreamInfoReader)
// InfoCount - size of StreamInfo (equals to total number of stream parts)
// StreamFilename - Filename of the stream (e.g. STREAML5RA.BUN)
// OutFolderPath - Folder to where the stream parts will go
XNFSSCE_API bool ExtractAllStreamParts(StreamInfo TheStreamInfo, unsigned int InfoCount, const char* StreamFilename, const char* OutFolderPath);

// Extracts specific stream part
// return = true on success, false on anything else
// StreamPartNumber - the part number
// TheStreamInfo - StreamInfo of a LocBundle (generate with StreamInfoReader)
// InfoCount - size of StreamInfo (equals to total number of stream parts)
// StreamFilename - Filename of the stream (e.g. STREAML5RA.BUN)
// OutFolderPath - Folder to where the stream part will go
XNFSSCE_API bool ExtractStreamPartByNumber(unsigned int StreamPartNumber, StreamInfo TheStreamInfo, unsigned int InfoCount, const char* StreamFilename, const char* OutFolderPath);

// Extracts stream parts
// LocBundle - The original location bundle path (e.g. L5RA.BUN)
// OutPath - Location where the stream parts will go
XNFSSCE_API int ExtractChunks(const char* LocBundle, const char* OutPath);

// Used to write the chunk type internally
XNFSSCE_API int WriteChunkTypeAndSize(FILE *fout, unsigned int ChunkMagic, unsigned int ChunkSize);

// Used to write the padding chunk internally
XNFSSCE_API int ZeroChunkWriter(FILE *fout, unsigned int ChunkSize);

// Combines stream parts back
// LocBundle - The original location bundle path (e.g. L5RA.BUN)
// StreamChunkPaths - Stream parts folder path
// OutPath - Output folder of the combined stream and new LocBundle (outputs a new STREAM(loc).BUN and (loc).BUN)
XNFSSCE_API int CombineChunks(const char* LocBundle, const char* StreamChunkPaths, const char* OutPath);


#ifdef __cplusplus
}
#endif
