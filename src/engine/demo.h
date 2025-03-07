/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_DEMO_H
#define ENGINE_DEMO_H

#include "kernel.h"
#include <base/hash.h>
#include <engine/map.h>
#include <engine/shared/uuid_manager.h>

enum
{
	MAX_TIMELINE_MARKERS = 64
};

static const unsigned char gs_aHeaderMarker[7] = {'T', 'W', 'D', 'E', 'M', 'O', 0};

constexpr int g_DemoSpeeds = 22;
extern const double g_aSpeeds[g_DemoSpeeds];

typedef bool (*DEMOFUNC_FILTER)(const void *pData, int DataSize, void *pUser);

// TODO: Properly extend demo format using uuids
// "6be6da4a-cebd-380c-9b5b-1289c842d780"
// "demoitem-sha256@ddnet.tw"
extern const CUuid SHA256_EXTENSION;

struct CDemoHeader
{
	unsigned char m_aMarker[7];
	unsigned char m_Version;
	char m_aNetversion[64];
	char m_aMapName[64];
	unsigned char m_aMapSize[sizeof(int32_t)];
	unsigned char m_aMapCrc[sizeof(int32_t)];
	char m_aType[8];
	unsigned char m_aLength[sizeof(int32_t)];
	char m_aTimestamp[20];

	bool Valid() const
	{
		// Check marker and ensure that strings are zero-terminated and valid UTF-8.
		return mem_comp(m_aMarker, gs_aHeaderMarker, sizeof(gs_aHeaderMarker)) == 0 &&
		       mem_has_null(m_aNetversion, sizeof(m_aNetversion)) && str_utf8_check(m_aNetversion) &&
		       mem_has_null(m_aMapName, sizeof(m_aMapName)) && str_utf8_check(m_aMapName) &&
		       mem_has_null(m_aType, sizeof(m_aType)) && str_utf8_check(m_aType) &&
		       mem_has_null(m_aTimestamp, sizeof(m_aTimestamp)) && str_utf8_check(m_aTimestamp);
	}
};

struct CTimelineMarkers
{
	unsigned char m_aNumTimelineMarkers[sizeof(int32_t)];
	unsigned char m_aTimelineMarkers[MAX_TIMELINE_MARKERS][sizeof(int32_t)];
};

struct CMapInfo
{
	char m_aName[MAX_MAP_LENGTH];
	SHA256_DIGEST m_Sha256;
	int m_Crc;
	int m_Size;
};

class IDemoPlayer : public IInterface
{
	MACRO_INTERFACE("demoplayer", 0)
public:
	class CInfo
	{
	public:
		bool m_Paused;
		float m_Speed;

		int m_FirstTick;
		int m_CurrentTick;
		int m_LastTick;

		int m_NumTimelineMarkers;
		int m_aTimelineMarkers[MAX_TIMELINE_MARKERS];
	};

	enum
	{
		DEMOTYPE_INVALID = 0,
		DEMOTYPE_CLIENT,
		DEMOTYPE_SERVER,
	};

	enum ETickOffset
	{
		TICK_CURRENT, // update the current tick again
		TICK_PREVIOUS, // go to the previous tick
		TICK_NEXT, // go to the next tick
	};

	virtual ~IDemoPlayer() {}
	virtual void SetSpeed(float Speed) = 0;
	virtual void SetSpeedIndex(int SpeedIndex) = 0;
	virtual void AdjustSpeedIndex(int Offset) = 0;
	virtual int SeekPercent(float Percent) = 0;
	virtual int SeekTime(float Seconds) = 0;
	virtual int SeekTick(ETickOffset TickOffset) = 0;
	virtual int SetPos(int WantedTick) = 0;
	virtual void Pause() = 0;
	virtual void Unpause() = 0;
	virtual bool IsPlaying() const = 0;
	virtual const CInfo *BaseInfo() const = 0;
	virtual void GetDemoName(char *pBuffer, int BufferSize) const = 0;
	virtual bool GetDemoInfo(class IStorage *pStorage, const char *pFilename, int StorageType, CDemoHeader *pDemoHeader, CTimelineMarkers *pTimelineMarkers, CMapInfo *pMapInfo) const = 0;
	virtual int GetDemoType() const = 0;
};

class IDemoRecorder : public IInterface
{
	MACRO_INTERFACE("demorecorder", 0)
public:
	virtual ~IDemoRecorder() {}
	virtual bool IsRecording() const = 0;
	virtual int Stop() = 0;
	virtual int Length() const = 0;
	virtual char *GetCurrentFilename() = 0;
};

class IDemoEditor : public IInterface
{
	MACRO_INTERFACE("demoeditor", 0)
public:
	virtual void Slice(const char *pDemo, const char *pDst, int StartTick, int EndTick, DEMOFUNC_FILTER pfnFilter, void *pUser) = 0;
};

#endif
