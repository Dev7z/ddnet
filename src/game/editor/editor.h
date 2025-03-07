/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_EDITOR_EDITOR_H
#define GAME_EDITOR_EDITOR_H

#include <base/bezier.h>
#include <base/system.h>

#include <game/client/render.h>
#include <game/client/ui.h>
#include <game/mapitems.h>
#include <game/mapitems_ex.h>

#include <game/editor/mapitems/envelope.h>
#include <game/editor/mapitems/layer.h>
#include <game/editor/mapitems/layer_front.h>
#include <game/editor/mapitems/layer_game.h>
#include <game/editor/mapitems/layer_group.h>
#include <game/editor/mapitems/layer_quads.h>
#include <game/editor/mapitems/layer_sounds.h>
#include <game/editor/mapitems/layer_speedup.h>
#include <game/editor/mapitems/layer_switch.h>
#include <game/editor/mapitems/layer_tele.h>
#include <game/editor/mapitems/layer_tiles.h>
#include <game/editor/mapitems/layer_tune.h>

#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/shared/datafile.h>
#include <engine/shared/jobs.h>

#include "auto_map.h"
#include "map_view.h"
#include "smooth_value.h"

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

typedef std::function<void(int *pIndex)> FIndexModifyFunction;

// CEditor SPECIFIC
enum
{
	MODE_LAYERS = 0,
	MODE_IMAGES,
	MODE_SOUNDS,
	NUM_MODES,

	DIALOG_NONE = 0,
	DIALOG_FILE,
};

class CEditorImage;
class CEditorSound;

class CEditorMap
{
	void MakeGameGroup(std::shared_ptr<CLayerGroup> pGroup);
	void MakeGameLayer(const std::shared_ptr<CLayer> &pLayer);

public:
	CEditor *m_pEditor;

	CEditorMap()
	{
		Clean();
	}

	~CEditorMap()
	{
		Clean();
	}

	bool m_Modified; // unsaved changes in manual save
	bool m_ModifiedAuto; // unsaved changes in autosave
	float m_LastModifiedTime;
	float m_LastSaveTime;
	float m_LastAutosaveUpdateTime;
	void OnModify();

	std::vector<std::shared_ptr<CLayerGroup>> m_vpGroups;
	std::vector<std::shared_ptr<CEditorImage>> m_vpImages;
	std::vector<std::shared_ptr<CEnvelope>> m_vpEnvelopes;
	std::vector<std::shared_ptr<CEditorSound>> m_vpSounds;

	class CMapInfo
	{
	public:
		char m_aAuthor[32];
		char m_aVersion[16];
		char m_aCredits[128];
		char m_aLicense[32];

		void Reset()
		{
			m_aAuthor[0] = '\0';
			m_aVersion[0] = '\0';
			m_aCredits[0] = '\0';
			m_aLicense[0] = '\0';
		}

		void Copy(const CMapInfo &Source)
		{
			str_copy(m_aAuthor, Source.m_aAuthor);
			str_copy(m_aVersion, Source.m_aVersion);
			str_copy(m_aCredits, Source.m_aCredits);
			str_copy(m_aLicense, Source.m_aLicense);
		}
	};
	CMapInfo m_MapInfo;
	CMapInfo m_MapInfoTmp;

	struct CSetting
	{
		char m_aCommand[256];

		CSetting(const char *pCommand)
		{
			str_copy(m_aCommand, pCommand);
		}
	};
	std::vector<CSetting> m_vSettings;

	std::shared_ptr<class CLayerGame> m_pGameLayer;
	std::shared_ptr<CLayerGroup> m_pGameGroup;

	std::shared_ptr<CEnvelope> NewEnvelope(CEnvelope::EType Type)
	{
		OnModify();
		std::shared_ptr<CEnvelope> pEnv = std::make_shared<CEnvelope>(Type);
		m_vpEnvelopes.push_back(pEnv);
		return pEnv;
	}

	void DeleteEnvelope(int Index);
	void SwapEnvelopes(int Index0, int Index1);
	template<typename F>
	void VisitEnvelopeReferences(F &&Visitor);

	std::shared_ptr<CLayerGroup> NewGroup()
	{
		OnModify();
		std::shared_ptr<CLayerGroup> pGroup = std::make_shared<CLayerGroup>();
		pGroup->m_pMap = this;
		m_vpGroups.push_back(pGroup);
		return pGroup;
	}

	int SwapGroups(int Index0, int Index1)
	{
		if(Index0 < 0 || Index0 >= (int)m_vpGroups.size())
			return Index0;
		if(Index1 < 0 || Index1 >= (int)m_vpGroups.size())
			return Index0;
		if(Index0 == Index1)
			return Index0;
		OnModify();
		std::swap(m_vpGroups[Index0], m_vpGroups[Index1]);
		return Index1;
	}

