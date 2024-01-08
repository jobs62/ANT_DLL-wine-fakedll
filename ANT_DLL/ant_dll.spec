1 stdcall ANTFS_CloseBeacon(  ) ANTFS_CloseBeacon
2 stdcall ANTFS_ConfigBeacon( long long long long ) ANTFS_ConfigBeacon
3 stdcall ANTFS_CryptoAddUserKeyIndex( long ptr ) ANTFS_CryptoAddUserKeyIndex
4 stdcall ANTFS_CryptoSetUserKeyIndex( long ) ANTFS_CryptoSetUserKeyIndex
5 stdcall ANTFS_CryptoSetUserKeyVal( ptr ) ANTFS_CryptoSetUserKeyVal
6 stdcall ANTFS_DirectoryGetSize(  ) ANTFS_DirectoryGetSize
7 stdcall ANTFS_DirectoryReadEntry( long ptr ) ANTFS_DirectoryReadEntry
8 stdcall ANTFS_DirectoryReadLock( long ) ANTFS_DirectoryReadLock
9 stdcall ANTFS_DirectoryRebuild(  ) ANTFS_DirectoryRebuild
10 stdcall ANTFS_EnableRemoteFileCreate( long ) ANTFS_EnableRemoteFileCreate
11 stdcall ANTFS_FileClose( long ) ANTFS_FileClose
12 stdcall ANTFS_FileCreate( long long int64 long long ) ANTFS_FileCreate
13 stdcall ANTFS_FileDelete( long ) ANTFS_FileDelete
14 stdcall ANTFS_FileGetSize( long ) ANTFS_FileGetSize
15 stdcall ANTFS_FileGetSizeInMem( long ) ANTFS_FileGetSizeInMem
16 stdcall ANTFS_FileGetSpecificFlags( long ) ANTFS_FileGetSpecificFlags
17 stdcall ANTFS_FileGetSystemTime(  ) ANTFS_FileGetSystemTime
18 stdcall ANTFS_FileOpen( long long ) ANTFS_FileOpen
19 stdcall ANTFS_FileReadAbsolute( long int64 long ptr ) ANTFS_FileReadAbsolute
20 stdcall ANTFS_FileReadRelative( long long ptr ) ANTFS_FileReadRelative
21 stdcall ANTFS_FileWriteAbsolute( long int64 long str ptr ) ANTFS_FileWriteAbsolute
22 stdcall ANTFS_FileWriteRelative( long long str ptr ) ANTFS_FileWriteRelative
23 stdcall ANTFS_FindFileIndex( long long long ) ANTFS_FindFileIndex
24 stdcall ANTFS_FitFileIntegrityCheck( long ) ANTFS_FitFileIntegrityCheck
25 stdcall ANTFS_FormatFSMemory( long long ) ANTFS_FormatFSMemory
26 stdcall ANTFS_GetCmdPipe( long long ptr ) ANTFS_GetCmdPipe
27 stdcall ANTFS_GetFreeSpace(  ) ANTFS_GetFreeSpace
28 stdcall ANTFS_GetLastError(  ) ANTFS_GetLastError
29 stdcall ANTFS_GetUsedSpace(  ) ANTFS_GetUsedSpace
30 stdcall ANTFS_InitEEPROMDevice( long long ) ANTFS_InitEEPROMDevice
31 stdcall ANTFS_InitFSMemory(  ) ANTFS_InitFSMemory
32 stdcall ANTFS_OpenBeacon(  ) ANTFS_OpenBeacon
33 stdcall ANTFS_PairResponse( long ) ANTFS_PairResponse
34 stdcall ANTFS_ReadDirectoryAbsolute( int64 long ptr ) ANTFS_ReadDirectoryAbsolute
35 stdcall ANTFS_SaveDirectory(  ) ANTFS_SaveDirectory
36 stdcall ANTFS_SetBeaconState( long ) ANTFS_SetBeaconState
37 stdcall ANTFS_SetBeaconTimeout( long ) ANTFS_SetBeaconTimeout
38 stdcall ANTFS_SetCmdPipe( long long str ) ANTFS_SetCmdPipe
39 stdcall ANTFS_SetFileSpecificFlags( long long ) ANTFS_SetFileSpecificFlags
40 stdcall ANTFS_SetFriendlyName( long str ) ANTFS_SetFriendlyName
41 stdcall ANTFS_SetLinkFrequency( long long ) ANTFS_SetLinkFrequency
42 stdcall ANTFS_SetPairingTimeout( long ) ANTFS_SetPairingTimeout
43 stdcall ANTFS_SetPasskey( long str ) ANTFS_SetPasskey
44 stdcall ANTFS_SetSystemTime( int64 ) ANTFS_SetSystemTime
45 stdcall ANT_AddChannelID( long long long long long ) ANT_AddChannelID
46 stdcall ANT_AddChannelID_RTO( long long long long long int64 ) ANT_AddChannelID_RTO
47 stdcall ANT_AssignChannel( long long long ) ANT_AssignChannel
48 stdcall ANT_AssignChannelEventFunction( long ptr long ) ANT_AssignChannelEventFunction
49 stdcall ANT_AssignChannelExt( long long long long ) ANT_AssignChannelExt
50 stdcall ANT_AssignChannelExt_RTO( long long long long int64 ) ANT_AssignChannelExt_RTO
51 stdcall ANT_AssignChannel_RTO( long long long int64 ) ANT_AssignChannel_RTO
52 stdcall ANT_AssignResponseFunction( ptr ptr ) ANT_AssignResponseFunction
53 stdcall ANT_Close(  ) ANT_Close
54 stdcall ANT_CloseChannel( long ) ANT_CloseChannel
55 stdcall ANT_CloseChannel_RTO( long int64 ) ANT_CloseChannel_RTO
56 stdcall ANT_ConfigEventBuffer( long long long ) ANT_ConfigEventBuffer
57 stdcall ANT_ConfigEventBuffer_RTO( long long long int64 ) ANT_ConfigEventBuffer_RTO
58 stdcall ANT_ConfigEventFilter( long ) ANT_ConfigEventFilter
59 stdcall ANT_ConfigEventFilter_RTO( long int64 ) ANT_ConfigEventFilter_RTO
60 stdcall ANT_ConfigFrequencyAgility( long long long long ) ANT_ConfigFrequencyAgility
61 stdcall ANT_ConfigFrequencyAgility_RTO( long long long long int64 ) ANT_ConfigFrequencyAgility_RTO
62 stdcall ANT_ConfigHighDutySearch( long long ) ANT_ConfigHighDutySearch
63 stdcall ANT_ConfigHighDutySearch_RTO( long long int64 ) ANT_ConfigHighDutySearch_RTO
64 stdcall ANT_ConfigList( long long long ) ANT_ConfigList
65 stdcall ANT_ConfigList_RTO( long long long int64 ) ANT_ConfigList_RTO
66 stdcall ANT_ConfigSelectiveDataUpdate( long long ) ANT_ConfigSelectiveDataUpdate
67 stdcall ANT_ConfigSelectiveDataUpdate_RTO( long long int64 ) ANT_ConfigSelectiveDataUpdate_RTO
68 stdcall ANT_ConfigUserNVM( long ptr long ) ANT_ConfigUserNVM
69 stdcall ANT_ConfigUserNVM_RTO( long ptr long int64 ) ANT_ConfigUserNVM_RTO
70 stdcall ANT_ConfigureAdvancedBurst( long long int64 int64 ) ANT_ConfigureAdvancedBurst
71 stdcall ANT_ConfigureAdvancedBurst_RTO( long long int64 int64 int64 ) ANT_ConfigureAdvancedBurst_RTO
72 stdcall ANT_ConfigureAdvancedBurst_ext( long long int64 int64 long long ) ANT_ConfigureAdvancedBurst_ext
73 stdcall ANT_ConfigureAdvancedBurst_ext_RTO( long long int64 int64 long long int64 ) ANT_ConfigureAdvancedBurst_ext_RTO
74 stdcall ANT_CrystalEnable(  ) ANT_CrystalEnable
75 stdcall ANT_CrystalEnable_RTO( int64 ) ANT_CrystalEnable_RTO
76 stdcall ANT_EnableLED( long ) ANT_EnableLED
77 stdcall ANT_EnableLED_RTO( long int64 ) ANT_EnableLED_RTO
78 stdcall ANT_GetDeviceUSBInfo( long ptr ptr ) ANT_GetDeviceUSBInfo
79 stdcall ANT_GetDeviceUSBPID( ptr ) ANT_GetDeviceUSBPID
80 stdcall ANT_GetDeviceUSBVID( ptr ) ANT_GetDeviceUSBVID
81 stdcall ANT_Init( long int64 ) ANT_Init
82 stdcall ANT_InitCWTestMode(  ) ANT_InitCWTestMode
83 stdcall ANT_InitCWTestMode_RTO( int64 ) ANT_InitCWTestMode_RTO
84 stdcall ANT_LibConfigCustom(  ) ANT_LibConfigCustom
85 stdcall ANT_LibVersion( ) ANT_LibVersion
86 stdcall ANT_NVM_Clear( long ) ANT_NVM_Clear
87 stdcall ANT_NVM_Clear_RTO( long int64 ) ANT_NVM_Clear_RTO
88 stdcall ANT_NVM_Dump(  ) ANT_NVM_Dump
89 stdcall ANT_NVM_Dump_RTO( int64 ) ANT_NVM_Dump_RTO
90 stdcall ANT_NVM_EndSector(  ) ANT_NVM_EndSector
91 stdcall ANT_NVM_EndSector_RTO( int64 ) ANT_NVM_EndSector_RTO
92 stdcall ANT_NVM_Lock(  ) ANT_NVM_Lock
93 stdcall ANT_NVM_Lock_RTO( int64 ) ANT_NVM_Lock_RTO
94 stdcall ANT_NVM_SetDefaultSector( long ) ANT_NVM_SetDefaultSector
95 stdcall ANT_NVM_SetDefaultSector_RTO( long int64 ) ANT_NVM_SetDefaultSector_RTO
96 stdcall ANT_NVM_Write( long long ) ANT_NVM_Write
97 stdcall ANT_NVM_Write_RTO( long long int64 ) ANT_NVM_Write_RTO
98 stdcall ANT_Nap( int64 ) ANT_Nap
99 stdcall ANT_OpenChannel( long ) ANT_OpenChannel
100 stdcall ANT_OpenChannel_RTO( long int64 ) ANT_OpenChannel_RTO
101 stdcall ANT_OpenRxScanMode(  ) ANT_OpenRxScanMode
102 stdcall ANT_OpenRxScanMode_RTO( int64 ) ANT_OpenRxScanMode_RTO
103 stdcall ANT_RSSI_SetSearchThreshold( long long ) ANT_RSSI_SetSearchThreshold
104 stdcall ANT_RSSI_SetSearchThreshold_RTO( long long int64 ) ANT_RSSI_SetSearchThreshold_RTO
105 stdcall ANT_RequestMessage( long long ) ANT_RequestMessage
106 stdcall ANT_ResetSystem(  ) ANT_ResetSystem
107 stdcall ANT_RxExtMesgsEnable( long ) ANT_RxExtMesgsEnable
108 stdcall ANT_RxExtMesgsEnable_RTO( long int64 ) ANT_RxExtMesgsEnable_RTO
109 stdcall ANT_SendAcknowledgedData( long ptr ) ANT_SendAcknowledgedData
110 stdcall ANT_SendAcknowledgedData_RTO( long long int64 ) ANT_SendAcknowledgedData_RTO
111 stdcall ANT_SendBroadcastData( long ptr ) ANT_SendBroadcastData
112 stdcall ANT_SendBurstTransfer( long long long ) ANT_SendBurstTransfer
113 stdcall ANT_SendBurstTransfer_RTO( long long long int64 ) ANT_SendBurstTransfer_RTO
114 stdcall ANT_SendExtAcknowledgedData( long ptr ) ANT_SendExtAcknowledgedData
115 stdcall ANT_SendExtAcknowledgedData_RTO( long long int64 ) ANT_SendExtAcknowledgedData_RTO
116 stdcall ANT_SendExtBroadcastData( long ptr ) ANT_SendExtBroadcastData
117 stdcall ANT_SendExtBurstTransfer( long long long ) ANT_SendExtBurstTransfer
118 stdcall ANT_SendExtBurstTransferPacket( long ptr ) ANT_SendExtBurstTransferPacket
119 stdcall ANT_SendExtBurstTransfer_RTO( long long long int64 ) ANT_SendExtBurstTransfer_RTO
120 stdcall ANT_SetCWTestMode( long long ) ANT_SetCWTestMode
121 stdcall ANT_SetCWTestMode_RTO( long long int64 ) ANT_SetCWTestMode_RTO
122 stdcall ANT_SetChannelId( long long long long ) ANT_SetChannelId
123 stdcall ANT_SetChannelId_RTO( long long long long int64 ) ANT_SetChannelId_RTO
124 stdcall ANT_SetChannelPeriod( long long ) ANT_SetChannelPeriod
125 stdcall ANT_SetChannelPeriod_RTO( long long int64 ) ANT_SetChannelPeriod_RTO
126 stdcall ANT_SetChannelRFFreq( long long ) ANT_SetChannelRFFreq
127 stdcall ANT_SetChannelRFFreq_RTO( long long int64 ) ANT_SetChannelRFFreq_RTO
128 stdcall ANT_SetChannelSearchTimeout( long long ) ANT_SetChannelSearchTimeout
129 stdcall ANT_SetChannelSearchTimeout_RTO( long long int64 ) ANT_SetChannelSearchTimeout_RTO
130 stdcall ANT_SetChannelTxPower( long long ) ANT_SetChannelTxPower
131 stdcall ANT_SetChannelTxPower_RTO( long long int64 ) ANT_SetChannelTxPower_RTO
132 stdcall ANT_SetDebugLogDirectory( ptr ) ANT_SetDebugLogDirectory
133 stdcall ANT_SetLowPriorityChannelSearchTimeout( long long ) ANT_SetLowPriorityChannelSearchTimeout
134 stdcall ANT_SetLowPriorityChannelSearchTimeout_RTO( long long int64 ) ANT_SetLowPriorityChannelSearchTimeout_RTO
135 stdcall ANT_SetNetworkKey( long long ) ANT_SetNetworkKey
136 stdcall ANT_SetNetworkKey_RTO( long long int64 ) ANT_SetNetworkKey_RTO
137 stdcall ANT_SetProximitySearch( long long ) ANT_SetProximitySearch
138 stdcall ANT_SetProximitySearch_RTO( long long int64 ) ANT_SetProximitySearch_RTO
139 stdcall ANT_SetSelectiveDataUpdateMask( long ptr ) ANT_SetSelectiveDataUpdateMask
140 stdcall ANT_SetSelectiveDataUpdateMask_RTO( long ptr int64 ) ANT_SetSelectiveDataUpdateMask_RTO
141 stdcall ANT_SetSerialNumChannelId( long long long ) ANT_SetSerialNumChannelId
142 stdcall ANT_SetSerialNumChannelId_RTO( long long long int64 ) ANT_SetSerialNumChannelId_RTO
143 stdcall ANT_SetTransmitPower( long ) ANT_SetTransmitPower
144 stdcall ANT_SetTransmitPower_RTO( long int64 ) ANT_SetTransmitPower_RTO
145 stdcall ANT_SleepMessage(  ) ANT_SleepMessage
146 stdcall ANT_SleepMessage_RTO( int64 ) ANT_SleepMessage_RTO
147 stdcall ANT_UnAssignChannel( long ) ANT_UnAssignChannel
148 stdcall ANT_UnAssignChannel_RTO( long int64 ) ANT_UnAssignChannel_RTO
149 stdcall ANT_UnassignAllResponseFunctions(  ) ANT_UnassignAllResponseFunctions
150 stdcall ANT_WriteMessage( long ptr long ) ANT_WriteMessage
151 stdcall FIT_AdjustPairingSettings( long long long ) FIT_AdjustPairingSettings
152 stdcall FIT_AdjustPairingSettings_RTO( long long long int64 ) FIT_AdjustPairingSettings_RTO
153 stdcall FIT_SetFEState( long ) FIT_SetFEState
154 stdcall FIT_SetFEState_RTO( long int64 ) FIT_SetFEState_RTO
