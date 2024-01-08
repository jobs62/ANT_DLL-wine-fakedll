import re

symb = [
    "ANTFS_CloseBeacon",
    "ANTFS_ConfigBeacon",
    "ANTFS_CryptoAddUserKeyIndex",
    "ANTFS_CryptoSetUserKeyIndex",
    "ANTFS_CryptoSetUserKeyVal",
    "ANTFS_DirectoryGetSize",
    "ANTFS_DirectoryReadEntry",
    "ANTFS_DirectoryReadLock",
    "ANTFS_DirectoryRebuild",
    "ANTFS_EnableRemoteFileCreate",
    "ANTFS_FileClose",
    "ANTFS_FileCreate",
    "ANTFS_FileDelete",
    "ANTFS_FileGetSize",
    "ANTFS_FileGetSizeInMem",
    "ANTFS_FileGetSpecificFlags",
    "ANTFS_FileGetSystemTime",
    "ANTFS_FileOpen",
    "ANTFS_FileReadAbsolute",
    "ANTFS_FileReadRelative",
    "ANTFS_FileWriteAbsolute",
    "ANTFS_FileWriteRelative",
    "ANTFS_FindFileIndex",
    "ANTFS_FitFileIntegrityCheck",
    "ANTFS_FormatFSMemory",
    "ANTFS_GetCmdPipe",
    "ANTFS_GetFreeSpace",
    "ANTFS_GetLastError",
    "ANTFS_GetUsedSpace",
    "ANTFS_InitEEPROMDevice",
    "ANTFS_InitFSMemory",
    "ANTFS_OpenBeacon",
    "ANTFS_PairResponse",
    "ANTFS_ReadDirectoryAbsolute",
    "ANTFS_SaveDirectory",
    "ANTFS_SetBeaconState",
    "ANTFS_SetBeaconTimeout",
    "ANTFS_SetCmdPipe",
    "ANTFS_SetFileSpecificFlags",
    "ANTFS_SetFriendlyName",
    "ANTFS_SetLinkFrequency",
    "ANTFS_SetPairingTimeout",
    "ANTFS_SetPasskey",
    "ANTFS_SetSystemTime",
    "ANT_AddChannelID",
    "ANT_AddChannelID_RTO",
    "ANT_AssignChannel",
    "ANT_AssignChannelEventFunction",
    "ANT_AssignChannelExt",
    "ANT_AssignChannelExt_RTO",
    "ANT_AssignChannel_RTO",
    "ANT_AssignResponseFunction",
    "ANT_Close",
    "ANT_CloseChannel",
    "ANT_CloseChannel_RTO",
    "ANT_ConfigEventBuffer",
    "ANT_ConfigEventBuffer_RTO",
    "ANT_ConfigEventFilter",
    "ANT_ConfigEventFilter_RTO",
    "ANT_ConfigFrequencyAgility",
    "ANT_ConfigFrequencyAgility_RTO",
    "ANT_ConfigHighDutySearch",
    "ANT_ConfigHighDutySearch_RTO",
    "ANT_ConfigList",
    "ANT_ConfigList_RTO",
    "ANT_ConfigSelectiveDataUpdate",
    "ANT_ConfigSelectiveDataUpdate_RTO",
    "ANT_ConfigUserNVM",
    "ANT_ConfigUserNVM_RTO",
    "ANT_ConfigureAdvancedBurst",
    "ANT_ConfigureAdvancedBurst_RTO",
    "ANT_ConfigureAdvancedBurst_ext",
    "ANT_ConfigureAdvancedBurst_ext_RTO",
    "ANT_CrystalEnable",
    "ANT_CrystalEnable_RTO",
    "ANT_EnableLED",
    "ANT_EnableLED_RTO",
    "ANT_GetDeviceUSBInfo",
    "ANT_GetDeviceUSBPID",
    "ANT_GetDeviceUSBVID",
    "ANT_Init",
    "ANT_InitCWTestMode",
    "ANT_InitCWTestMode_RTO",
    "ANT_LibConfigCustom",
    "ANT_LibVersion",
    "ANT_NVM_Clear",
    "ANT_NVM_Clear_RTO",
    "ANT_NVM_Dump",
    "ANT_NVM_Dump_RTO",
    "ANT_NVM_EndSector",
    "ANT_NVM_EndSector_RTO",
    "ANT_NVM_Lock",
    "ANT_NVM_Lock_RTO",
    "ANT_NVM_SetDefaultSector",
    "ANT_NVM_SetDefaultSector_RTO",
    "ANT_NVM_Write",
    "ANT_NVM_Write_RTO",
    "ANT_Nap",
    "ANT_OpenChannel",
    "ANT_OpenChannel_RTO",
    "ANT_OpenRxScanMode",
    "ANT_OpenRxScanMode_RTO",
    "ANT_RSSI_SetSearchThreshold",
    "ANT_RSSI_SetSearchThreshold_RTO",
    "ANT_RequestMessage",
    "ANT_ResetSystem",
    "ANT_RxExtMesgsEnable",
    "ANT_RxExtMesgsEnable_RTO",
    "ANT_SendAcknowledgedData",
    "ANT_SendAcknowledgedData_RTO",
    "ANT_SendBroadcastData",
    "ANT_SendBurstTransfer",
    "ANT_SendBurstTransfer_RTO",
    "ANT_SendExtAcknowledgedData",
    "ANT_SendExtAcknowledgedData_RTO",
    "ANT_SendExtBroadcastData",
    "ANT_SendExtBurstTransfer",
    "ANT_SendExtBurstTransferPacket",
    "ANT_SendExtBurstTransfer_RTO",
    "ANT_SetCWTestMode",
    "ANT_SetCWTestMode_RTO",
    "ANT_SetChannelId",
    "ANT_SetChannelId_RTO",
    "ANT_SetChannelPeriod",
    "ANT_SetChannelPeriod_RTO",
    "ANT_SetChannelRFFreq",
    "ANT_SetChannelRFFreq_RTO",
    "ANT_SetChannelSearchTimeout",
    "ANT_SetChannelSearchTimeout_RTO",
    "ANT_SetChannelTxPower",
    "ANT_SetChannelTxPower_RTO",
    "ANT_SetDebugLogDirectory",
    "ANT_SetLowPriorityChannelSearchTimeout",
    "ANT_SetLowPriorityChannelSearchTimeout_RTO",
    "ANT_SetNetworkKey",
    "ANT_SetNetworkKey_RTO",
    "ANT_SetProximitySearch",
    "ANT_SetProximitySearch_RTO",
    "ANT_SetSelectiveDataUpdateMask",
    "ANT_SetSelectiveDataUpdateMask_RTO",
    "ANT_SetSerialNumChannelId",
    "ANT_SetSerialNumChannelId_RTO",
    "ANT_SetTransmitPower",
    "ANT_SetTransmitPower_RTO",
    "ANT_SleepMessage",
    "ANT_SleepMessage_RTO",
    "ANT_UnAssignChannel",
    "ANT_UnAssignChannel_RTO",
    "ANT_UnassignAllResponseFunctions",
    "ANT_WriteMessage",
    "FIT_AdjustPairingSettings",
    "FIT_AdjustPairingSettings_RTO",
    "FIT_SetFEState",
    "FIT_SetFEState_RTO",
]