	void DeleteGroup(int Index)
	{
		if(Index < 0 || Index >= (int)m_vpGroups.size())
			return;
		OnModify();
		m_vpGroups.erase(m_vpGroups.begin() + Index);
	}

	void ModifyImageIndex(FIndexModifyFunction pfnFunc)
	{
		OnModify();
		for(auto &pGroup : m_vpGroups)
			pGroup->ModifyImageIndex(pfnFunc);
	}

	void ModifyEnvelopeIndex(FIndexModifyFunction pfnFunc)
	{
		OnModify();
		for(auto &pGroup : m_vpGroups)
			pGroup->ModifyEnvelopeIndex(pfnFunc);
	}

	void ModifySoundIndex(FIndexModifyFunction pfnFunc)
	{
		OnModify();
		for(auto &pGroup : m_vpGroups)
			pGroup->ModifySoundIndex(pfnFunc);
	}

	void Clean();
	void CreateDefault(IGraphics::CTextureHandle EntitiesTexture);

	// io
	bool Save(const char *pFilename);
	bool Load(const char *pFilename, int StorageType, const std::function<void(const char *pErrorMessage)> &ErrorHandler);
	void PerformSanityChecks(const std::function<void(const char *pErrorMessage)> &ErrorHandler);

	// DDRace

	std::shared_ptr<class CLayerTele> m_pTeleLayer;
	std::shared_ptr<class CLayerSpeedup> m_pSpeedupLayer;
	std::shared_ptr<class CLayerFront> m_pFrontLayer;
	std::shared_ptr<class CLayerSwitch> m_pSwitchLayer;
	std::shared_ptr<class CLayerTune> m_pTuneLayer;
	void MakeTeleLayer(const std::shared_ptr<CLayer> &pLayer);
	void MakeSpeedupLayer(const std::shared_ptr<CLayer> &pLayer);
	void MakeFrontLayer(const std::shared_ptr<CLayer> &pLayer);
	void MakeSwitchLayer(const std::shared_ptr<CLayer> &pLayer);
	void MakeTuneLayer(const std::shared_ptr<CLayer> &pLayer);
};

struct CProperty
{
	const char *m_pName;
	int m_Value;
	int m_Type;
	int m_Min;
	int m_Max;
};

enum
{
	PROPTYPE_NULL = 0,
	PROPTYPE_BOOL,
	PROPTYPE_INT_STEP,
	PROPTYPE_INT_SCROLL,
	PROPTYPE_ANGLE_SCROLL,
	PROPTYPE_COLOR,
	PROPTYPE_IMAGE,
	PROPTYPE_ENVELOPE,
	PROPTYPE_SHIFT,
	PROPTYPE_SOUND,
	PROPTYPE_AUTOMAPPER,
};

class CDataFileWriterFinishJob : public IJob
{
	char m_aRealFileName[IO_MAX_PATH_LENGTH];
	char m_aTempFileName[IO_MAX_PATH_LENGTH];
	CDataFileWriter m_Writer;

	void Run() override
	{
		m_Writer.Finish();
	}

public:
	CDataFileWriterFinishJob(const char *pRealFileName, const char *pTempFileName, CDataFileWriter &&Writer) :
		m_Writer(std::move(Writer))
	{
		str_copy(m_aRealFileName, pRealFileName);
		str_copy(m_aTempFileName, pTempFileName);
	}

	const char *GetRealFileName() const { return m_aRealFileName; }
	const char *GetTempFileName() const { return m_aTempFileName; }
};

class CEditor : public IEditor
{
	class IInput *m_pInput = nullptr;
	class IClient *m_pClient = nullptr;
	class CConfig *m_pConfig = nullptr;
	class IConsole *m_pConsole = nullptr;
	class IEngine *m_pEngine = nullptr;
	class IGraphics *m_pGraphics = nullptr;
	class ITextRender *m_pTextRender = nullptr;
	class ISound *m_pSound = nullptr;
	class IStorage *m_pStorage = nullptr;
	CRenderTools m_RenderTools;
	CUI m_UI;

	std::vector<std::reference_wrapper<CEditorComponent>> m_vComponents;
	CMapView m_MapView;

	bool m_EditorWasUsedBefore = false;

	IGraphics::CTextureHandle m_EntitiesTexture;

	IGraphics::CTextureHandle m_FrontTexture;
	IGraphics::CTextureHandle m_TeleTexture;
	IGraphics::CTextureHandle m_SpeedupTexture;
	IGraphics::CTextureHandle m_SwitchTexture;
	IGraphics::CTextureHandle m_TuneTexture;

	int GetTextureUsageFlag();

	enum EPreviewState
	{
		PREVIEW_UNLOADED,
		PREVIEW_LOADED,
		PREVIEW_ERROR,
	};

