#pragma once

enum LanguageID
{
	MTitle,
	MDescription,
	MOK,
	MCancel,
	MRetry,
	MSkip,
	MError,

	MRecoveryDir,

	MPluginOptionsTitle,
	MEnableDesktopNotifications,
	MEnterExecRemotely,
	MSmartSymlinksCopy,
	MUseOfChmod,
	MUseOfChmod_Auto,
	MUseOfChmod_Always,
	MUseOfChmod_Never,
	MRememberDirectory,
	MConnPoolExpiration,

	MRememberChoice,
	MOperationFailed,
	MCouldNotConnect,
	MWrongPath,

	MLoginAuthTitle,
	MLoginAuthRetryTitle,
	MLoginAuthTo,
	MLoginAuthRetryTo,

	MProtocol,
	MHost,
	MPort,
	MLoginMode,
	MPasswordModeNoPassword,
	MPasswordModeAskPassword,
	MPasswordModeSavedPassword,
	MUserName,
	MPassword,
	MDirectory,
	MKeepAlive,
	MCodepage,
	MTimeAdjust,
	MDisplayName,
	MExtraOptions,
	MProtocolOptions,
	MProxyOptions,
	MSave,
	MConnect,
	MSaveConnect,

	MCreateSiteConnection,
	MEditHost,

	MXferCopyDownloadTitle,
	MXferCopyUploadTitle,
	MXferCopyCrossloadTitle,
	MXferMoveDownloadTitle,
	MXferMoveUploadTitle,
	MXferMoveCrossloadTitle,
	MXferRenameTitle,

	MXferCopyDownloadText,
	MXferCopyUploadText,
	MXferCopyCrossloadText,
	MXferMoveDownloadText,
	MXferMoveUploadText,
	MXferMoveCrossloadText,
	MXferRenameText,

	MXferDOAText,

	MXferDOAAsk,
	MXferDOAOverwrite,

	MXferDOASkip,
	MXferDOAOverwriteIfNewer,

	MXferDOAResume,
	MXferDOACreateDifferentName,

	MXferConfirmOverwriteNotify,

	MXferCurrentFile,
	MXferFileSize,
	MXferAllSize,
	MXferCount,
	MXferOf,

	MXferFileTimeSpent,
	MXferRemain,
	MXferAllTimeSpent,
	MXferSpeedCurrent,
	MXferAverage, 

	MBackground,
	MPause,
	MResume,
	MErrorActionReset,

	MProceedCopyDownload,
	MProceedCopyUpload,
	MProceedCopyCrossload,
	MProceedMoveDownload,
	MProceedMoveUpload,
	MProceedMoveCrossload,
	MProceedRename,
	MProceedChangeMode,

	MDestinationExists,
	MSourceInfo,
	MDestinationInfo,
	MOverwriteOptions,

	MRemoveTitle,
	MRemoveText,
	MProceedRemoval,

	MRemoveSitesTitle,
	MRemoveSitesText,
	MCopySitesTitle,
	MCopySitesText,
	MMoveSitesTitle,
	MMoveSitesText,
	MImportCopySitesTitle,
	MImportCopySitesText,
	MImportMoveSitesTitle,
	MImportMoveSitesText,
	MExportCopySitesTitle,
	MExportCopySitesText,
	MExportMoveSitesTitle,
	MExportMoveSitesText,

	MMakeDirTitle,
	MMakeDirText,
	MProceedMakeDir,

	MConnectProgressTitle,
	MGetModeProgressTitle,
	MChangeModeProgressTitle,
	MEnumDirProgressTitle,
	MCreateDirProgressTitle,
	MExecuteProgressTitle,
	MGetLinkProgressTitle,

	MAbortTitle,
	MAbortText,
	MAbortConfirm,
	MAbortNotConfirm,

	MAbortingOperationTitle,
	MBreakConnection,

	MNotificationSuccess,
	MNotificationFailed,
	MNotificationUpload,
	MNotificationDownload,
	MNotificationCrossload,
	MNotificationRename,

	MErrorDownloadTitle,
	MErrorUploadTitle,
	MErrorCrossloadTitle,
	MErrorQueryInfoTitle,
	MErrorCheckDirTitle,
	MErrorEnumDirTitle,
	MErrorMakeDirTitle,
	MErrorRenameTitle,
	MErrorRmDirTitle,
	MErrorRemoveTitle,
	MErrorSetTimes,
	MErrorChangeMode,
	MErrorSymlinkQuery,
	MErrorSymlinkCreate,

	MErrorError,
	MErrorObject,
	MErrorSite,

