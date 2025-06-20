/*
 * Copyright (c) 2012-2018 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

#if !defined( HDD_CFG80211_H__ )
#define HDD_CFG80211_H__


/**===========================================================================

  \file  wlan_hdd_cfg80211.h

  \brief cfg80211 functions declarations

  ==========================================================================*/

/* $HEADER$ */


//value for initial part of frames and number of bytes to be compared
#define GAS_INITIAL_REQ "\x04\x0a"
#define GAS_INITIAL_REQ_SIZE 2

#define GAS_INITIAL_RSP "\x04\x0b"
#define GAS_INITIAL_RSP_SIZE 2

#define GAS_COMEBACK_REQ "\x04\x0c"
#define GAS_COMEBACK_REQ_SIZE 2

#define GAS_COMEBACK_RSP "\x04\x0d"
#define GAS_COMEBACK_RSP_SIZE 2

#define P2P_PUBLIC_ACTION_FRAME "\x04\x09\x50\x6f\x9a\x09"
#define P2P_PUBLIC_ACTION_FRAME_SIZE 6

#define P2P_ACTION_FRAME "\x7f\x50\x6f\x9a\x09"
#define P2P_ACTION_FRAME_SIZE 5

#define SA_QUERY_FRAME_REQ "\x08\x00"
#define SA_QUERY_FRAME_REQ_SIZE 2

#define SA_QUERY_FRAME_RSP "\x08\x01"
#define SA_QUERY_FRAME_RSP_SIZE 2

#define HDD_P2P_WILDCARD_SSID "DIRECT-" //TODO Put it in proper place;
#define HDD_P2P_WILDCARD_SSID_LEN 7

#define WNM_BSS_ACTION_FRAME "\x0a\x07"
#define WNM_BSS_ACTION_FRAME_SIZE 2

#define WNM_NOTIFICATION_FRAME "\x0a\x1a"
#define WNM_NOTIFICATION_FRAME_SIZE 2

#define WPA_OUI_TYPE   "\x00\x50\xf2\x01"
#define BLACKLIST_OUI_TYPE   "\x00\x50\x00\x00"
#define WHITELIST_OUI_TYPE   "\x00\x50\x00\x01"
#define WPA_OUI_TYPE_SIZE  4

#define WLAN_BSS_MEMBERSHIP_SELECTOR_HT_PHY 127
#define BASIC_RATE_MASK   0x80
#define RATE_MASK         0x7f

#ifdef WLAN_ENABLE_AGEIE_ON_SCAN_RESULTS
/* GPS application requirement */
#define QCOM_VENDOR_IE_ID 221
#define QCOM_OUI1         0x00
#define QCOM_OUI2         0xA0
#define QCOM_OUI3         0xC6
#define QCOM_VENDOR_IE_AGE_TYPE  0x100
#define QCOM_VENDOR_IE_AGE_LEN   11

#ifdef FEATURE_WLAN_TDLS
#define WLAN_IS_TDLS_SETUP_ACTION(action) \
         ((SIR_MAC_TDLS_SETUP_REQ <= action) && (SIR_MAC_TDLS_SETUP_CNF >= action))
#if !defined (TDLS_MGMT_VERSION2)
#define TDLS_MGMT_VERSION2 0
#endif
#endif

/*
 * Max number of supported csa_counters in beacons
 * and probe responses. Set to the same value as
 * IEEE80211_MAX_CSA_COUNTERS_NUM
 */
#define WLAN_HDD_MAX_NUM_CSA_COUNTERS 2

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 12, 0)) \
	|| defined(BACKPORTED_CHANNEL_SWITCH_PRESENT)
#define CHANNEL_SWITCH_SUPPORTED
#endif

#if defined(CFG80211_DEL_STA_V2) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)) || \
	defined(WITH_BACKPORTS)
#define USE_CFG80211_DEL_STA_V2
#endif

#define MAX_CHANNEL NUM_2_4GHZ_CHANNELS + NUM_5GHZ_CHANNELS

#define IS_CHANNEL_VALID(channel) ((channel >= 0 && channel < 15) \
                     || (channel >= 36 && channel <= 184))

#ifdef WLAN_FEATURE_LINK_LAYER_STATS
/* Number of Radios */
#define NUM_RADIOS  0x1
#endif /* WLAN_FEATURE_LINK_LAYER_STATS */

#define WOWL_PTRN_MAX_SIZE          128
#define WOWL_PTRN_MASK_MAX_SIZE      16
#define WOWL_MAX_PTRNS_ALLOWED       16

typedef struct {
   u8 element_id;
   u8 len;
   u8 oui_1;
   u8 oui_2;
   u8 oui_3;
   u32 type;
   u32 age;
}__attribute__((packed)) qcom_ie_age ;
#endif

enum qca_nl80211_vendor_subcmds {
    QCA_NL80211_VENDOR_SUBCMD_UNSPEC = 0,
    QCA_NL80211_VENDOR_SUBCMD_TEST = 1,

    /* subcmds 2..8 not yet allocated */
    QCA_NL80211_VENDOR_SUBCMD_ROAMING = 9,

    QCA_NL80211_VENDOR_SUBCMD_AVOID_FREQUENCY = 10,
    QCA_NL80211_VENDOR_SUBCMD_DFS_CAPABILITY =  11,
    QCA_NL80211_VENDOR_SUBCMD_NAN =  12,
    QCA_NL80211_VENDOR_SUBCMD_STATS_EXT = 13,

    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET = 14,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET = 15,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR = 16,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_RADIO_RESULTS = 17,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_IFACE_RESULTS = 18,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_PEERS_RESULTS = 19,
    /* subcommands for extscan start here */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_START = 20,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_STOP = 21,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_VALID_CHANNELS = 22,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_CAPABILITIES = 23,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_CACHED_RESULTS = 24,
    /* Used when report_threshold is reached in scan cache. */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_RESULTS_AVAILABLE = 25,
    /* Used to report scan results when each probe rsp. is received,
      * if report_events enabled in wifi_scan_cmd_params.
      */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_FULL_SCAN_RESULT = 26,
    /* Indicates progress of scanning state-machine. */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_EVENT = 27,
    /* Indicates BSSID Hotlist. */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_HOTLIST_AP_FOUND = 28,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SET_BSSID_HOTLIST = 29,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_RESET_BSSID_HOTLIST = 30,
    /* EXT TDLS */
    QCA_NL80211_VENDOR_SUBCMD_TDLS_ENABLE = 34,
    QCA_NL80211_VENDOR_SUBCMD_TDLS_DISABLE = 35,
    QCA_NL80211_VENDOR_SUBCMD_TDLS_GET_STATUS = 36,
    QCA_NL80211_VENDOR_SUBCMD_TDLS_STATE = 37,
    /* Get supported features */
    QCA_NL80211_VENDOR_SUBCMD_GET_SUPPORTED_FEATURES = 38,
    QCA_NL80211_VENDOR_SUBCMD_MAC_OUI = 39,
    /* Set nodfs_flag */
    QCA_NL80211_VENDOR_SUBCMD_NO_DFS_FLAG = 40,

    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_HOTLIST_AP_LOST = 41,

    /* Get Concurrency Matrix */
    QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX = 42,
    QCA_NL80211_VENDOR_SUBCMD_APFIND = 52,

    QCA_NL80211_VENDOR_SUBCMD_DO_ACS = 54, //IKLOCSEN-3054, MOTO Gambugge

    /* Start Wifi Logger */
    QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_START = 62,

    /* Get Wifi Specific Info */
    QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO = 61,
    /* Start Wifi Memory Dump */
    QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP = 63,

    /*
     * APIs corresponding to the sub commands 65-68 are deprecated.
     * These sub commands are reserved and not supposed to be used
     * for any other purpose
     */

    /* Wi-Fi Configuration subcommands */
    QCA_NL80211_VENDOR_SUBCMD_SET_WIFI_CONFIGURATION = 74,
    QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_CONFIGURATION = 75,

    QCA_NL80211_VENDOR_SUBCMD_GET_LOGGER_FEATURE_SET = 76,

    QCA_NL80211_VENDOR_SUBCMD_GET_RING_DATA = 77,

    QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI = 80,

    QCA_NL80211_VENDOR_SUBCMD_OFFLOADED_PACKETS = 79,

    /* subcommand to get link properties */
    QCA_NL80211_VENDOR_SUBCMD_LINK_PROPERTIES = 101,

    QCA_NL80211_VENDOR_SUBCMD_SETBAND = 105,

    /* send BSS Information */
    QCA_NL80211_VENDOR_SUBCMD_GET_STATION = 121,