	std::shared_ptr<CLayerGroup> m_apSavedBrushes[10];

public:
	class IInput *Input() { return m_pInput; }
	class IClient *Client() { return m_pClient; }
	class CConfig *Config() { return m_pConfig; }
	class IConsole *Console() { return m_pConsole; }
	class IEngine *Engine() { return m_pEngine; }
	class IGraphics *Graphics() { return m_pGraphics; }
	class ISound *Sound() { return m_pSound; }
	class ITextRender *TextRender() { return m_pTextRender; }
	class IStorage *Storage() { return m_pStorage; }
	CUI *UI() { return &m_UI; }
	CRenderTools *RenderTools() { return &m_RenderTools; }

	CMapView *MapView() { return &m_MapView; }
	const CMapView *MapView() const { return &m_MapView; }

	CEditor() :
		m_ZoomEnvelopeX(1.0f, 0.1f, 600.0f),
		m_ZoomEnvelopeY(640.0f, 0.1f, 32000.0f)
	{
		m_EntitiesTexture.Invalidate();
		m_FrontTexture.Invalidate();
		m_TeleTexture.Invalidate();
		m_SpeedupTexture.Invalidate();
		m_SwitchTexture.Invalidate();
		m_TuneTexture.Invalidate();

		m_Mode = MODE_LAYERS;
		m_Dialog = 0;

		m_BrushColorEnabled = true;

		m_aFileName[0] = '\0';
		m_aFileNamePending[0] = '\0';
		m_aFileSaveName[0] = '\0';
		m_ValidSaveFilename = false;

		m_PopupEventActivated = false;
		m_PopupEventWasActivated = false;

		m_FileDialogStorageType = 0;
		m_FileDialogLastPopulatedStorageType = 0;
		m_pFileDialogTitle = nullptr;
		m_pFileDialogButtonText = nullptr;
		m_pFileDialogUser = nullptr;
		m_aFileDialogCurrentFolder[0] = '\0';
		m_aFileDialogCurrentLink[0] = '\0';
		m_aFilesSelectedName[0] = '\0';
		m_pFileDialogPath = m_aFileDialogCurrentFolder;
		m_FileDialogOpening = false;
		m_FilesSelectedIndex = -1;

		m_FilePreviewImage.Invalidate();
		m_FilePreviewSound = -1;
		m_FilePreviewState = PREVIEW_UNLOADED;

		m_SelectEntitiesImage = "DDNet";

		m_ResetZoomEnvelope = true;
		m_OffsetEnvelopeX = 0.1f;
		m_OffsetEnvelopeY = 0.5f;

		m_ShowMousePointer = true;
		m_MouseDeltaX = 0;
		m_MouseDeltaY = 0;
		m_MouseDeltaWx = 0;
		m_MouseDeltaWy = 0;

		m_GuiActive = true;
		m_PreviewZoom = false;

		m_ShowTileInfo = SHOW_TILE_OFF;
		m_ShowDetail = true;
		m_Animate = false;
		m_AnimateStart = 0;
		m_AnimateTime = 0;
		m_AnimateSpeed = 1;

		m_ShowEnvelopePreview = SHOWENV_NONE;
		m_SelectedQuadEnvelope = -1;

		m_vSelectedEnvelopePoints = {};
		m_UpdateEnvPointInfo = false;
		m_SelectedTangentInPoint = std::pair(-1, -1);
		m_SelectedTangentOutPoint = std::pair(-1, -1);
		m_CurrentQuadIndex = -1;

		m_QuadKnifeActive = false;
		m_QuadKnifeCount = 0;

		m_CheckerTexture.Invalidate();
		m_BackgroundTexture.Invalidate();
		m_CursorTexture.Invalidate();

		ms_pUiGotContext = nullptr;

		// DDRace

		m_TeleNumber = 1;
		m_SwitchNum = 1;
		m_TuningNum = 1;
		m_SwitchDelay = 0;
		m_SpeedupForce = 50;
		m_SpeedupMaxSpeed = 0;
		m_SpeedupAngle = 0;
		m_LargeLayerWasWarned = false;
		m_PreventUnusedTilesWasWarned = false;
		m_AllowPlaceUnusedTiles = 0;
		m_BrushDrawDestructive = true;
	}

	void Init() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnActivate() override;
	void OnWindowResize() override;
	bool HasUnsavedData() const override { return m_Map.m_Modified; }
	void UpdateMentions() override { m_Mentions++; }
	void ResetMentions() override { m_Mentions = 0; }
	void OnIngameMoved() override { m_IngameMoved = true; }
	void ResetIngameMoved() override { m_IngameMoved = false; }

	void HandleCursorMovement();
	void DispatchInputEvents();
	void HandleAutosave();
	bool PerformAutosave();
	void HandleWriterFinishJobs();