	MHostLocalName,
	MErrorsStatus,

	MCommandsNotSupportedTitle,
	MCommandsNotSupportedText,
	MCommandNotificationTitle,

	MNewServerIdentityTitle,
	MNewServerIdentitySite,
	MNewServerIdentityText,

	MNewServerIdentityAllowOnce,
	MNewServerIdentityAllowAlways,

	MBackgroundTasksTitle,
	MBackgroundTasksMenuActive,
	MBackgroundTasksMenuPaused,
	MBackgroundTasksMenuComplete,
	MBackgroundTasksMenuAborted,
	MNoBackgroundTasks,

	MConfirmExitFARTitle,
	MConfirmExitFARText,
	MConfirmExitFARQuestion,
	MConfirmExitFARBackgroundTasks,

	MSFTPOptionsTitle,
	MSCPOptionsTitle,
	MSFTPAuthMode,
	MSFTPAuthModeUserPassword,
	MSFTPAuthModeUserPasswordInteractive,
	MSFTPAuthModeKeyFile,
	MSFTPAuthModeSSHAgent,
	MSFTPPrivateKeyPath,
	MSFTPCompression,
	MSFTPCompressionNone,
	MSFTPCompressionIncoming,
	MSFTPCompressionOutgoing,
	MSFTPCompressionAll,
	MSFTPCustomSubsystem,
	MSFTPMaxReadBlockSize,
	MSFTPMaxWriteBlockSize,
	MSFTPTCPNodelay,
	MSFTPTCPQuickAck,
	MSFTPIgnoreTimeAndModeErrors,
	MSFTPConnectRetries,
	MSFTPConnectTimeout,
	MSFTPAllowedHostkeys,
	MSFTPAllowedKexAlgorithms,
	MSFTPAllowedHMAC_CS,
	MSFTPAllowedHMAC_SC,
	MSFTPOpenSSHConfigs,
	MSFTPCfgFilesDefault,
	MSFTPCfgFilesNone,
	MSFTPCfgFilesSpecified,
	MSFTPEnableSandbox,

	MSFileOptionsTitle,
	MSFileCommand,
	MSFileCommandDeinit,
	MSFileExtra,
	MSFileCommandTimeLimit,
	MSFileCommandVarsHint,

	MSMBOptionsTitle,
	MSMBWorkgroup,
	MSMBEnumBySMB,
	MSMBEnumByNMB,

	MNFSOptionsTitle,
	MNFSOverride,
	MNFSHost,
	MNFSUID,
	MNFSGID,
	MNFSGroups,

	MWebDAVOptionsTitle,
	MWebDAVUserAgent,
	MWebDAVUseProxy,
	MWebDAVProxyHost,
	MWebDAVProxyPort,
	MWebDAVAuthProxy,
	MWebDAVProxyUsername,
	MWebDAVProxyPassword,

	MConfirmChangeModeTitle,
	MConfirmChangeModeTextOne,
	MConfirmChangeModeTextMany,
	MThatIsSymlink,
	MRecurseSubdirs,
	MOwnerTitle,
	MOwner,
	MGroup,
	MOwnerMultiple,
	MOwnerUnknown,
	MGroupUnknown,
	MPermissionsTtitle,
	MModeUser,
	MModeGroup,
	MModeOther,
	MModeReadHot,
	MModeWriteHot,
	MModeExecuteHot,
	MModeRead,
	MModeWrite,
	MModeExecute,
	MModeSetUID,
	MModeSetGID,
	MModeSticky,
	MModeOriginal,
	MTimeTitle,
	MTimeAccess,
	MTimeModification,
	MTimeChange,

	MFTPOptionsTitle,
	MFTPSOptionsTitle,
	MFTPExplicitEncryption, 
	MSFTPEncryptionProtocol,
	MSFTPListCommand,
	MSFTPServerCodepage,
	MFTPPassiveMode,
	MFTPListAutodetect,
	MFTPUseMLSDMLST,
	MFTPCommandsPipelining,
	MFTPRestrictDataPeer,
	MFTPTCPNoDelay,
	MFTPQuickAck,

	MSHELLOptionsTitle,
	MSHELLWay,
	MSHELLWaySettings,

	MProxySettingsTitle,
	MProxySettingsDisabled,
	MProxySettingsKind,
	MProxySettingsEdit,

	MAWSOprionTitle,
	MAWSUserAgent,
	MAWSRegion,
	MAWSUseProxy,
	MAWSProxyHost,
	MAWSProxyPort,
	MAWSAuthProxy,
	MAWSProxyUsername,
	MAWSProxyPassword,

};