class Delc:
    def __init__(self, name, args):
        self.name = name
        self.args = args

    def __str__(self):
        return f"cdecl {self.name}( {' '.join(self.args)} ) {self.name}"

delc_regex = r"EXPORT ([a-zA-Z]*) ([a-zA-Z_]*)\((.*)\);"
delc = {}
fp = open("ant.h")
matches = re.finditer(delc_regex, fp.read(), re.MULTILINE)
for _, match in enumerate(matches, start=1):
    groups = match.groups()
    fcname = groups[1]
    args = []
    for arg in groups[2].split(","):
        arg = arg.lstrip().rstrip().split(' ')[0]
        if len(arg) == 0:
            continue

        if arg == "UCHAR*" or "args" == "char*":
            t = "str"
        if arg[-1] == "*" or arg == "RESPONSE_FUNC" or arg == "CHANNEL_EVENT_FUNC":
            t = "ptr"
        elif arg == "ULONG" or arg == "LONG":
            t = "int64"
        elif arg == "BOOL":
            t = "long"
        elif arg == "UCHAR" or arg == "USHORT":
            t = "long"
        elif arg == "const":
            t = "str"
        elif arg == "void":
            t = "void"
        else:
            print(f"je c'est po: {arg}")

        if not t == "void":
            args.append(t)
    delc[fcname] = Delc(fcname, args)

for i, name in enumerate(symb):
    try:
        d = delc[name]
        print(f"{i+1} {str(d)}")
    except KeyError:
        print(f"{i+1} stub {name}")