	void RefreshFilteredFileList();
	void FilelistPopulate(int StorageType, bool KeepSelection = false);
	void InvokeFileDialog(int StorageType, int FileType, const char *pTitle, const char *pButtonText,
		const char *pBasepath, bool FilenameAsDefault,
		bool (*pfnFunc)(const char *pFilename, int StorageType, void *pUser), void *pUser);
	struct SStringKeyComparator
	{
		bool operator()(char const *pLhs, char const *pRhs) const
		{
			return str_comp(pLhs, pRhs) < 0;
		}
	};
	std::map<const char *, CUI::SMessagePopupContext *, SStringKeyComparator> m_PopupMessageContexts;
	void ShowFileDialogError(const char *pFormat, ...)
		GNUC_ATTRIBUTE((format(printf, 2, 3)));

	void Reset(bool CreateDefault = true);
	bool Save(const char *pFilename) override;
	bool Load(const char *pFilename, int StorageType) override;
	bool HandleMapDrop(const char *pFilename, int StorageType) override;
	bool Append(const char *pFilename, int StorageType);
	void LoadCurrentMap();
	void Render();

	void RenderPressedKeys(CUIRect View);
	void RenderSavingIndicator(CUIRect View);
	void FreeDynamicPopupMenus();
	void RenderMousePointer();

	std::vector<CQuad *> GetSelectedQuads();
	std::shared_ptr<CLayer> GetSelectedLayerType(int Index, int Type) const;
	std::shared_ptr<CLayer> GetSelectedLayer(int Index) const;
	std::shared_ptr<CLayerGroup> GetSelectedGroup() const;
	CSoundSource *GetSelectedSource();
	void SelectLayer(int LayerIndex, int GroupIndex = -1);
	void AddSelectedLayer(int LayerIndex);
	void SelectQuad(int Index);
	void ToggleSelectQuad(int Index);
	void DeselectQuads();
	void DeselectQuadPoints();
	void SelectQuadPoint(int QuadIndex, int Index);
	void ToggleSelectQuadPoint(int QuadIndex, int Index);
	void DeleteSelectedQuads();
	bool IsQuadSelected(int Index) const;
	bool IsQuadCornerSelected(int Index) const;
	bool IsQuadPointSelected(int QuadIndex, int Index) const;
	int FindSelectedQuadIndex(int Index) const;

	int FindEnvPointIndex(int Index, int Channel) const;
	void SelectEnvPoint(int Index);
	void SelectEnvPoint(int Index, int Channel);
	void ToggleEnvPoint(int Index, int Channel);
	bool IsEnvPointSelected(int Index, int Channel) const;
	bool IsEnvPointSelected(int Index) const;
	void DeselectEnvPoints();
	void SelectTangentOutPoint(int Index, int Channel);
	bool IsTangentOutPointSelected(int Index, int Channel) const;
	void SelectTangentInPoint(int Index, int Channel);
	bool IsTangentInPointSelected(int Index, int Channel) const;
	bool IsTangentInSelected() const;
	bool IsTangentOutSelected() const;
	bool IsTangentSelected() const;

	int DoProperties(CUIRect *pToolbox, CProperty *pProps, int *pIDs, int *pNewVal, ColorRGBA Color = ColorRGBA(1, 1, 1, 0.5f));

	int m_Mode;
	int m_Dialog;
	char m_aTooltip[256] = "";

	bool m_BrushColorEnabled;

	char m_aFileName[IO_MAX_PATH_LENGTH];
	char m_aFileNamePending[IO_MAX_PATH_LENGTH];
	char m_aFileSaveName[IO_MAX_PATH_LENGTH];
	bool m_ValidSaveFilename;

	enum
	{
		POPEVENT_EXIT = 0,
		POPEVENT_LOAD,
		POPEVENT_LOADCURRENT,
		POPEVENT_LOADDROP,
		POPEVENT_NEW,
		POPEVENT_SAVE,
		POPEVENT_SAVE_COPY,
		POPEVENT_SAVE_IMG,
		POPEVENT_SAVE_SOUND,
		POPEVENT_LARGELAYER,
		POPEVENT_PREVENTUNUSEDTILES,
		POPEVENT_IMAGEDIV16,
		POPEVENT_IMAGE_MAX,
		POPEVENT_PLACE_BORDER_TILES,
		POPEVENT_PIXELART_BIG_IMAGE,
		POPEVENT_PIXELART_MANY_COLORS,
		POPEVENT_PIXELART_TOO_MANY_COLORS
	};

	int m_PopupEventType;
	int m_PopupEventActivated;
	int m_PopupEventWasActivated;
	bool m_LargeLayerWasWarned;
	bool m_PreventUnusedTilesWasWarned;
	int m_AllowPlaceUnusedTiles;
	bool m_BrushDrawDestructive;

	int m_Mentions = 0;
	bool m_IngameMoved = false;

	enum
	{
		FILETYPE_MAP,
		FILETYPE_IMG,
		FILETYPE_SOUND,
		NUM_FILETYPES
	};