    /* Start / Stop the NUD stats collections */
    QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_SET = 149,
    /* Get the NUD stats, represented by the enum qca_attr_nud_stats_get */
    QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_GET = 150,
    /*
     * Event indicating to the user space that the driver has detected an
     * internal failure. This event carries the information indicating the
     * reason that triggered this detection. The attributes for this
     * command are defined in enum qca_wlan_vendor_attr_hang.
     */
    QCA_NL80211_VENDOR_SUBCMD_HANG = 157,
};

enum qca_wlan_vendor_hang_reason {
	/* Unspecified reason */
	QCA_WLAN_HANG_REASON_UNSPECIFIED = 0,
	/* No Map for the MAC entry for the received frame */
	QCA_WLAN_HANG_RX_HASH_NO_ENTRY_FOUND = 1,
	/* peer deletion timeout happened */
	QCA_WLAN_HANG_PEER_DELETION_TIMEDOUT = 2,
	/* peer unmap timeout */
	QCA_WLAN_HANG_PEER_UNMAP_TIMEDOUT = 3,
	/* Scan request timed out */
	QCA_WLAN_HANG_SCAN_REQ_EXPIRED = 4,
	/* Consecutive Scan attempt failures */
	QCA_WLAN_HANG_SCAN_ATTEMPT_FAILURES = 5,
	/* Unable to get the message buffer */
	QCA_WLAN_HANG_GET_MSG_BUFF_FAILURE = 6,
	/* Current command processing is timedout */
	QCA_WLAN_HANG_ACTIVE_LIST_TIMEOUT = 7,
	/* Timeout for an ACK from FW for suspend request */
	QCA_WLAN_HANG_SUSPEND_TIMEOUT = 8,
	/* Timeout for an ACK from FW for resume request */
	QCA_WLAN_HANG_RESUME_TIMEOUT = 9,
	/* Transmission timeout for consecutive data frames */
	QCA_WLAN_HANG_TRANSMISSIONS_TIMEOUT = 10,
	/* Timeout for the TX completion status of data frame */
	QCA_WLAN_HANG_TX_COMPLETE_TIMEOUT = 11,
	/* DXE failure for tx/Rx, DXE resource unavailability */
	QCA_WLAN_HANG_DXE_FAILURE = 12,
	/* WMI pending commands exceed the maximum count */
	QCA_WLAN_HANG_WMI_EXCEED_MAX_PENDING_CMDS = 13,
	/* WDI failure for commands, WDI resource unavailability */
	QCA_WLAN_HANG_WDI_FAILURE = 14,
};

/**
 * enum qca_wlan_vendor_attr_hang - Used by the vendor command
 * QCA_NL80211_VENDOR_SUBCMD_HANG.
 */
enum qca_wlan_vendor_attr_hang {
	QCA_WLAN_VENDOR_ATTR_HANG_INVALID = 0,
	/*
	 * Reason for the Hang - Represented by enum
	 * qca_wlan_vendor_hang_reason.
	 */
	 QCA_WLAN_VENDOR_ATTR_HANG_REASON = 1,
	 QCA_WLAN_VENDOR_ATTR_HANG_AFTER_LAST,
	 QCA_WLAN_VENDOR_ATTR_HANG_MAX =
		QCA_WLAN_VENDOR_ATTR_HANG_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_get_station - Sub commands used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_STATION to get the corresponding
 * station information. The information obtained through these
 * commands signify the current info in connected state and
 * latest cached information during the connected state , if queried
 * when in disconnected state.
 *
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO: bss info
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_ASSOC_FAIL_REASON: assoc fail reason
 */
enum qca_wlan_vendor_attr_get_station {
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_ASSOC_FAIL_REASON,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_REMOTE,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_GET_STATION_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_MAX =
		QCA_WLAN_VENDOR_ATTR_GET_STATION_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_802_11_mode - dot11 mode
 * @QCA_WLAN_802_11_MODE_11B: mode B
 * @QCA_WLAN_802_11_MODE_11G: mode G
 * @QCA_WLAN_802_11_MODE_11N: mode N
 * @QCA_WLAN_802_11_MODE_11A: mode A
 * @QCA_WLAN_802_11_MODE_11AC: mode AC
 * @QCA_WLAN_802_11_MODE_INVALID: Invalid dot11 mode
 */
enum qca_wlan_802_11_mode {
	QCA_WLAN_802_11_MODE_11B,
	QCA_WLAN_802_11_MODE_11G,
	QCA_WLAN_802_11_MODE_11N,
	QCA_WLAN_802_11_MODE_11A,
	QCA_WLAN_802_11_MODE_11AC,
	QCA_WLAN_802_11_MODE_INVALID,
};


/**
 * enum qca_wlan_auth_type - Authentication key management type
 * @QCA_WLAN_AUTH_TYPE_INVALID: Invalid key management type
 * @QCA_WLAN_AUTH_TYPE_OPEN: Open key
 * @QCA_WLAN_AUTH_TYPE_SHARED: shared key
 * @QCA_WLAN_AUTH_TYPE_WPA: wpa key
 * @QCA_WLAN_AUTH_TYPE_WPA_PSK: wpa psk key
 * @QCA_WLAN_AUTH_TYPE_WPA_NONE: wpa none key
 * @QCA_WLAN_AUTH_TYPE_RSN: rsn key
 * @QCA_WLAN_AUTH_TYPE_RSN_PSK: rsn psk key
 * @QCA_WLAN_AUTH_TYPE_FT: ft key
 * @QCA_WLAN_AUTH_TYPE_FT_PSK: ft psk key
 * @QCA_WLAN_AUTH_TYPE_SHA256: shared 256 key
 * @QCA_WLAN_AUTH_TYPE_SHA256_PSK: shared 256 psk
 * @QCA_WLAN_AUTH_TYPE_WAI: wai key
 * @QCA_WLAN_AUTH_TYPE_WAI_PSK wai psk key
 * @QCA_WLAN_AUTH_TYPE_CCKM_WPA: cckm wpa key
 * @QCA_WLAN_AUTH_TYPE_CCKM_RSN: cckm rsn key
 */
enum qca_wlan_auth_type {
	QCA_WLAN_AUTH_TYPE_INVALID,
	QCA_WLAN_AUTH_TYPE_OPEN,
	QCA_WLAN_AUTH_TYPE_SHARED,
	QCA_WLAN_AUTH_TYPE_WPA,
	QCA_WLAN_AUTH_TYPE_WPA_PSK,
	QCA_WLAN_AUTH_TYPE_WPA_NONE,
	QCA_WLAN_AUTH_TYPE_RSN,
	QCA_WLAN_AUTH_TYPE_RSN_PSK,
	QCA_WLAN_AUTH_TYPE_FT,
	QCA_WLAN_AUTH_TYPE_FT_PSK,
	QCA_WLAN_AUTH_TYPE_SHA256,
	QCA_WLAN_AUTH_TYPE_SHA256_PSK,
	QCA_WLAN_AUTH_TYPE_WAI,
	QCA_WLAN_AUTH_TYPE_WAI_PSK,
	QCA_WLAN_AUTH_TYPE_CCKM_WPA,
	QCA_WLAN_AUTH_TYPE_CCKM_RSN,
	QCA_WLAN_AUTH_TYPE_AUTOSWITCH,
};

/**
 * enum qca_wlan_vendor_attr_get_station_info - Station Info queried
 * through QCA_NL80211_VENDOR_SUBCMD_GET_STATION.
 *
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_INVALID: Invalid Attribute
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_LINK_STANDARD_NL80211_ATTR:
 *  Get the standard NL attributes Nested with this attribute.
 *  Ex : Query BW , BITRATE32 , NSS , Signal , Noise of the Link -
 *  NL80211_ATTR_SSID / NL80211_ATTR_SURVEY_INFO (Connected Channel) /
 *  NL80211_ATTR_STA_INFO
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_STANDARD_NL80211_ATTR:
 *  Get the standard NL attributes Nested with this attribute.
 *  Ex : Query HT/VHT Capability advertized by the AP.
 *  NL80211_ATTR_VHT_CAPABILITY / NL80211_ATTR_HT_CAPABILITY
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ROAM_COUNT:
 *  Number of successful Roam attempts before a
 *  disconnect, Unsigned 32 bit value
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AKM:
 *  Authentication Key Management Type used for the connected session.
 *  Signified by enum qca_wlan_auth_type
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_802_11_MODE: 802.11 Mode of the
 *  connected Session, signified by enum qca_wlan_802_11_mode
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_HS20_INDICATION:
 *  HS20 Indication Element
 * @QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ASSOC_FAIL_REASON:
 *  Status Code Corresponding to the Association Failure.
 *  Unsigned 32 bit value.
 */
enum qca_wlan_vendor_attr_get_station_info {
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_LINK_STANDARD_NL80211_ATTR,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_STANDARD_NL80211_ATTR,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ROAM_COUNT,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AKM,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_802_11_MODE,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_HS20_INDICATION,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_HT_OPERATION,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_VHT_OPERATION,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ASSOC_FAIL_REASON,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_MAX_PHY_RATE,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_TX_PACKETS,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_TX_BYTES,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_RX_PACKETS,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_RX_BYTES,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_LAST_TX_RATE,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_LAST_RX_RATE,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_WMM,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_SUPPORTED_MODE,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_AMPDU,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_TX_STBC,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_RX_STBC,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_CH_WIDTH,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_SGI_ENABLE,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_MAX =
		QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AFTER_LAST - 1,
};

/* define short names for get station info attributes */
#define LINK_INFO_STANDARD_NL80211_ATTR \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_LINK_STANDARD_NL80211_ATTR
#define AP_INFO_STANDARD_NL80211_ATTR \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_STANDARD_NL80211_ATTR
#define INFO_ROAM_COUNT \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ROAM_COUNT
#define INFO_AKM \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AKM
#define WLAN802_11_MODE \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_802_11_MODE
#define AP_INFO_HS20_INDICATION \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_AP_HS20_INDICATION
#define HT_OPERATION \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_HT_OPERATION
#define VHT_OPERATION \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_VHT_OPERATION
#define INFO_ASSOC_FAIL_REASON \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_ASSOC_FAIL_REASON
#define REMOTE_SUPPORTED_MODE \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_SUPPORTED_MODE
#define REMOTE_CH_WIDTH\
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_CH_WIDTH
#define REMOTE_LAST_RX_RATE \
	QCA_WLAN_VENDOR_ATTR_GET_STATION_INFO_REMOTE_LAST_RX_RATE

enum qca_nl80211_vendor_subcmds_index {
#ifdef FEATURE_WLAN_CH_AVOID
    QCA_NL80211_VENDOR_SUBCMD_AVOID_FREQUENCY_INDEX,
#endif /* FEATURE_WLAN_CH_AVOID */
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_SET_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_GET_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LL_STATS_CLR_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LL_RADIO_STATS_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LL_IFACE_STATS_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LL_PEER_INFO_STATS_INDEX,
    /* EXTSCAN Events */
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_START_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_STOP_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_CAPABILITIES_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_CACHED_RESULTS_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_RESULTS_AVAILABLE_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_FULL_SCAN_RESULT_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_EVENT_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_HOTLIST_AP_FOUND_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SET_BSSID_HOTLIST_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_RESET_BSSID_HOTLIST_INDEX,
    /*EXT TDLS*/
    QCA_NL80211_VENDOR_SUBCMD_TDLS_STATE_CHANGE_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_NAN_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP_INDEX,