	int m_FileDialogStorageType;
	int m_FileDialogLastPopulatedStorageType;
	const char *m_pFileDialogTitle;
	const char *m_pFileDialogButtonText;
	bool (*m_pfnFileDialogFunc)(const char *pFileName, int StorageType, void *pUser);
	void *m_pFileDialogUser;
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_FileDialogFileNameInput;
	char m_aFileDialogCurrentFolder[IO_MAX_PATH_LENGTH];
	char m_aFileDialogCurrentLink[IO_MAX_PATH_LENGTH];
	char m_aFilesSelectedName[IO_MAX_PATH_LENGTH];
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_FileDialogFilterInput;
	char *m_pFileDialogPath;
	int m_FileDialogFileType;
	bool m_FileDialogMultipleStorages = false;
	bool m_FileDialogShowingRoot = false;
	int m_FilesSelectedIndex;
	CLineInputBuffered<IO_MAX_PATH_LENGTH> m_FileDialogNewFolderNameInput;

	IGraphics::CTextureHandle m_FilePreviewImage;
	int m_FilePreviewSound;
	EPreviewState m_FilePreviewState;
	CImageInfo m_FilePreviewImageInfo;
	bool m_FileDialogOpening;

	struct CFilelistItem
	{
		char m_aFilename[IO_MAX_PATH_LENGTH];
		char m_aName[IO_MAX_PATH_LENGTH];
		bool m_IsDir;
		bool m_IsLink;
		int m_StorageType;
		time_t m_TimeModified;
	};
	std::vector<CFilelistItem> m_vCompleteFileList;
	std::vector<const CFilelistItem *> m_vpFilteredFileList;

	static bool CompareFilenameAscending(const CFilelistItem *pLhs, const CFilelistItem *pRhs)
	{
		if(str_comp(pLhs->m_aFilename, "..") == 0)
			return true;
		if(str_comp(pRhs->m_aFilename, "..") == 0)
			return false;
		if(pLhs->m_IsLink != pRhs->m_IsLink)
			return pLhs->m_IsLink;
		if(pLhs->m_IsDir != pRhs->m_IsDir)
			return pLhs->m_IsDir;
		return str_comp_filenames(pLhs->m_aName, pRhs->m_aName) < 0;
	}

	static bool CompareFilenameDescending(const CFilelistItem *pLhs, const CFilelistItem *pRhs)
	{
		if(str_comp(pLhs->m_aFilename, "..") == 0)
			return true;
		if(str_comp(pRhs->m_aFilename, "..") == 0)
			return false;
		if(pLhs->m_IsLink != pRhs->m_IsLink)
			return pLhs->m_IsLink;
		if(pLhs->m_IsDir != pRhs->m_IsDir)
			return pLhs->m_IsDir;
		return str_comp_filenames(pLhs->m_aName, pRhs->m_aName) > 0;
	}

	static bool CompareTimeModifiedAscending(const CFilelistItem *pLhs, const CFilelistItem *pRhs)
	{
		if(str_comp(pLhs->m_aFilename, "..") == 0)
			return true;
		if(str_comp(pRhs->m_aFilename, "..") == 0)
			return false;
		if(pLhs->m_IsLink || pRhs->m_IsLink)
			return pLhs->m_IsLink;
		if(pLhs->m_IsDir != pRhs->m_IsDir)
			return pLhs->m_IsDir;
		return pLhs->m_TimeModified < pRhs->m_TimeModified;
	}

	static bool CompareTimeModifiedDescending(const CFilelistItem *pLhs, const CFilelistItem *pRhs)
	{
		if(str_comp(pLhs->m_aFilename, "..") == 0)
			return true;
		if(str_comp(pRhs->m_aFilename, "..") == 0)
			return false;
		if(pLhs->m_IsLink || pRhs->m_IsLink)
			return pLhs->m_IsLink;
		if(pLhs->m_IsDir != pRhs->m_IsDir)
			return pLhs->m_IsDir;
		return pLhs->m_TimeModified > pRhs->m_TimeModified;
	}

	void SortFilteredFileList();
	int m_SortByFilename = 1;
	int m_SortByTimeModified = 0;

	std::vector<std::string> m_vSelectEntitiesFiles;
	std::string m_SelectEntitiesImage;

	// Zooming
	CSmoothValue m_ZoomEnvelopeX;
	CSmoothValue m_ZoomEnvelopeY;

	bool m_ResetZoomEnvelope;

	float m_OffsetEnvelopeX;
	float m_OffsetEnvelopeY;

	bool m_ShowMousePointer;
	bool m_GuiActive;

	char m_aMenuBackgroundTooltip[256];
	bool m_PreviewZoom;
	float m_MouseWScale = 1.0f; // Mouse (i.e. UI) scale relative to the World (selected Group)
	float m_MouseX = 0.0f;
	float m_MouseY = 0.0f;
	float m_MouseWorldX = 0.0f;
	float m_MouseWorldY = 0.0f;
	float m_MouseWorldNoParaX = 0.0f;
	float m_MouseWorldNoParaY = 0.0f;
	float m_MouseDeltaX;
	float m_MouseDeltaY;
	float m_MouseDeltaWx;
	float m_MouseDeltaWy;

	enum EShowTile
	{
		SHOW_TILE_OFF,
		SHOW_TILE_DECIMAL,
		SHOW_TILE_HEXADECIMAL
	};
	EShowTile m_ShowTileInfo;
	bool m_ShowDetail;
	bool m_Animate;
	int64_t m_AnimateStart;
	float m_AnimateTime;
	float m_AnimateSpeed;

	enum EExtraEditor
	{
		EXTRAEDITOR_NONE = -1,
		EXTRAEDITOR_ENVELOPES,
		EXTRAEDITOR_SERVER_SETTINGS,
		NUM_EXTRAEDITORS,
	};
	EExtraEditor m_ActiveExtraEditor = EXTRAEDITOR_NONE;
	float m_aExtraEditorSplits[NUM_EXTRAEDITORS] = {250.0f, 250.0f};

	enum EShowEnvelope
	{
		SHOWENV_NONE = 0,
		SHOWENV_SELECTED,
		SHOWENV_ALL
	};
	EShowEnvelope m_ShowEnvelopePreview;
	bool m_ShowPicker;

	std::vector<int> m_vSelectedLayers;
	std::vector<int> m_vSelectedQuads;
	int m_SelectedQuadPoint;
	int m_SelectedQuadIndex;
	int m_SelectedGroup;
	int m_SelectedQuadPoints;
	int m_SelectedEnvelope;
	std::vector<std::pair<int, int>> m_vSelectedEnvelopePoints;
	int m_SelectedQuadEnvelope;
	int m_CurrentQuadIndex;
	int m_SelectedImage;
	int m_SelectedSound;
	int m_SelectedSource;
	std::pair<int, int> m_SelectedTangentInPoint;
	std::pair<int, int> m_SelectedTangentOutPoint;
	bool m_UpdateEnvPointInfo;

	bool m_QuadKnifeActive;
	int m_QuadKnifeCount;
	vec2 m_aQuadKnifePoints[4];

	IGraphics::CTextureHandle m_CheckerTexture;
	IGraphics::CTextureHandle m_BackgroundTexture;
	IGraphics::CTextureHandle m_CursorTexture;

	IGraphics::CTextureHandle GetEntitiesTexture();

	std::shared_ptr<CLayerGroup> m_pBrush;
	std::shared_ptr<CLayerTiles> m_pTilesetPicker;
	std::shared_ptr<CLayerQuads> m_pQuadsetPicker;

	static const void *ms_pUiGotContext;

	CEditorMap m_Map;
	std::deque<std::shared_ptr<CDataFileWriterFinishJob>> m_WriterFinishJobs;

	int m_ShiftBy;

	static void EnvelopeEval(int TimeOffsetMillis, int Env, ColorRGBA &Channels, void *pUser);

	CLineInputBuffered<256> m_SettingsCommandInput;

	CImageInfo m_TileartImageInfo;
	char m_aTileartFilename[IO_MAX_PATH_LENGTH];
	void AddTileart();
	void TileartCheckColors();

	void PlaceBorderTiles();