    QCA_NL80211_VENDOR_SUBCMD_MONITOR_RSSI_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_HOTLIST_AP_LOST_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_GET_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_HANG_REASON_INDEX,
    QCA_NL80211_VENDOR_SUBCMD_LINK_PROPERTIES_INDEX,

    QCA_NL80211_VENDOR_SUBCMD_DO_ACS_INDEX, //IKLOCSEN-3054, MOTO Gambugge
};

/**
 * qca_wlan_vendor_attr_nud_stats_set: attribute to vendor subcmd
 * QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_SET. This carry the requisite
 * information to start / stop the NUD stats collection.
 */
enum qca_attr_nud_stats_set {
    QCA_ATTR_NUD_STATS_SET_INVALID = 0,

    /* Flag to Start / Stop the NUD stats collection
     * Start - If included , Stop - If not included
     */
    QCA_ATTR_NUD_STATS_SET_START = 1,
    /* IPv4 address of Default Gateway (in network byte order) */
    QCA_ATTR_NUD_STATS_GW_IPV4 = 2,

    /* keep last */
    QCA_ATTR_NUD_STATS_SET_LAST,
    QCA_ATTR_NUD_STATS_SET_MAX =
            QCA_ATTR_NUD_STATS_SET_LAST - 1,
};

/**
 * qca_attr_nud_stats_get: attribute to vendor subcmd
 * QCA_NL80211_VENDOR_SUBCMD_NUD_STATS_GET. This carry the requisite
 * NUD stats collected when queried.
 */
enum qca_attr_nud_stats_get {
    QCA_ATTR_NUD_STATS_GET_INVALID = 0,
    /* ARP Request Count from net dev */
    QCA_ATTR_NUD_STATS_ARP_REQ_COUNT_FROM_NETDEV = 1,
    /* ARP Request Count sent to lower MAC from upper MAC */
    QCA_ATTR_NUD_STATS_ARP_REQ_COUNT_TO_LOWER_MAC = 2,
    /* ARP Request Count received by lower MAC from upper MAC */
    QCA_ATTR_NUD_STATS_ARP_REQ_RX_COUNT_BY_LOWER_MAC = 3,
    /* ARP Request Count successfully transmitted by the device */
    QCA_ATTR_NUD_STATS_ARP_REQ_COUNT_TX_SUCCESS = 4,
    /* ARP Response Count received by lower MAC */
    QCA_ATTR_NUD_STATS_ARP_RSP_RX_COUNT_BY_LOWER_MAC = 5,
    /* ARP Response Count received by upper MAC */
    QCA_ATTR_NUD_STATS_ARP_RSP_RX_COUNT_BY_UPPER_MAC = 6,
    /* ARP Response Count delivered to netdev */
    QCA_ATTR_NUD_STATS_ARP_RSP_COUNT_TO_NETDEV = 7,
    /* ARP Response Count delivered to netdev */
    QCA_ATTR_NUD_STATS_ARP_RSP_COUNT_OUT_OF_ORDER_DROP = 8,
    /*
     * Flag indicating if the Stations Link to AP is active.
     * Active Link - If exists, Inactive link - If not included
     */
    QCA_ATTR_NUD_STATS_AP_LINK_ACTIVE= 9,
    QCA_ATTR_NUD_STATS_AP_LINK_DAD= 10,
     /* keep last */
    QCA_ATTR_NUD_STATS_GET_LAST,
    QCA_ATTR_NUD_STATS_GET_MAX =
            QCA_ATTR_NUD_STATS_GET_LAST - 1,
 };

enum qca_wlan_vendor_attr
{
    QCA_WLAN_VENDOR_ATTR_INVALID = 0,
    /* used by QCA_NL80211_VENDOR_SUBCMD_DFS_CAPABILITY */
    QCA_WLAN_VENDOR_ATTR_DFS     = 1,
    /* used by QCA_NL80211_VENDOR_SUBCMD_NAN */
    QCA_WLAN_VENDOR_ATTR_NAN     = 2,
    /* used by QCA_NL80211_VENDOR_SUBCMD_STATS_EXT */
    QCA_WLAN_VENDOR_ATTR_STATS_EXT     = 3,
    /* used by QCA_NL80211_VENDOR_SUBCMD_STATS_EXT */
    QCA_WLAN_VENDOR_ATTR_IFINDEX     = 4,
    /* used by QCA_NL80211_VENDOR_SUBCMD_ROAMING, u32 with values defined
     * by enum qca_roaming_policy. */
    QCA_WLAN_VENDOR_ATTR_ROAMING_POLICY = 5,
    QCA_WLAN_VENDOR_ATTR_MAC_ADDR = 6,
    /* Unsigned 32-bit value from enum qca_set_band. */
    QCA_WLAN_VENDOR_ATTR_SETBAND_VALUE = 12,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_MAX = QCA_WLAN_VENDOR_ATTR_AFTER_LAST - 1,
};

/*EXT TDLS*/
enum qca_wlan_vendor_attr_tdls_enable
{
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAC_ADDR,
    /* signed 32-bit value, but lets keep as unsigned for now */
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_GLOBAL_OPERATING_CLASS,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAX_LATENCY_MS,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MIN_BANDWIDTH_KBPS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_MAX =
         QCA_WLAN_VENDOR_ATTR_TDLS_ENABLE_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_disable
{
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_MAC_ADDR,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_MAX =
       QCA_WLAN_VENDOR_ATTR_TDLS_DISABLE_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_get_status
{
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_MAC_ADDR,
    /* signed 32-bit value, but lets keep as unsigned for now */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_STATE,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_REASON,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_GLOBAL_OPERATING_CLASS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_MAX =
      QCA_WLAN_VENDOR_ATTR_TDLS_GET_STATUS_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_tdls_state
{
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_INVALID = 0,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_MAC_ADDR,
    /* signed 32-bit value, but lets keep as unsigned for now */
    QCA_WLAN_VENDOR_ATTR_TDLS_NEW_STATE,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_REASON,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_CHANNEL,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_GLOBAL_OPERATING_CLASS,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_TDLS_STATE_MAX =
        QCA_WLAN_VENDOR_ATTR_TDLS_STATE_AFTER_LAST - 1,
};


#ifdef WLAN_FEATURE_LINK_LAYER_STATS

enum qca_wlan_vendor_attr_ll_stats_set
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_MPDU_SIZE_THRESHOLD = 1,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_CONFIG_AGGRESSIVE_STATS_GATHERING,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_MAX =
                    QCA_WLAN_VENDOR_ATTR_LL_STATS_SET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_get
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_INVALID = 0,
    /* Unsigned 32bit value provided by the caller issuing the GET stats
     * command. When reporting the stats results, the driver uses the same
     * value to indicate which GET request the results correspond to.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_ID,
    /* Unsigned 34bit value - bit mask to identify what
     * statistics are requested for retrieval.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_CONFIG_REQ_MASK,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_MAX =
                  QCA_WLAN_VENDOR_ATTR_LL_STATS_GET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_ll_stats_clr
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_REQ_MASK,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_REQ,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_RSP_MASK,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_CONFIG_STOP_RSP,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_MAX       =
                        QCA_WLAN_VENDOR_ATTR_LL_STATS_CLR_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_ll_stats_results_type - ll stats result type
 *
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_INVALID: Initial invalid value
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_RADIO: Link layer stats type radio
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_IFACE: Link layer stats type interface
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_PEER: Link layer stats type peer
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_AFTER_LAST: Last value
 * @QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_MAX: Max value
 */
enum qca_wlan_vendor_attr_ll_stats_results_type {
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_INVALID = 0,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_RADIO = 1,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_IFACE,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_PEER,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_MAX =
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE_AFTER_LAST - 1
};

enum qca_wlan_vendor_attr_ll_stats_results
{
    QCA_WLAN_VENDOR_ATTR_LL_STATS_INVALID = 0,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_REQ_ID = 1,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_BEACON_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_RX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_MGMT_ACTION_TX,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_MGMT,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_DATA,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_RSSI_ACK,
    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_* are
     * nested within the interface stats.
     */

    /* Interface mode, e.g., STA, SOFTAP, IBSS, etc.
     * Type = enum wifi_interface_mode */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MODE,
    /* Interface MAC address. An array of 6 Unsigned int8 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_MAC_ADDR,
    /* Type = enum wifi_connection_state,
     * e.g., DISCONNECTED, AUTHENTICATING, etc.
     * valid for STA, CLI only.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_STATE,
    /* Type = enum wifi_roam_state. Roaming state,
     * e.g., IDLE or ACTIVE (is that valid for STA only?)
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_ROAMING,
    /* Unsigned 32bit value. WIFI_CAPABILITY_XXX */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_CAPABILITIES,
    /* NULL terminated SSID. An array of 33 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_SSID,
    /* BSSID. An array of 6 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_BSSID,
    /* Country string advertised by AP. An array of 3 Unsigned 8bit values */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_AP_COUNTRY_STR,
    /* Country string for this association. An array of 3 Unsigned 8bit values*/
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_INFO_COUNTRY_STR,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_* could
     * be nested within the interface stats.
     */

    /* Type = enum wifi_traffic_ac, e.g., V0, VI, BE and BK */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_AC,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_MCAST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_TX_AMPDU,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_MPDU_LOST,
    /* Unsigned int 32 value corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES,
    /* Unsigned int 32 value corresponding to respective AC  */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_SHORT,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_RETRIES_LONG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MIN,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_MAX,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_TIME_AVG,
    /* Unsigned int 32 values corresponding to respective AC */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_AC_CONTENTION_NUM_SAMPLES,
    /* Unsigned 32bit value. Number of peers */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_NUM_PEERS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_* are
     * nested within the interface stats.
     */

    /* Type = enum wifi_peer_type. Peer type, e.g., STA, AP, P2P GO etc. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_TYPE,
    /* MAC addr corresponding to respective peer.
     *  An array of 6 Unsigned 8bit values.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_MAC_ADDRESS,
    /* Unsigned int 32bit value representing capabilities
     * corresponding to respective peer.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_CAPABILITIES,
    /* Unsigned 32bit value. Number of rates */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_NUM_RATES,

    /* Attributes nested within the rate stats.*/
    /* Unsigned 8bit value */
    /* Unsigned int 8bit value; 0: OFDM, 1:CCK, 2:HT 3:VHT 4..7 reserved */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_PREAMBLE,
    /* Unsigned int 8bit value; 0:1x1, 1:2x2, 3:3x3, 4:4x4 */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_NSS,
    /* Unsigned int 8bit value; 0:20MHz, 1:40Mhz, 2:80Mhz, 3:160Mhz */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BW,
    /* Unsigned int 8bit value; OFDM/CCK rate code would be as per IEEE Std
     * in the units of 0.5mbps HT/VHT it would be mcs index */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MCS_INDEX,

    /* Unsigned 32bit value. Bit rate in units of 100Kbps */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_BIT_RATE,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_* could be
     * nested within the peer info stats.
     */

    /* Unsigned int 32bit value. Number of successfully transmitted data pkts,
     * i.e., with ACK received  *corresponding to the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_TX_MPDU,
    /* Unsigned int 32bit value. Number of received data pkts
     * corresponding to the respective rate. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RX_MPDU,
    /* Unsigned int 32bit value. Number of data pkts losses, i.e.,
     * no ACK received corresponding to *the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_MPDU_LOST,
    /* Unsigned int 32bit value. Total number of data pkt retries for
     *   the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES,
    /* Unsigned int 32bit value. Total number of short data pkt retries for
      the respective rate. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_SHORT,
    /* Unsigned int 32bit value. Total number of long data pkt retries for
     * the respective rate.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RATE_RETRIES_LONG,

    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ID,
    /* Unsigned 32bit value. Total number of msecs the radio is awake
     *  accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * transmitting accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_TX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * in active receive accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_RX_TIME,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * awake due to all scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is
     *   awake due to NAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_NBD,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * awake due to EXTSCAN accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_EXTSCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * awake due to roam scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_ROAM_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * awake due to PNO scan accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_PNO_SCAN,
    /* Unsigned 32bit value. Total number of msecs the radio is
     * awake due to HS2.0 scans and GAS exchange accruing over time.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_ON_TIME_HS20,
    /* Unsigned 32bit value. Number of channels. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RADIO_NUM_CHANNELS,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_
     * could be nested within the channel stats.
     */

    /* Type = enum wifi_channel_width. Channel width, e.g., 20, 40, 80, etc.*/
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_WIDTH,
    /* Unsigned 32bit value. Primary 20MHz channel. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ,
    /* Unsigned 32bit value. Center frequency (MHz) first segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ0,
    /* Unsigned 32bit value. Center frequency (MHz) second segment. */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_INFO_CENTER_FREQ1,

    /* Attributes of type QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_ could be
     * nested within the radio stats.
     */

    /* Unsigned int 32bit value representing total number of msecs the radio
     * s awake on that *channel accruing over time, corresponding to
     * the respective channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_ON_TIME,
    /* Unsigned int 32bit value representing total number of msecs the
     * CCA register is busy accruing  *over time corresponding to the
     * respective channel.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CHANNEL_CCA_BUSY_TIME,


    QCA_WLAN_VENDOR_ATTR_LL_STATS_NUM_RADIOS,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_CH_INFO,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_PEER_INFO_RATE_INFO,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_WMM_INFO,

    /* Unsigned 8bit value. Used by the driver; if set to 1, it indicates that
     * more stats, e.g., peers or radio, are to follow in the next
     * QCA_NL80211_VENDOR_SUBCMD_LL_STATS_*_RESULTS event.
     * Otherwise, it is set to 0.
     */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_RESULTS_MORE_DATA,

    /* Unsigned 64bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_AVERAGE_TSF_OFFSET,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_DETECTED,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_AVG_NUM_FRAMES_LEAKED,

    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_IFACE_LEAKY_AP_GUARD_TIME,

    /* Unsigned 32bit value to indicate ll stats result type */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_TYPE,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LL_STATS_MAX   =
                        QCA_WLAN_VENDOR_ATTR_LL_STATS_AFTER_LAST -1,
};


#endif /* WLAN_FEATURE_LINK_LAYER_STATS */

#ifdef WLAN_FEATURE_EXTSCAN

enum qca_wlan_vendor_attr_extscan_config_params
{
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SUBCMD_CONFIG_PARAM_INVALID = 0,

    /* Unsigned 32-bit value; Middleware provides it to the driver. Middle ware
     * either gets it from caller, e.g., framework, or generates one if
     * framework doesn't provide it.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,

    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_VALID_CHANNELS sub command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,

    /* NL attributes for input params used by
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_START sub command.
     */

    /* Unsigned 32-bit value; channel frequency */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC_CHANNEL,
    /* Unsigned 32-bit value; dwell time in ms. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC_DWELL_TIME,
    /* Unsigned 8-bit value; 0: active; 1: passive; N/A for DFS */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC_PASSIVE,
    /* Unsigned 8-bit value; channel class */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC_CLASS,

    /* Unsigned 8-bit value; bucket index, 0 based */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_INDEX,
    /* Unsigned 8-bit value; band. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_BAND,
    /* Unsigned 32-bit value; desired period, in ms. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_PERIOD,
    /* Unsigned 8-bit value; report events semantics. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_REPORT_EVENTS,
    /* Unsigned 32-bit value.
     * Followed by a nested array of EXTSCAN_CHANNEL_SPEC_* attributes.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,

    /* Array of QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CHANNEL_SPEC,

    /* Unsigned 32-bit value; base timer period in ms. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
    /* Unsigned 32-bit value; number of APs to store in each scan in the
     * BSSID/RSSI history buffer (keep the highest RSSI APs).
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
    /* Unsigned 8-bit value; in %, when scan buffer is this much full, wake up
     * APPS.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_PERCENT,
    /* Unsigned 8-bit value; number of scan bucket specs; followed by a nested
     * array of_EXTSCAN_BUCKET_SPEC_* attributes and values. The size of the
     * array is determined by NUM_BUCKETS.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,

    /* Array of QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC,

    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
    /* Unsigned 32-bit value; maximum number of results to be returned. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,

    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM_BSSID,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM_CHANNEL,

    /* Number of hotlist APs as unsigned 32-bit value, followed by a nested
     * array of AP_THRESHOLD_PARAM attributes and values. The size of the
     * array is determined by NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,

    /* Array of QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_AP_THRESHOLD_PARAM,

    /* Unsigned 32bit value; number of samples for averaging RSSI. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of samples to confirm AP loss. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
    /* Unsigned 32bit value; number of APs breaching threshold. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
    /* Unsigned 32bit value; number of APs. Followed by an array of
     * AP_THRESHOLD_PARAM attributes. Size of the array is NUM_AP.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,
    /* Unsigned 32bit value; number of samples to confirm AP loss. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE,

    /* Unsigned 32-bit value. If max_period is non zero or different than
     * period, then this bucket is an exponential backoff bucket.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_MAX_PERIOD,
    /* Unsigned 32-bit value. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_EXPONENT,
    /* Unsigned 32-bit value. For exponential back off bucket, number of scans
     * performed at a given period and until the exponent is applied.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_BUCKET_SPEC_STEP_COUNT,
    /* Unsigned 8-bit value; in number of scans, wake up AP after these
     * many scans.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_NUM_SCANS,

    /* NL attributes for data used by
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SET_SSID_HOTLIST sub command.
     */
    /* Unsigned 32bit value; number of samples to confirm SSID loss. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE,
    /* Number of hotlist SSIDs as unsigned 32-bit value, followed by a nested
     * array of SSID_THRESHOLD_PARAM_* attributes and values. The size of the
     * array is determined by NUM_SSID.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_HOTLIST_PARAMS_NUM_SSID,
    /* Array of QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM_* attributes.
     * Array size: QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_HOTLIST_PARAMS_NUM_SSID
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM,

    /* An array of 33 x Unsigned 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM_SSID,
    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM_BAND,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM_RSSI_LOW,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SSID_THRESHOLD_PARAM_RSSI_HIGH,

    /* Unsigned 32-bit value; a bitmask w/additional extscan config flag. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CONFIGURATION_FLAGS,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_SUBCMD_CONFIG_PARAM_MAX =
        QCA_WLAN_VENDOR_ATTR_EXTSCAN_SUBCMD_CONFIG_PARAM_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_extscan_results
{
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_INVALID = 0,

    /* Unsigned 32-bit value; must match the request Id supplied by Wi-Fi HAL
      * in the corresponding subcmd NL msg
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_REQUEST_ID,

    /* Unsigned 32-bit value; used to indicate the status response from
      * firmware/driver for the vendor sub-command.
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_STATUS,

    /* EXTSCAN Valid Channels attributes */
    /* Unsigned 32bit value; followed by a nested array of CHANNELS.
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_CHANNELS,
    /* An array of NUM_CHANNELS x Unsigned 32bit value integers representing
      * channel numbers
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CHANNELS,

    /* EXTSCAN Capabilities attributes */
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE,
    /* Signed 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS,
    /* Unsigned 32bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES,

    /* EXTSCAN Attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_RESULTS_AVAILABLE sub-command.
     */

    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_RESULTS_AVAILABLE,


    /* EXTSCAN attributes used with
      * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_FULL_SCAN_RESULT sub-command.
      */

    /* An array of NUM_RESULTS_AVAILABLE x
     * QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_*
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_LIST,

    /* Unsigned 32-bit value; age of sample at the time of retrieval */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_TIME_STAMP,
    /* 33 x unsiged 8-bit value; NULL terminated SSID */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_SSID,
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_BSSID,
    /* Unsigned 32-bit value; channel frequency in MHz */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_CHANNEL,
    /* Signed 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_RSSI,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_RTT,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_RTT_SD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_BEACON_PERIOD,
    /* Unsigned 16-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_CAPABILITY,
    /* Unsigned 32-bit value; size of the IE DATA blob */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_IE_LENGTH,
    /* An array of IE_LENGTH x Unsigned 8-bit value; blob of all the
     * information elements found in the beacon; this data should be a
     * packed list of wifi_information_element objects, one after the other.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_IE_DATA,
    /* Unsigned 8-bit value; set by driver to indicate more scan results are
     * available.
     */
    /* Unsigned 8-bit value; set by driver to indicate more scan results are
      * available.
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_RESULT_MORE_DATA,

    /* Use attr QCA_WLAN_VENDOR_ATTR_EXTSCAN_NUM_RESULTS_AVAILABLE
     * to indicate number of wifi scan results/bssids retrieved by the scan.
     * Also, use QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_LIST to indicate the list
     * of wifi scan results returned for each cached result block.
     */

    /* EXTSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SCAN_EVENT sub-command.
     */
    /* Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_EVENT_TYPE,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SCAN_EVENT_STATUS,

    /* EXTSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_HOTLIST_AP_FOUND sub-command.
     */
    /* Use attr QCA_WLAN_VENDOR_ATTR_EXTSCAN_NUM_RESULTS_AVAILABLE
     * to indicate number of results.
     */

    /* EXTSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_SIGNIFICANT_CHANGE sub-command.
     */
    /* An array of 6 x Unsigned 8-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_BSSID,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_CHANNEL,
    /* Unsigned 32-bit value.
      */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_NUM_RSSI,
    /* A nested array of signed 32-bit RSSI values. Size of the array is
     * determined by (NUM_RSSI of SIGNIFICANT_CHANGE_RESULT_NUM_RSSI.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_SIGNIFICANT_CHANGE_RESULT_RSSI_LIST,

    /* EXTSCAN attributes used with
     * QCA_NL80211_VENDOR_SUBCMD_EXTSCAN_GET_CACHED_RESULTS sub-command.
     */
    /* Use attr QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of gscan cached results returned.
     * Also, use QCA_WLAN_VENDOR_ATTR_EXTSCAN_CACHED_RESULTS_LIST to indicate
     *  the list of gscan cached results.
     */

    /* An array of NUM_RESULTS_AVAILABLE x
     * QCA_NL80211_VENDOR_ATTR_EXTSCAN_CACHED_RESULTS_*
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CACHED_RESULTS_LIST,
    /* Unsigned 32-bit value; a unique identifier for the scan unit. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CACHED_RESULTS_SCAN_ID,
    /* Unsigned 32-bit value; a bitmask w/additional information about scan. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_CACHED_RESULTS_FLAGS,
    /* Use attr QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_RESULTS_AVAILABLE
     * to indicate number of wifi scan results/bssids retrieved by the scan.
     * Also, use QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_LIST to indicate the list
     * of wifi scan results returned for each cached result block.
     */

    /* EXTSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_PNO_NETWORK_FOUND sub-command.
     */
    /* Use QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_RESULTS_AVAILABLE for number
     * of results.
     * Use QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_LIST to indicate the nested
     * list of wifi scan results returned for each wifi_passpoint_match_result block.
     * Array size: QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_NUM_RESULTS_AVAILABLE.
     */

    /* EXTSCAN attributes for
     * QCA_NL80211_VENDOR_SUBCMD_PNO_PASSPOINT_NETWORK_FOUND sub-command.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES,
    /* A nested array of
     * QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_MATCH_*
     * attributes. Array size =
     * *_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_NETWORK_FOUND_NUM_MATCHES.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_MATCH_RESULT_LIST,

    /* Unsigned 32-bit value; network block id for the matched network */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_MATCH_ID,
    /* Use QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_LIST to indicate the nested
     * list of wifi scan results returned for each wifi_passpoint_match_result block.
     */
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP_LEN,
    /* An array size of PASSPOINT_MATCH_ANQP_LEN of unsigned 8-bit values;
     * ANQP data in the information_element format.
     */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_PNO_RESULTS_PASSPOINT_MATCH_ANQP,

    /* Unsigned 32bit value; a EXTSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS,
    /* Unsigned 32bit value; a EXTSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS,
    /* Unsigned 32bit value; a EXTSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID,
    /* Unsigned 32bit value; a EXTSCAN Capabilities attribute. */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID,

    /* Use attr QCA_WLAN_VENDOR_ATTR_EXTSCAN_NUM_RESULTS_AVAILABLE
     * to indicate number of results.
     */

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_MAX =
        QCA_WLAN_VENDOR_ATTR_EXTSCAN_RESULTS_AFTER_LAST - 1,
};

#endif /* WLAN_FEATURE_EXTSCAN */
enum qca_wlan_vendor_attr_set_scanning_mac_oui{
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_MAX =
        QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO sub command.
 */
enum qca_wlan_vendor_attr_get_wifi_info {
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION     = 1,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION   = 2,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_MAX  =
        QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_AFTER_LAST - 1,
};

enum qca_wlan_vendor_attr_get_supported_features {
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_FEATURE_SET_MAX =
        QCA_WLAN_VENDOR_ATTR_FEATURE_SET_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX sub command.
 */
enum qca_wlan_vendor_attr_get_concurrency_matrix {
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_CONFIG_PARAM_SET_SIZE_MAX = 1,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET_SIZE = 2,
    /* An array of SET_SIZE x Unsigned 32bit values representing
     * concurrency combinations.
     */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_RESULTS_SET = 3,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_MAX =
        QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_attr_wifi_logger_start - Enum for wifi logger starting
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_INVALID: Invalid attribute
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_RING_ID: Ring ID
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_VERBOSE_LEVEL: Verbose level
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_FLAGS: Flag
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_AFTER_LAST: Last value
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_MAX: Max value
 */
enum qca_wlan_vendor_attr_wifi_logger_start {
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_RING_ID = 1,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_VERBOSE_LEVEL = 2,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_FLAGS = 3,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_MAX =
       QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_START_AFTER_LAST - 1,
};

/* NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_LINK_PROPERTIES.
 */
enum qca_wlan_vendor_attr_link_properties {
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_INVALID    = 0,
    /* Unsigned 8bit value for specifying nof spatial streams */
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_NSS        = 1,
    /* Unsigned 8bit value for the rate flags */
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_RATE_FLAGS = 2,
    /* Unsigned 32bit value for operating frequency */
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_FREQ       = 3,
    /* Unsigned 32bit value for STA flags*/
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_STA_FLAGS  = 4,
    /*  An array of 6 Unsigned 8bit values for the STA MAC address*/
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_STA_MAC  = 5,

    /* KEEP LAST */
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_MAX =
    QCA_WLAN_VENDOR_ATTR_LINK_PROPERTIES_AFTER_LAST - 1,
};

/**
 * enum qca_wlan_vendor_config: wifi config attr
 *
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_INVALID: invalid config
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_MODULATED_DTIM: dynamic dtim
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_STATS_AVG_FACTOR: stats avg. factor
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_GUARD_TIME: guard time
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_LAST: last config
 * @QCA_WLAN_VENDOR_ATTR_CONFIG_MAX: max config
 */
enum qca_wlan_vendor_config {
    QCA_WLAN_VENDOR_ATTR_CONFIG_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_CONFIG_MODULATED_DTIM,
    QCA_WLAN_VENDOR_ATTR_CONFIG_STATS_AVG_FACTOR,
    QCA_WLAN_VENDOR_ATTR_CONFIG_GUARD_TIME,
    QCA_WLAN_VENDOR_ATTR_CONFIG_FINE_TIME_MEASUREMENT,
    QCA_WLAN_VENDOR_ATTR_CONFIG_TX_RATE,
    QCA_WLAN_VENDOR_ATTR_CONFIG_PENALIZE_AFTER_NCONS_BEACON_MISS,

    /* 8-bit unsigned value to trigger QPower: 1-Enable, 0-Disable */
    QCA_WLAN_VENDOR_ATTR_CONFIG_QPOWER = 25,
    /* 8-bit unsigned value to set the beacon miss threshold in 2.4 GHz */
    QCA_WLAN_VENDOR_ATTR_CONFIG_BEACON_MISS_THRESHOLD_24 = 37,
    /* 8-bit unsigned value to set the beacon miss threshold in 5 GHz */
    QCA_WLAN_VENDOR_ATTR_CONFIG_BEACON_MISS_THRESHOLD_5 = 38,

    /*8-bit unsigned value indicating the driver to use the RSNE as-is from
     * the connect interface. Exclusively used for the scenarios where the
     * device is used as a test bed device with special functionality and
     * not recommended for production. This helps driver to not validate the
     * RSNE passed from user space and thus allow arbitrary IE data to be
     * used for testing purposes.
     * 1-enable, 0-disable.
     * Applications set/reset this configuration. If not reset, this
     * parameter remains in use until the driver is unloaded.
     */
    QCA_WLAN_VENDOR_ATTR_CONFIG_RSN_IE = 39,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_CONFIG_LAST,
    QCA_WLAN_VENDOR_ATTR_CONFIG_MAX =
    QCA_WLAN_VENDOR_ATTR_CONFIG_LAST - 1
};

/**
 * enum qca_wlan_vendor_attr_get_logger_features - value for logger
 *						   supported features
 * @QCA_WLAN_VENDOR_ATTR_LOGGER_INVALID - Invalid
 * @QCA_WLAN_VENDOR_ATTR_LOGGER_SUPPORTED - Indicate the supported features
 * @QCA_WLAN_VENDOR_ATTR_LOGGER_AFTER_LAST - To keep track of the last enum
 * @QCA_WLAN_VENDOR_ATTR_LOGGER_MAX - max value possible for this type
 *
 * enum values are used for NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_GET_LOGGER_FEATURE_SET sub command.
 */
enum qca_wlan_vendor_attr_get_logger_features {
	QCA_WLAN_VENDOR_ATTR_LOGGER_INVALID = 0,
	QCA_WLAN_VENDOR_ATTR_LOGGER_SUPPORTED = 1,

	/* keep last */
	QCA_WLAN_VENDOR_ATTR_LOGGER_AFTER_LAST,
	QCA_WLAN_VENDOR_ATTR_LOGGER_MAX =
		QCA_WLAN_VENDOR_ATTR_LOGGER_AFTER_LAST - 1,
};

/* Feature defines */
#define WIFI_FEATURE_INFRA              0x0001   /* Basic infrastructure mode */
#define WIFI_FEATURE_INFRA_5G           0x0002   /* Support for 5 GHz Band */
#define WIFI_FEATURE_HOTSPOT            0x0004   /* Support for GAS/ANQP */
#define WIFI_FEATURE_P2P                0x0008   /* Wifi-Direct */
#define WIFI_FEATURE_SOFT_AP            0x0010   /* Soft AP */
#define WIFI_FEATURE_EXTSCAN            0x0020   /* Extended Scan APIs */
#define WIFI_FEATURE_NAN                0x0040   /* Neighbor Awareness
                                                    Networking */
#define WIFI_FEATURE_D2D_RTT            0x0080   /* Device-to-device RTT */
#define WIFI_FEATURE_D2AP_RTT           0x0100   /* Device-to-AP RTT */
#define WIFI_FEATURE_BATCH_SCAN         0x0200   /* Batched Scan (legacy) */
#define WIFI_FEATURE_PNO                0x0400   /* Preferred network offload */
#define WIFI_FEATURE_ADDITIONAL_STA     0x0800   /* Support for two STAs */
#define WIFI_FEATURE_TDLS               0x1000   /* Tunnel directed link
                                                    setup */
#define WIFI_FEATURE_TDLS_OFFCHANNEL    0x2000   /* Support for TDLS off
                                                    channel */
#define WIFI_FEATURE_EPR                0x4000   /* Enhanced power reporting */
#define WIFI_FEATURE_AP_STA             0x8000   /* Support for AP STA
                                                    Concurrency */
#define WIFI_FEATURE_LINK_LAYER_STATS   0x10000  /* Link layer stats */

#define WIFI_FEATURE_RTT3               0x20000  /* RTT3 */

/* WIFI CONFIG Parameter defines */
#define WIFI_CONFIG_SET_AVG_STATS_FACTOR 0x0001  /* Average stats factor */
#define WIFI_CONFIG_SET_GUARD_TIME      0x0002  /* Guard Time */
#define WIFI_CONFIG_SET_BCNMISS_PENALTY_COUNT 0x0001 /* BCNMISS_PENALTY_COUNT */

/* Add more features here */
enum qca_wlan_vendor_attr_set_no_dfs_flag
{
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_INVALID = 0,
    /* Unsigned 32-bit value */
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_MAX =
        QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG_AFTER_LAST - 1,
};
/*
 * enum qca_wlan_vendor_attr_wifi_logger_get_ring_data - Get ring data
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_INVALID: Invalid attribute
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_ID: Ring ID
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_AFTER_LAST: Last value
 * @QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_MAX: Max value
 */
enum qca_wlan_vendor_attr_wifi_logger_get_ring_data {
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_ID = 1,
    /* keep last */
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_MAX =
        QCA_WLAN_VENDOR_ATTR_WIFI_LOGGER_GET_RING_DATA_AFTER_LAST - 1,
};


/* Vendor id to be used in vendor specific command and events
 * to user space. Use QCA OUI 00:13:74 to match with define in
 * supplicant code.
 */
#define QCA_NL80211_VENDOR_ID                0x001374

#ifdef FEATURE_WLAN_CH_AVOID
#define HDD_MAX_AVOID_FREQ_RANGES   15
typedef struct sHddAvoidFreqRange
{
   u32 startFreq;
   u32 endFreq;
} tHddAvoidFreqRange;

typedef struct sHddAvoidFreqList
{
   u32 avoidFreqRangeCount;
   tHddAvoidFreqRange avoidFreqRange[HDD_MAX_AVOID_FREQ_RANGES];
} tHddAvoidFreqList;
#endif /* FEATURE_WLAN_CH_AVOID */


/**
 * enum qca_wlan_rssi_monitoring_control - rssi control commands
 * @QCA_WLAN_RSSI_MONITORING_CONTROL_INVALID: invalid
 * @QCA_WLAN_RSSI_MONITORING_START: rssi monitoring start
 * @QCA_WLAN_RSSI_MONITORING_STOP: rssi monitoring stop
 */
enum qca_wlan_rssi_monitoring_control {
        QCA_WLAN_RSSI_MONITORING_CONTROL_INVALID = 0,
        QCA_WLAN_RSSI_MONITORING_START,
        QCA_WLAN_RSSI_MONITORING_STOP,
};

/**
 * enum qca_wlan_vendor_attr_rssi_monitoring - rssi monitoring
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_INVALID: Invalid
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CONTROL: control
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX_RSSI: max rssi
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MIN_RSSI: min rssi
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_BSSID: current bssid
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI: current rssi
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_AFTER_LAST: after last
 * @QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX: max
 */
enum qca_wlan_vendor_attr_rssi_monitoring {
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_INVALID = 0,

        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CONTROL,
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_REQUEST_ID,

        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX_RSSI,
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MIN_RSSI,

        /* attributes to be used/received in callback */
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_BSSID,
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_CUR_RSSI,

        /* keep last */
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_AFTER_LAST,
        QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_MAX =
                QCA_WLAN_VENDOR_ATTR_RSSI_MONITORING_AFTER_LAST - 1,
};

#ifdef WLAN_FEATURE_OFFLOAD_PACKETS
/**
 * enum wlan_offloaded_packets_control - control commands
 * @WLAN_START_OFFLOADED_PACKETS: start offloaded packets
 * @WLAN_STOP_OFFLOADED_PACKETS: stop offloaded packets
 *
 */
enum wlan_offloaded_packets_control
{
    WLAN_START_OFFLOADED_PACKETS = 1,
    WLAN_STOP_OFFLOADED_PACKETS  = 2
};

/**
 * enum qca_wlan_vendor_attr_offloaded_packets - offloaded packets
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_INVALID: invalid
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SENDING_CONTROL: control
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_REQUEST_ID: request id
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA: ip packet data
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR: src mac address
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR: destination mac address
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_PERIOD: period in milli seconds
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_AFTER_LAST: after last
 * @QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_MAX: max
 */
enum qca_wlan_vendor_attr_offloaded_packets
{
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SENDING_CONTROL,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_REQUEST_ID,

    /* Packet in hex format */
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_PERIOD,

    /* keep last */
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_MAX =
    QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_AFTER_LAST - 1,
};
#endif

/**
 * enum wifi_logger_supported_features - values for supported logger features
 * @WIFI_LOGGER_PER_PACKET_TX_RX_STATUS_SUPPORTED - Per packet statistics
 * @WIFI_LOGGER_CONNECT_EVENT_SUPPORTED - Logging of Connectivity events
 * @WIFI_LOGGER_POWER_EVENT_SUPPORTED - Power of driver
 * @WIFI_LOGGER_WAKE_LOCK_SUPPORTED - Wakelock of driver
 * @WIFI_LOGGER_WATCHDOG_TIMER_SUPPORTED - monitor FW health
 */
enum wifi_logger_supported_features {
    WIFI_LOGGER_MEMORY_DUMP_SUPPORTED = (1 << (0)),
    WIFI_LOGGER_PER_PACKET_TX_RX_STATUS_SUPPORTED = (1 << (1)),
    WIFI_LOGGER_CONNECT_EVENT_SUPPORTED = (1 << (2)),
    WIFI_LOGGER_POWER_EVENT_SUPPORTED = (1 << (3)),
    WIFI_LOGGER_WAKE_LOCK_SUPPORTED = (1 << (4)),
    WIFI_LOGGER_VERBOSE_SUPPORTED = (1 << (5)),
    WIFI_LOGGER_WATCHDOG_TIMER_SUPPORTED = (1 << (6)),
};

struct cfg80211_bss* wlan_hdd_cfg80211_update_bss_db( hdd_adapter_t *pAdapter,
                                      tCsrRoamInfo *pRoamInfo
                                      );

#ifdef FEATURE_WLAN_LFR
int wlan_hdd_cfg80211_pmksa_candidate_notify(
                    hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                    int index, bool preauth );
#endif

#ifdef FEATURE_WLAN_LFR_METRICS
VOS_STATUS wlan_hdd_cfg80211_roam_metrics_preauth(hdd_adapter_t *pAdapter,
                                                  tCsrRoamInfo *pRoamInfo);

VOS_STATUS wlan_hdd_cfg80211_roam_metrics_preauth_status(
    hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo, bool preauth_status);

VOS_STATUS wlan_hdd_cfg80211_roam_metrics_handover(hdd_adapter_t *pAdapter,
                                                   tCsrRoamInfo *pRoamInfo);
#endif

#ifdef FEATURE_WLAN_WAPI
void wlan_hdd_cfg80211_set_key_wapi(hdd_adapter_t* pAdapter,
              u8 key_index, const u8 *mac_addr, const u8 *key , int key_Len);
#endif
struct wiphy *wlan_hdd_cfg80211_wiphy_alloc(int priv_size);

int wlan_hdd_cfg80211_scan( struct wiphy *wiphy,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
                            struct net_device *dev,
#endif
                            struct cfg80211_scan_request *request);

int wlan_hdd_cfg80211_update_band(struct wiphy *wiphy, eCsrBand eBand);

int wlan_hdd_cfg80211_init(struct device *dev,
                               struct wiphy *wiphy,
                               hdd_config_t *pCfg
                                         );

int wlan_hdd_cfg80211_register( struct wiphy *wiphy);
void wlan_hdd_cfg80211_register_frames(hdd_adapter_t* pAdapter);

void wlan_hdd_cfg80211_deregister_frames(hdd_adapter_t* pAdapter);
#ifdef CONFIG_ENABLE_LINUX_REG
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
void wlan_hdd_linux_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#else
int wlan_hdd_linux_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#endif
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
void wlan_hdd_crda_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#else
int wlan_hdd_crda_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request);
#endif
#endif

extern v_VOID_t hdd_connSetConnectionState( hdd_station_ctx_t *pHddStaCtx,
                                        eConnectionState connState );
VOS_STATUS wlan_hdd_validate_operation_channel(hdd_adapter_t *pAdapter,int channel);
#ifdef FEATURE_WLAN_TDLS
int wlan_hdd_cfg80211_send_tdls_discover_req(struct wiphy *wiphy,
                            struct net_device *dev, u8 *peer);
#endif
#ifdef WLAN_FEATURE_GTK_OFFLOAD
extern void wlan_hdd_cfg80211_update_replayCounterCallback(void *callbackContext,
                            tpSirGtkOffloadGetInfoRspParams pGtkOffloadGetInfoRsp);
#endif
void* wlan_hdd_change_country_code_cb(void *pAdapter);
void hdd_select_cbmode( hdd_adapter_t *pAdapter,v_U8_t operationChannel);

/*
 * wlan_hdd_restore_channels() Restore the channels which were cached
 * and disabled in __wlan_hdd_disable_channels api.
 * @hdd_ctx: Pointer to the HDD context
 *
 * @Return: 0 on success, Error code on failure
 */
int wlan_hdd_restore_channels(hdd_context_t *pHddCtx);

/*
 * hdd_update_indoor_channel() - enable/disable indoor channel
 * @hdd_ctx: hdd context
 * @disable: whether to enable / disable indoor channel
 *
 * enable/disable indoor channel in wiphy/cds
 *
 * Return: void
 */
void hdd_update_indoor_channel(hdd_context_t *hdd_ctx,
    bool disable);

/*
 * hdd_modify_indoor_channel_state_flags() - modify wiphy flags and cds state
 * @wiphy_chan: wiphy channel number
 * @rfChannel: channel hw value
 * @disable: Disable/enable the flags
 *
 * Modify wiphy flags and cds state if channel is indoor.
 *
 * Return: void
 */
void hdd_modify_indoor_channel_state_flags(
    struct ieee80211_channel *wiphy_chan,
    v_U32_t rfChannel, bool disable, hdd_context_t *hdd_ctx);


v_U8_t* wlan_hdd_cfg80211_get_ie_ptr(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
                                     const v_U8_t *pIes,
#else
                                     v_U8_t *pIes,
#endif
                                     int length, v_U8_t eid);

#ifdef FEATURE_WLAN_CH_AVOID
int wlan_hdd_send_avoid_freq_event(hdd_context_t *pHddCtx,
                                   tHddAvoidFreqList *pAvoidFreqList);
#endif /* FEATURE_WLAN_CH_AVOID */

#ifdef WLAN_FEATURE_EXTSCAN
void wlan_hdd_cfg80211_extscan_callback(void *ctx, const tANI_U16 evType,
                                      void *pMsg);
#endif /* WLAN_FEATURE_EXTSCAN */

void hdd_rssi_threshold_breached_cb(void *hddctx,
                                 struct rssi_breach_event *data);

void wlan_hdd_cfg80211_nan_init(hdd_context_t *pHddCtx);

#ifdef FEATURE_OEM_DATA_SUPPORT
void wlan_hdd_cfg80211_oemdata_callback(void *ctx, const tANI_U16 evType,
                                      void *pMsg,  tANI_U32 evLen);
#endif /* FEATURE_OEM_DATA_SUPPORT */

#if !(defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) && \
	(LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0)) && \
	!(defined(WITH_BACKPORTS))
static inline struct sk_buff *
backported_cfg80211_vendor_event_alloc(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
    struct wireless_dev *wdev,
#endif
    int approxlen,
    int event_idx, gfp_t gfp)
{
    return cfg80211_vendor_event_alloc(wiphy, approxlen, event_idx, gfp);
}
#define cfg80211_vendor_event_alloc backported_cfg80211_vendor_event_alloc
#endif

/**
 * wlan_hdd_send_hang_reason_event() - Send hang reason to the userspace
 * @hdd_ctx: Pointer to hdd context
 * @reason: cds recovery reason
 *
 * Return: 0 on success or failure reason
 */
int wlan_hdd_send_hang_reason_event(hdd_context_t *hdd_ctx,
				    unsigned int reason);

/**
 * enum qca_wlan_vendor_attr_memory_dump - values for memory dump attributes
 * @QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_INVALID - Invalid
 * @QCA_WLAN_VENDOR_ATTR_REQUEST_ID - Indicate request ID
 * @QCA_WLAN_VENDOR_ATTR_MEMDUMP_SIZE - Indicate size of the memory dump
 * @QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_AFTER_LAST - To keep track of the last enum
 * @QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_MAX - max value possible for this type
 *
 * enum values are used for NL attributes for data used by
 * QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP sub command.
 */
enum qca_wlan_vendor_attr_memory_dump {
    QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_INVALID = 0,
    QCA_WLAN_VENDOR_ATTR_REQUEST_ID = 1,
    QCA_WLAN_VENDOR_ATTR_MEMDUMP_SIZE = 2,

    QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_AFTER_LAST,
    QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_MAX =
    QCA_WLAN_VENDOR_ATTR_MEMORY_DUMP_AFTER_LAST - 1,
};

#if defined(CFG80211_DISCONNECTED_V2) || \
(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0))
static inline void wlan_hdd_cfg80211_indicate_disconnect(struct net_device *dev,
                                                         bool locally_generated,
                                                         int reason)
{
    cfg80211_disconnected(dev, reason, NULL, 0,
                          locally_generated, GFP_KERNEL);
}
#else
static inline void wlan_hdd_cfg80211_indicate_disconnect(struct net_device *dev,
                                                         bool locally_generated,
                                                         int reason)
{
    cfg80211_disconnected(dev, reason, NULL, 0,
                          GFP_KERNEL);
}
#endif

struct cfg80211_bss* wlan_hdd_cfg80211_update_bss_list(
   hdd_adapter_t *pAdapter, tSirMacAddr bssid);

struct cfg80211_bss *wlan_hdd_cfg80211_inform_bss_frame(hdd_adapter_t *pAdapter,
		tSirBssDescription *bss_desc);
#ifdef USE_CFG80211_DEL_STA_V2
int wlan_hdd_cfg80211_del_station(struct wiphy *wiphy,
                                  struct net_device *dev,
                                  struct station_del_parameters *param);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,16,0)) || defined(WITH_BACKPORTS)
int wlan_hdd_cfg80211_del_station(struct wiphy *wiphy,
                                  struct net_device *dev, const u8 *mac);
#else
int wlan_hdd_cfg80211_del_station(struct wiphy *wiphy,
                                  struct net_device *dev, u8 *mac);
#endif
#endif

int wlan_hdd_cfg80211_update_apies(hdd_adapter_t *pHostapdAdapter);
int wlan_hdd_try_disconnect(hdd_adapter_t *pAdapter);
void wlan_hdd_sap_get_sta_rssi(hdd_adapter_t *adapter, uint8_t staid, s8 *rssi);

/*
 *wlan_hdd_send_sta_authorized_event: Function to send station authorized
 *event to user space in case of SAP
 *@pAdapter: Pointer to the adapter
 *@pHddCtx:  HDD Context
 *@mac_addr: MAC address of the STA for whic the Authorized event needs to
 *           be sent
 *This api is used to send station authorized event to user space
 */
VOS_STATUS wlan_hdd_send_sta_authorized_event(hdd_adapter_t *adapter,
                                              hdd_context_t *hdd_ctx,
                                              const v_MACADDR_t *mac_addr);

/**
 * wlan_hdd_disconnect() - hdd disconnect api
 * @pAdapter: Pointer to adapter
 * @reason: Disconnect reason code
 *
 * This function is used to issue a disconnect request to SME
 *
 * Return: 0 for success, non-zero for failure
 */
int wlan_hdd_disconnect(hdd_adapter_t *pAdapter, u16 reason);

#if defined(CFG80211_SCAN_RANDOM_MAC_ADDR) || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 4, 0))
/**
 * wlan_hdd_cfg80211_scan_randomization_init() - Enable NL80211 scan randomize
 * @wiphy: Pointer to wiphy structure
 *
 * This function is used to enable NL80211 scan randomization feature when
 * ini: gEnableMacAddrSpoof is set to MAC_ADDR_SPOOFING_FW_HOST_ENABLE and
 * cfg80211 supports scan randomization.
 *
 * Return: None
 */
void wlan_hdd_cfg80211_scan_randomization_init(struct wiphy *wiphy);
#else
static inline
void wlan_hdd_cfg80211_scan_randomization_init(struct wiphy *wiphy)
{
}
#endif

#endif