	void UpdateTooltip(const void *pID, const CUIRect *pRect, const char *pToolTip);
	int DoButton_Editor_Common(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Editor(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_Env(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pToolTip, ColorRGBA Color, int Corners);

	int DoButton_Ex(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip, int Corners, float FontSize = 10.0f);
	int DoButton_FontIcon(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip, int Corners, float FontSize = 10.0f);
	int DoButton_ButtonDec(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_ButtonInc(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_File(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);

	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags, const char *pToolTip);
	int DoButton_MenuItem(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Flags = 0, const char *pToolTip = nullptr);

	int DoButton_DraggableEx(const void *pID, const char *pText, int Checked, const CUIRect *pRect, bool *pClicked, bool *pAbrupted, int Flags, const char *pToolTip = nullptr, int Corners = IGraphics::CORNER_ALL, float FontSize = 10.0f);

	bool DoEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, int Corners = IGraphics::CORNER_ALL, const char *pToolTip = nullptr);
	bool DoClearableEditBox(CLineInput *pLineInput, const CUIRect *pRect, float FontSize, int Corners = IGraphics::CORNER_ALL, const char *pToolTip = nullptr);

	void RenderBackground(CUIRect View, IGraphics::CTextureHandle Texture, float Size, float Brightness);

	int UiDoValueSelector(void *pID, CUIRect *pRect, const char *pLabel, int Current, int Min, int Max, int Step, float Scale, const char *pToolTip, bool IsDegree = false, bool IsHex = false, int corners = IGraphics::CORNER_ALL, ColorRGBA *pColor = nullptr, bool ShowValue = true);

	static CUI::EPopupMenuFunctionResult PopupMenuFile(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupMenuTools(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupMenuSettings(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupGroup(void *pContext, CUIRect View, bool Active);
	struct SLayerPopupContext : public SPopupMenuId
	{
		CEditor *m_pEditor;
		std::vector<std::shared_ptr<CLayerTiles>> m_vpLayers;
		CLayerTiles::SCommonPropState m_CommonPropState;
	};
	static CUI::EPopupMenuFunctionResult PopupLayer(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupQuad(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSource(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupPoint(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupEnvPoint(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupEnvPointMulti(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupEnvPointCurveType(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupImage(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSound(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupNewFolder(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupMapInfo(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupEvent(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSelectImage(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSelectSound(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSelectGametileOp(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSelectConfigAutoMap(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupTele(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSpeedup(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupSwitch(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupTune(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupGoto(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupEntities(void *pContext, CUIRect View, bool Active);
	static CUI::EPopupMenuFunctionResult PopupProofMode(void *pContext, CUIRect View, bool Active);

	static bool CallbackOpenMap(const char *pFileName, int StorageType, void *pUser);
	static bool CallbackAppendMap(const char *pFileName, int StorageType, void *pUser);
	static bool CallbackSaveMap(const char *pFileName, int StorageType, void *pUser);
	static bool CallbackSaveCopyMap(const char *pFileName, int StorageType, void *pUser);
	static bool CallbackAddTileart(const char *pFilepath, int StorageType, void *pUser);
	static bool CallbackSaveImage(const char *pFileName, int StorageType, void *pUser);
	static bool CallbackSaveSound(const char *pFileName, int StorageType, void *pUser);

	void PopupSelectImageInvoke(int Current, float x, float y);
	int PopupSelectImageResult();

	void PopupSelectGametileOpInvoke(float x, float y);
	int PopupSelectGameTileOpResult();

	void PopupSelectConfigAutoMapInvoke(int Current, float x, float y);
	int PopupSelectConfigAutoMapResult();

	void PopupSelectSoundInvoke(int Current, float x, float y);
	int PopupSelectSoundResult();

	void DoQuadEnvelopes(const std::vector<CQuad> &vQuads, IGraphics::CTextureHandle Texture = IGraphics::CTextureHandle());
	void DoQuadEnvPoint(const CQuad *pQuad, int QIndex, int pIndex);
	void DoQuadPoint(CQuad *pQuad, int QuadIndex, int v);
	void SetHotQuadPoint(const std::shared_ptr<CLayerQuads> &pLayer);

	float TriangleArea(vec2 A, vec2 B, vec2 C);
	bool IsInTriangle(vec2 Point, vec2 A, vec2 B, vec2 C);
	void DoQuadKnife(int QuadIndex);

	void DoSoundSource(CSoundSource *pSource, int Index);

	void DoMapEditor(CUIRect View);
	void DoToolbarLayers(CUIRect Toolbar);
	void DoToolbarSounds(CUIRect Toolbar);
	void DoQuad(CQuad *pQuad, int Index);
	ColorRGBA GetButtonColor(const void *pID, int Checked);

	bool ReplaceImage(const char *pFilename, int StorageType, bool CheckDuplicate);
	static bool ReplaceImageCallback(const char *pFilename, int StorageType, void *pUser);
	bool ReplaceSound(const char *pFileName, int StorageType, bool CheckDuplicate);
	static bool ReplaceSoundCallback(const char *pFileName, int StorageType, void *pUser);
	static bool AddImage(const char *pFilename, int StorageType, void *pUser);
	static bool AddSound(const char *pFileName, int StorageType, void *pUser);

	bool IsEnvelopeUsed(int EnvelopeIndex) const;
	void RemoveUnusedEnvelopes();

	static bool IsVanillaImage(const char *pImage);

	void RenderLayers(CUIRect LayersBox);
	void RenderImagesList(CUIRect Toolbox);
	void RenderSelectedImage(CUIRect View);
	void RenderSounds(CUIRect Toolbox);
	void RenderModebar(CUIRect View);
	void RenderStatusbar(CUIRect View, CUIRect *pTooltipRect);
	void RenderTooltip(CUIRect TooltipRect);

	void RenderEnvelopeEditor(CUIRect View);
	void RenderServerSettingsEditor(CUIRect View, bool ShowServerSettingsEditorLast);

	void RenderExtraEditorDragBar(CUIRect View, CUIRect DragBar);

	void SetHotEnvelopePoint(const CUIRect &View, const std::shared_ptr<CEnvelope> &pEnvelope, int ActiveChannels);

	void RenderMenubar(CUIRect Menubar);
	void RenderFileDialog();

	void SelectGameLayer();
	void SortImages();
	bool SelectLayerByTile();

	void DoAudioPreview(CUIRect View, const void *pPlayPauseButtonID, const void *pStopButtonID, const void *pSeekBarID, const int SampleID);

	// Tile Numbers For Explanations - TODO: Add/Improve tiles and explanations
	enum
	{
		TILE_PUB_AIR,
		TILE_PUB_HOOKABLE,
		TILE_PUB_DEATH,
		TILE_PUB_UNHOOKABLE,

		TILE_PUB_CREDITS1 = 140,
		TILE_PUB_CREDITS2,
		TILE_PUB_CREDITS3,
		TILE_PUB_CREDITS4,
		TILE_PUB_CREDITS5 = 156,
		TILE_PUB_CREDITS6,
		TILE_PUB_CREDITS7,
		TILE_PUB_CREDITS8,

		TILE_PUB_ENTITIES_OFF1 = 190,
		TILE_PUB_ENTITIES_OFF2,
	};

	enum
	{
		TILE_FNG_SPIKE_GOLD = 7,
		TILE_FNG_SPIKE_NORMAL,
		TILE_FNG_SPIKE_RED,
		TILE_FNG_SPIKE_BLUE,
		TILE_FNG_SCORE_RED,
		TILE_FNG_SCORE_BLUE,

		TILE_FNG_SPIKE_GREEN = 14,
		TILE_FNG_SPIKE_PURPLE,

		TILE_FNG_SPAWN = 192,
		TILE_FNG_SPAWN_RED,
		TILE_FNG_SPAWN_BLUE,
		TILE_FNG_FLAG_RED,
		TILE_FNG_FLAG_BLUE,
		TILE_FNG_SHIELD,
		TILE_FNG_HEART,
		TILE_FNG_SHOTGUN,
		TILE_FNG_GRENADE,
		TILE_FNG_NINJA,
		TILE_FNG_LASER,

		TILE_FNG_SPIKE_OLD1 = 208,
		TILE_FNG_SPIKE_OLD2,
		TILE_FNG_SPIKE_OLD3
	};

	enum
	{
		TILE_VANILLA_SPAWN = 192,
		TILE_VANILLA_SPAWN_RED,
		TILE_VANILLA_SPAWN_BLUE,
		TILE_VANILLA_FLAG_RED,
		TILE_VANILLA_FLAG_BLUE,
		TILE_VANILLA_SHIELD,
		TILE_VANILLA_HEART,
		TILE_VANILLA_SHOTGUN,
		TILE_VANILLA_GRENADE,
		TILE_VANILLA_NINJA,
		TILE_VANILLA_LASER,
	};

	// Explanations
	enum class EExplanation
	{
		DDNET,
		FNG,
		RACE,
		VANILLA,
		BLOCKWORLDS
	};
	static const char *ExplainDDNet(int Tile, int Layer);
	static const char *ExplainFNG(int Tile, int Layer);
	static const char *ExplainVanilla(int Tile, int Layer);
	static const char *Explain(EExplanation Explanation, int Tile, int Layer);

	// Zooming
	void ZoomAdaptOffsetX(float ZoomFactor, const CUIRect &View);
	void UpdateZoomEnvelopeX(const CUIRect &View);

	void ZoomAdaptOffsetY(float ZoomFactor, const CUIRect &View);
	void UpdateZoomEnvelopeY(const CUIRect &View);

	void ResetZoomEnvelope(const std::shared_ptr<CEnvelope> &pEnvelope, int ActiveChannels);
	void RemoveTimeOffsetEnvelope(const std::shared_ptr<CEnvelope> &pEnvelope);
	float ScreenToEnvelopeX(const CUIRect &View, float x) const;
	float EnvelopeToScreenX(const CUIRect &View, float x) const;
	float ScreenToEnvelopeY(const CUIRect &View, float y) const;
	float EnvelopeToScreenY(const CUIRect &View, float y) const;
	float ScreenToEnvelopeDX(const CUIRect &View, float dx);
	float ScreenToEnvelopeDY(const CUIRect &View, float dy);

	// DDRace

	IGraphics::CTextureHandle GetFrontTexture();
	IGraphics::CTextureHandle GetTeleTexture();
	IGraphics::CTextureHandle GetSpeedupTexture();
	IGraphics::CTextureHandle GetSwitchTexture();
	IGraphics::CTextureHandle GetTuneTexture();

	unsigned char m_TeleNumber;

	unsigned char m_TuningNum;

	unsigned char m_SpeedupForce;
	unsigned char m_SpeedupMaxSpeed;
	short m_SpeedupAngle;

	unsigned char m_SwitchNum;
	unsigned char m_SwitchDelay;
};

// make sure to inline this function
inline class IGraphics *CLayer::Graphics() { return m_pEditor->Graphics(); }
inline class ITextRender *CLayer::TextRender() { return m_pEditor->TextRender(); }

#endif
