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

/**========================================================================

  \file  wlan_hdd_assoc.c
  \brief WLAN Host Device Driver implementation

  ========================================================================*/
/**=========================================================================
                       EDIT HISTORY FOR FILE


  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


  $Header:$   $DateTime: $ $Author: $


  when        who    what, where, why
  --------    ---    --------------------------------------------------------
  05/06/09     Shailender     Created module.
  ==========================================================================*/

#include "wlan_hdd_includes.h"
#include <aniGlobal.h>
#include "dot11f.h"
#include "wlan_nlink_common.h"
#include "wlan_btc_svc.h"
#include "wlan_hdd_power.h"
#include "wlan_hdd_trace.h"
#include <linux/ieee80211.h>
#include <linux/wireless.h>
#include <net/cfg80211.h>
#include "wlan_hdd_cfg80211.h"
#include "csrInsideApi.h"
#include "wlan_hdd_p2p.h"
#include <vos_sched.h>
#include "wlan_hdd_tdls.h"
#include "sme_Api.h"
#include "wlan_hdd_hostapd.h"
#include "vos_utils.h"
#include <wlan_hdd_wext.h>
#include "sapInternal.h"

#if defined CFG80211_ROAMED_API_UNIFIED || \
	(LINUX_VERSION_CODE >= KERNEL_VERSION(4, 12, 0))
/**
 * hdd_send_roamed_ind() - send roamed indication to cfg80211
 * @dev: network device
 * @bss: cfg80211 roamed bss pointer
 * @req_ie: IEs used in reassociation request
 * @req_ie_len: Length of the @req_ie
 * @resp_ie: IEs received in successful reassociation response
 * @resp_ie_len: Length of @resp_ie
 *
 * Return: none
 */
static void hdd_send_roamed_ind(struct net_device *dev,
				struct cfg80211_bss *bss, const uint8_t *req_ie,
				size_t req_ie_len, const uint8_t *resp_ie,
				size_t resp_ie_len)
{
	struct cfg80211_roam_info info = {0};

	info.bss = bss;
	info.req_ie = req_ie;
	info.req_ie_len = req_ie_len;
	info.resp_ie = resp_ie;
	info.resp_ie_len = resp_ie_len;

	cfg80211_roamed(dev, &info, GFP_KERNEL);
}
#else
/**
 * hdd_send_roamed_ind() - send roamed indication to cfg80211
 * @dev: network device
 * @bss: cfg80211 roamed bss pointer
 * @req_ie: IEs used in reassociation request
 * @req_ie_len: Length of the @req_ie
 * @resp_ie: IEs received in successful reassociation response
 * @resp_ie_len: Length of @resp_ie
 *
 * Return: none
 */
static inline void hdd_send_roamed_ind(struct net_device *dev,
				       struct cfg80211_bss *bss,
				       const uint8_t *req_ie, size_t req_ie_len,
				       const uint8_t *resp_ie,
				       size_t resp_ie_len)
{
	cfg80211_roamed_bss(dev, bss, req_ie, req_ie_len, resp_ie, resp_ie_len,
			    GFP_KERNEL);
}
#endif

v_BOOL_t mibIsDot11DesiredBssTypeInfrastructure( hdd_adapter_t *pAdapter );

struct ether_addr
{
    u_char  ether_addr_octet[6];
};
// These are needed to recognize WPA and RSN suite types
#define HDD_WPA_OUI_SIZE 4
v_U8_t ccpWpaOui00[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x00 };
v_U8_t ccpWpaOui01[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x01 };
v_U8_t ccpWpaOui02[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x02 };
v_U8_t ccpWpaOui03[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x03 };
v_U8_t ccpWpaOui04[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x04 };
v_U8_t ccpWpaOui05[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x50, 0xf2, 0x05 };
#ifdef FEATURE_WLAN_ESE
v_U8_t ccpWpaOui06[ HDD_WPA_OUI_SIZE ] = { 0x00, 0x40, 0x96, 0x00 }; // CCKM
#endif /* FEATURE_WLAN_ESE */
#define HDD_RSN_OUI_SIZE 4
v_U8_t ccpRSNOui00[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x00 }; // group cipher
v_U8_t ccpRSNOui01[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x01 }; // WEP-40 or RSN
v_U8_t ccpRSNOui02[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x02 }; // TKIP or RSN-PSK
v_U8_t ccpRSNOui03[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x03 }; // Reserved
v_U8_t ccpRSNOui04[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x04 }; // AES-CCMP
v_U8_t ccpRSNOui05[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x05 }; // WEP-104
#ifdef FEATURE_WLAN_ESE
v_U8_t ccpRSNOui06[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x40, 0x96, 0x00 }; // CCKM
#endif /* FEATURE_WLAN_ESE */
#ifdef WLAN_FEATURE_11W
v_U8_t ccpRSNOui07[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x06 }; // RSN-PSK-SHA256
/* RSN-8021X-SHA256 */
v_U8_t ccpRSNOui08[ HDD_RSN_OUI_SIZE ] = { 0x00, 0x0F, 0xAC, 0x05 };
#endif

#if defined(WLAN_FEATURE_VOWIFI_11R)
// Offset where the EID-Len-IE, start.
#define FT_ASSOC_RSP_IES_OFFSET 6 /* Capability(2) + AID(2) + Status Code(2)*/
#define FT_ASSOC_REQ_IES_OFFSET 4 /* Capability(2) + LI(2) */
#endif

#define BEACON_FRAME_IES_OFFSET 12

/* The time after add bss, in which SAP should start ECSA to move to SCC */
#define ECSA_SCC_CHAN_CHANGE_DEFER_INTERVAL 1500
/*
 * Time in ms after disconnect, in which the SAP should move to non DFS channel.
 * This will avoid multiple SAP channel switch if disconnet is followed by
 * connect.
 */
#define ECSA_DFS_CHAN_CHANGE_DEFER_TIME 200


#ifdef WLAN_FEATURE_PACKET_FILTERING
extern int wlan_hdd_update_v6_filters(hdd_adapter_t *pAdapter, v_U8_t set); // IKJB42MAIN-1244, Motorola, a19091
#endif

#ifdef WLAN_FEATURE_11W
void hdd_indicateUnprotMgmtFrame(hdd_adapter_t *pAdapter,
                            tANI_U32 nFrameLength,
                            tANI_U8* pbFrames,
                            tANI_U8 frameType );
#endif

#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
static void hdd_indicateTsmIe(hdd_adapter_t *pAdapter, tANI_U8 tid,
                            tANI_U8  state,
                            tANI_U16 measInterval );
static void hdd_indicateCckmPreAuth(hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo);
static void hdd_indicateEseAdjApRepInd(hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo);
static void hdd_indicateEseBcnReportInd(const hdd_adapter_t *pAdapter, const tCsrRoamInfo *pRoamInfo);

#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */

static eHalStatus hdd_RoamSetKeyCompleteHandler( hdd_adapter_t *pAdapter,
                                                tCsrRoamInfo *pRoamInfo,
                                                tANI_U32 roamId,
                                                eRoamCmdStatus roamStatus,
                                                eCsrRoamResult roamResult );

v_VOID_t hdd_connSetConnectionState( hdd_station_ctx_t *pHddStaCtx,
                                        eConnectionState connState )
{
   // save the new connection state
   hddLog(LOG1, FL("ConnectionState Changed from oldState:%d to State:%d"),
                    pHddStaCtx->conn_info.connState,connState);
   pHddStaCtx->conn_info.connState = connState;
}

// returns FALSE if not connected.
// returns TRUE for the two 'connected' states (Infra Associated or IBSS Connected ).
// returns the connection state.  Can specify NULL if you dont' want to get the actual state.

static inline v_BOOL_t hdd_connGetConnectionState( hdd_station_ctx_t *pHddStaCtx,
                                    eConnectionState *pConnState )
{
   v_BOOL_t fConnected;
   eConnectionState connState;

   // get the connection state.
   connState = pHddStaCtx->conn_info.connState;
   // Set the fConnected return variable based on the Connected State.
   if ( eConnectionState_Associated == connState ||
        eConnectionState_IbssConnected == connState ||
        eConnectionState_IbssDisconnected == connState)
   {
      fConnected = VOS_TRUE;
   }
   else
   {
      fConnected = VOS_FALSE;
   }

   if ( pConnState )
   {
      *pConnState = connState;
   }

   return( fConnected );
}

v_BOOL_t hdd_connIsConnected( hdd_station_ctx_t *pHddStaCtx )
{
   return( hdd_connGetConnectionState( pHddStaCtx, NULL ) );
}

eCsrBand hdd_connGetConnectedBand( hdd_station_ctx_t *pHddStaCtx )
{
   v_U8_t staChannel = 0;

   if ( eConnectionState_Associated == pHddStaCtx->conn_info.connState )
   {
       staChannel = pHddStaCtx->conn_info.operationChannel;
   }

   if ( staChannel > 0 && staChannel < 14 )
       return eCSR_BAND_24;
   else if (staChannel >= 36 && staChannel <= 165 )
      return eCSR_BAND_5G;
   else  /* If station is not connected return as eCSR_BAND_ALL */
      return eCSR_BAND_ALL;
}


//TODO - Not used anyhwere. Can be removed.
#if 0
//
v_BOOL_t hdd_connIsConnectedInfra( hdd_adapter_t *pAdapter )
{
   v_BOOL_t fConnectedInfra = FALSE;
   eConnectionState connState;

   if ( hdd_connGetConnectionState( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter), &connState ) )
   {
      if ( eConnectionState_Associated == connState )
      {
         fConnectedInfra = TRUE;
      }
   }

   return( fConnectedInfra );
}
#endif

static inline v_BOOL_t hdd_connGetConnectedCipherAlgo( hdd_station_ctx_t *pHddStaCtx, eCsrEncryptionType *pConnectedCipherAlgo )
{
    v_BOOL_t fConnected = VOS_FALSE;

    fConnected = hdd_connGetConnectionState( pHddStaCtx, NULL );

    if ( pConnectedCipherAlgo )
    {
        *pConnectedCipherAlgo = pHddStaCtx->conn_info.ucEncryptionType;
    }

    return( fConnected );
}

inline v_BOOL_t hdd_connGetConnectedBssType( hdd_station_ctx_t *pHddStaCtx, eMib_dot11DesiredBssType *pConnectedBssType )
{
    v_BOOL_t fConnected = VOS_FALSE;

    fConnected = hdd_connGetConnectionState( pHddStaCtx, NULL );

    if ( pConnectedBssType )
    {
        *pConnectedBssType = pHddStaCtx->conn_info.connDot11DesiredBssType;
    }

    return( fConnected );
}

static inline void hdd_connSaveConnectedBssType( hdd_station_ctx_t *pHddStaCtx, eCsrRoamBssType csrRoamBssType )
{
   switch( csrRoamBssType )
   {
      case eCSR_BSS_TYPE_INFRASTRUCTURE:
          pHddStaCtx->conn_info.connDot11DesiredBssType = eMib_dot11DesiredBssType_infrastructure;
         break;

      case eCSR_BSS_TYPE_IBSS:
      case eCSR_BSS_TYPE_START_IBSS:
          pHddStaCtx->conn_info.connDot11DesiredBssType = eMib_dot11DesiredBssType_independent;
         break;

      /** We will never set the BssType to 'any' when attempting a connection
            so CSR should never send this back to us.*/
      case eCSR_BSS_TYPE_ANY:
      default:
         VOS_ASSERT( 0 );
         break;
   }

}

/**
 * hdd_copy_ht_caps()- Populate kernel HT caps structure object
 * @hdd_ht_cap: HT capabilities of kernel type
 * @roam_ht_cap: HT capabilities maintained locally within driver
 *
 * Return: None
 */
void hdd_copy_ht_caps(struct ieee80211_ht_cap *hdd_ht_cap,
                      tDot11fIEHTCaps *roam_ht_cap)
{
    uint32_t i, temp_ht_cap;

    vos_mem_zero(hdd_ht_cap, sizeof(struct ieee80211_ht_cap));

    if (roam_ht_cap->advCodingCap)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_LDPC_CODING;
    if (roam_ht_cap->supportedChannelWidthSet)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_SUP_WIDTH_20_40;
    temp_ht_cap = roam_ht_cap->mimoPowerSave &
        (IEEE80211_HT_CAP_SM_PS >> IEEE80211_HT_CAP_SM_PS_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->cap_info |=
            temp_ht_cap << IEEE80211_HT_CAP_SM_PS_SHIFT;
    if (roam_ht_cap->greenField)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_GRN_FLD;
    if (roam_ht_cap->shortGI20MHz)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_SGI_20;
    if (roam_ht_cap->shortGI40MHz)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_SGI_40;
    if (roam_ht_cap->txSTBC)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_TX_STBC;
    temp_ht_cap = roam_ht_cap->rxSTBC & (IEEE80211_HT_CAP_RX_STBC >>
        IEEE80211_HT_CAP_RX_STBC_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->cap_info |=
            temp_ht_cap << IEEE80211_HT_CAP_RX_STBC_SHIFT;
    if (roam_ht_cap->delayedBA)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_DELAY_BA;
    if (roam_ht_cap->maximalAMSDUsize)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_MAX_AMSDU;
    if (roam_ht_cap->dsssCckMode40MHz)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_DSSSCCK40;
    if (roam_ht_cap->psmp)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_RESERVED;
    if (roam_ht_cap->stbcControlFrame)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_40MHZ_INTOLERANT;
    if (roam_ht_cap->lsigTXOPProtection)
        hdd_ht_cap->cap_info |= IEEE80211_HT_CAP_LSIG_TXOP_PROT;

    /* 802.11n HT capability AMPDU settings (for ampdu_params_info) */
    if (roam_ht_cap->maxRxAMPDUFactor)
        hdd_ht_cap->ampdu_params_info |=
            IEEE80211_HT_AMPDU_PARM_FACTOR;
    temp_ht_cap = roam_ht_cap->mpduDensity &
        (IEEE80211_HT_AMPDU_PARM_DENSITY >>
         IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->ampdu_params_info |=
        temp_ht_cap << IEEE80211_HT_AMPDU_PARM_DENSITY_SHIFT;

    /* 802.11n HT extended capabilities masks */
    if (roam_ht_cap->pco)
        hdd_ht_cap->extended_ht_cap_info |=
            IEEE80211_HT_EXT_CAP_PCO;
    temp_ht_cap = roam_ht_cap->transitionTime &
        (IEEE80211_HT_EXT_CAP_PCO_TIME >>
        IEEE80211_HT_EXT_CAP_PCO_TIME_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->extended_ht_cap_info |=
            temp_ht_cap << IEEE80211_HT_EXT_CAP_PCO_TIME_SHIFT;
    temp_ht_cap = roam_ht_cap->mcsFeedback &
        (IEEE80211_HT_EXT_CAP_MCS_FB >> IEEE80211_HT_EXT_CAP_MCS_FB_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->extended_ht_cap_info |=
            temp_ht_cap << IEEE80211_HT_EXT_CAP_MCS_FB_SHIFT;

    /* tx_bf_cap_info capabilities */
    if (roam_ht_cap->txBF)
        hdd_ht_cap->tx_BF_cap_info |= TX_BF_CAP_INFO_TX_BF;
    if (roam_ht_cap->rxStaggeredSounding)
        hdd_ht_cap->tx_BF_cap_info |=
            TX_BF_CAP_INFO_RX_STAG_RED_SOUNDING;
    if (roam_ht_cap->txStaggeredSounding)
        hdd_ht_cap->tx_BF_cap_info |=
            TX_BF_CAP_INFO_TX_STAG_RED_SOUNDING;
    if (roam_ht_cap->rxZLF)
        hdd_ht_cap->tx_BF_cap_info |= TX_BF_CAP_INFO_RX_ZFL;
    if (roam_ht_cap->txZLF)
        hdd_ht_cap->tx_BF_cap_info |= TX_BF_CAP_INFO_TX_ZFL;
    if (roam_ht_cap->implicitTxBF)
        hdd_ht_cap->tx_BF_cap_info |= TX_BF_CAP_INFO_IMP_TX_BF;
    temp_ht_cap = roam_ht_cap->calibration &
        (TX_BF_CAP_INFO_CALIBRATION >> TX_BF_CAP_INFO_CALIBRATION_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap << TX_BF_CAP_INFO_CALIBRATION_SHIFT;
    if (roam_ht_cap->explicitCSITxBF)
        hdd_ht_cap->tx_BF_cap_info |= TX_BF_CAP_INFO_EXP_CSIT_BF;
    if (roam_ht_cap->explicitUncompressedSteeringMatrix)
        hdd_ht_cap->tx_BF_cap_info |=
            TX_BF_CAP_INFO_EXP_UNCOMP_STEER_MAT;
    temp_ht_cap = roam_ht_cap->explicitBFCSIFeedback &
        (TX_BF_CAP_INFO_EXP_BF_CSI_FB >>
         TX_BF_CAP_INFO_EXP_BF_CSI_FB_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap << TX_BF_CAP_INFO_EXP_BF_CSI_FB_SHIFT;
    temp_ht_cap =
        roam_ht_cap->explicitUncompressedSteeringMatrixFeedback &
        (TX_BF_CAP_INFO_EXP_UNCMP_STEER_MAT >>
         TX_BF_CAP_INFO_EXP_UNCMP_STEER_MAT_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap <<
            TX_BF_CAP_INFO_EXP_UNCMP_STEER_MAT_SHIFT;
    temp_ht_cap =
        roam_ht_cap->explicitCompressedSteeringMatrixFeedback &
        (TX_BF_CAP_INFO_EXP_CMP_STEER_MAT_FB >>
         TX_BF_CAP_INFO_EXP_CMP_STEER_MAT_FB_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap <<
                TX_BF_CAP_INFO_EXP_CMP_STEER_MAT_FB_SHIFT;
    temp_ht_cap = roam_ht_cap->csiNumBFAntennae &
        (TX_BF_CAP_INFO_CSI_NUM_BF_ANT >>
         TX_BF_CAP_INFO_CSI_NUM_BF_ANT_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap << TX_BF_CAP_INFO_CSI_NUM_BF_ANT_SHIFT;
    temp_ht_cap = roam_ht_cap->uncompressedSteeringMatrixBFAntennae &
        (TX_BF_CAP_INFO_UNCOMP_STEER_MAT_BF_ANT >>
         TX_BF_CAP_INFO_UNCOMP_STEER_MAT_BF_ANT_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap <<
               TX_BF_CAP_INFO_UNCOMP_STEER_MAT_BF_ANT_SHIFT;
    temp_ht_cap = roam_ht_cap->compressedSteeringMatrixBFAntennae &
        (TX_BF_CAP_INFO_COMP_STEER_MAT_BF_ANT >>
         TX_BF_CAP_INFO_COMP_STEER_MAT_BF_ANT_SHIFT);
    if (temp_ht_cap)
        hdd_ht_cap->tx_BF_cap_info |=
            temp_ht_cap <<
                TX_BF_CAP_INFO_COMP_STEER_MAT_BF_ANT_SHIFT;

    /* antenna selection */
    if (roam_ht_cap->antennaSelection)
        hdd_ht_cap->antenna_selection_info |= ANTENNA_SEL_INFO;
    if (roam_ht_cap->explicitCSIFeedbackTx)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_EXP_CSI_FB_TX;
    if (roam_ht_cap->antennaIndicesFeedbackTx)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_ANT_ID_FB_TX;
    if (roam_ht_cap->explicitCSIFeedback)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_EXP_CSI_FB;
    if (roam_ht_cap->antennaIndicesFeedback)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_ANT_ID_FB;
    if (roam_ht_cap->rxAS)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_RX_AS;
    if (roam_ht_cap->txSoundingPPDUs)
        hdd_ht_cap->antenna_selection_info |=
            ANTENNA_SEL_INFO_TX_SOUNDING_PPDU;

    /* mcs data rate */
    for (i = 0; i < IEEE80211_HT_MCS_MASK_LEN; ++i)
        hdd_ht_cap->mcs.rx_mask[i] =
            roam_ht_cap->supportedMCSSet[i];

        hdd_ht_cap->mcs.rx_highest =
            ((short) (roam_ht_cap->supportedMCSSet[11]) << 8) |
            ((short) (roam_ht_cap->supportedMCSSet[10]));
        hdd_ht_cap->mcs.tx_params =
            roam_ht_cap->supportedMCSSet[12];

}


#define VHT_CAP_MAX_MPDU_LENGTH_MASK		0x00000003
#define VHT_CAP_SUPP_CHAN_WIDTH_MASK_SHIFT		 2
#define VHT_CAP_RXSTBC_MASK_SHIFT			 8
#define VHT_CAP_BEAMFORMEE_STS_SHIFT			13
#define VHT_CAP_BEAMFORMEE_STS_MASK \
    (0x0000e000 >> VHT_CAP_BEAMFORMEE_STS_SHIFT)
#define VHT_CAP_SOUNDING_DIMENSIONS_SHIFT		16
#define VHT_CAP_SOUNDING_DIMENSIONS_MASK \
    (0x00070000 >> VHT_CAP_SOUNDING_DIMENSIONS_SHIFT)
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK_SHIFT	23
#define VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK \
    (0x03800000 >> VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK_SHIFT)
#define VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB_SHIFT	26

/**
 * hdd_copy_vht_caps()- Populate kernel VHT caps structure object
 * @hdd_ht_cap: VHT capabilities of kernel type
 * @roam_ht_cap: VHT capabilities maintained locally within driver
 *
 * Return: None
 */
void hdd_copy_vht_caps(struct ieee80211_vht_cap *hdd_vht_cap,
			      tDot11fIEVHTCaps *roam_vht_cap)
{
    uint32_t temp_vht_cap;

    vos_mem_zero(hdd_vht_cap, sizeof(struct ieee80211_vht_cap));

    temp_vht_cap = roam_vht_cap->maxMPDULen & VHT_CAP_MAX_MPDU_LENGTH_MASK;
    hdd_vht_cap->vht_cap_info |= temp_vht_cap;
    temp_vht_cap = roam_vht_cap->supportedChannelWidthSet &
        (IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_MASK >>
            VHT_CAP_SUPP_CHAN_WIDTH_MASK_SHIFT);
    if (temp_vht_cap)
        if (roam_vht_cap->supportedChannelWidthSet &
            (IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ >>
            VHT_CAP_SUPP_CHAN_WIDTH_MASK_SHIFT))
            hdd_vht_cap->vht_cap_info |=
                temp_vht_cap <<
                IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160MHZ;
        if (roam_vht_cap->supportedChannelWidthSet &
            (IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ >>
            VHT_CAP_SUPP_CHAN_WIDTH_MASK_SHIFT))
            hdd_vht_cap->vht_cap_info |=
            temp_vht_cap <<
            IEEE80211_VHT_CAP_SUPP_CHAN_WIDTH_160_80PLUS80MHZ;
    if (roam_vht_cap->ldpcCodingCap)
        hdd_vht_cap->vht_cap_info |= IEEE80211_VHT_CAP_RXLDPC;
    if (roam_vht_cap->shortGI80MHz)
        hdd_vht_cap->vht_cap_info |= IEEE80211_VHT_CAP_SHORT_GI_80;
    if (roam_vht_cap->shortGI160and80plus80MHz)
        hdd_vht_cap->vht_cap_info |= IEEE80211_VHT_CAP_SHORT_GI_160;
    if (roam_vht_cap->txSTBC)
        hdd_vht_cap->vht_cap_info |= IEEE80211_VHT_CAP_TXSTBC;
    temp_vht_cap = roam_vht_cap->rxSTBC & (IEEE80211_VHT_CAP_RXSTBC_MASK >>
        VHT_CAP_RXSTBC_MASK_SHIFT);
    if (temp_vht_cap)
        hdd_vht_cap->vht_cap_info |=
            temp_vht_cap << VHT_CAP_RXSTBC_MASK_SHIFT;
    if (roam_vht_cap->suBeamFormerCap)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_SU_BEAMFORMER_CAPABLE;
    if (roam_vht_cap->suBeamformeeCap)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_SU_BEAMFORMEE_CAPABLE;
    temp_vht_cap = roam_vht_cap->csnofBeamformerAntSup &
            (VHT_CAP_BEAMFORMEE_STS_MASK);
    if (temp_vht_cap)
        hdd_vht_cap->vht_cap_info |=
            temp_vht_cap << VHT_CAP_BEAMFORMEE_STS_SHIFT;
    temp_vht_cap = roam_vht_cap->numSoundingDim &
            (VHT_CAP_SOUNDING_DIMENSIONS_MASK);
    if (temp_vht_cap)
        hdd_vht_cap->vht_cap_info |=
            temp_vht_cap << VHT_CAP_SOUNDING_DIMENSIONS_SHIFT;
    if (roam_vht_cap->muBeamformerCap)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_MU_BEAMFORMER_CAPABLE;
    if (roam_vht_cap->muBeamformeeCap)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_MU_BEAMFORMEE_CAPABLE;
    if (roam_vht_cap->vhtTXOPPS)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_VHT_TXOP_PS;
    if (roam_vht_cap->htcVHTCap)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_HTC_VHT;
    temp_vht_cap = roam_vht_cap->maxAMPDULenExp &
            (VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK);
    if (temp_vht_cap)
        hdd_vht_cap->vht_cap_info |=
            temp_vht_cap <<
            VHT_CAP_MAX_A_MPDU_LENGTH_EXPONENT_MASK_SHIFT;
    temp_vht_cap = roam_vht_cap->vhtLinkAdaptCap &
        (IEEE80211_VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB >>
            VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB_SHIFT);
    if (temp_vht_cap)
        hdd_vht_cap->vht_cap_info |= temp_vht_cap <<
            VHT_CAP_VHT_LINK_ADAPTATION_VHT_MRQ_MFB_SHIFT;
    if (roam_vht_cap->rxAntPattern)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_RX_ANTENNA_PATTERN;
    if (roam_vht_cap->txAntPattern)
        hdd_vht_cap->vht_cap_info |=
            IEEE80211_VHT_CAP_TX_ANTENNA_PATTERN;
    hdd_vht_cap->supp_mcs.rx_mcs_map = roam_vht_cap->rxMCSMap;
    hdd_vht_cap->supp_mcs.rx_highest =
        ((uint16_t)roam_vht_cap->rxHighSupDataRate);
    hdd_vht_cap->supp_mcs.tx_mcs_map = roam_vht_cap->txMCSMap;
    hdd_vht_cap->supp_mcs.tx_highest =
        ((uint16_t)roam_vht_cap->txSupDataRate);
}

/* ht param */
#define HT_PARAM_CONTROLLED_ACCESS_ONLY 0x10
#define HT_PARAM_SERVICE_INT_GRAN 0xe0
#define HT_PARAM_SERVICE_INT_GRAN_SHIFT 5

/* operatinon mode */
#define HT_OP_MODE_TX_BURST_LIMIT 0x0008

/* stbc_param */
#define HT_STBC_PARAM_MCS 0x007f

/**
 * hdd_copy_ht_operation()- copy HT operation element from roam info to
 *  hdd station context.
 * @hdd_sta_ctx: pointer to hdd station context
 * @roam_info: pointer to roam info
 *
 * Return: None
 */
static void hdd_copy_ht_operation(hdd_station_ctx_t *hdd_sta_ctx,
                                  tCsrRoamInfo *roam_info)
{
    tDot11fIEHTInfo *roam_ht_ops = &roam_info->ht_operation;
    struct ieee80211_ht_operation *hdd_ht_ops =
                    &hdd_sta_ctx->conn_info.ht_operation;
    uint32_t i, temp_ht_ops;

    vos_mem_zero(hdd_ht_ops, sizeof(struct ieee80211_ht_operation));

    hdd_ht_ops->primary_chan = roam_ht_ops->primaryChannel;

    /* HT_PARAMS */
    temp_ht_ops = roam_ht_ops->secondaryChannelOffset &
        IEEE80211_HT_PARAM_CHA_SEC_OFFSET;
    if (temp_ht_ops)
        hdd_ht_ops->ht_param |= temp_ht_ops;
    else
        hdd_ht_ops->ht_param = IEEE80211_HT_PARAM_CHA_SEC_NONE;
    if (roam_ht_ops->recommendedTxWidthSet)
        hdd_ht_ops->ht_param |= IEEE80211_HT_PARAM_CHAN_WIDTH_ANY;
    if (roam_ht_ops->rifsMode)
        hdd_ht_ops->ht_param |= IEEE80211_HT_PARAM_RIFS_MODE;
    if (roam_ht_ops->controlledAccessOnly)
        hdd_ht_ops->ht_param |= HT_PARAM_CONTROLLED_ACCESS_ONLY;
    temp_ht_ops = roam_ht_ops->serviceIntervalGranularity &
        (HT_PARAM_SERVICE_INT_GRAN >> HT_PARAM_SERVICE_INT_GRAN_SHIFT);
    if (temp_ht_ops)
        hdd_ht_ops->ht_param |= temp_ht_ops <<
            HT_PARAM_SERVICE_INT_GRAN_SHIFT;

    /* operation mode */
    temp_ht_ops = roam_ht_ops->opMode &
            IEEE80211_HT_OP_MODE_PROTECTION;
    switch (temp_ht_ops) {
    case IEEE80211_HT_OP_MODE_PROTECTION_NONMEMBER:
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_PROTECTION_NONMEMBER;
        break;
    case IEEE80211_HT_OP_MODE_PROTECTION_20MHZ:
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_PROTECTION_20MHZ;
        break;
    case IEEE80211_HT_OP_MODE_PROTECTION_NONHT_MIXED:
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_PROTECTION_NONHT_MIXED;
        break;
    case IEEE80211_HT_OP_MODE_PROTECTION_NONE:
    default:
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_PROTECTION_NONE;
    }
    if (roam_ht_ops->nonGFDevicesPresent)
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_NON_GF_STA_PRSNT;
    if (roam_ht_ops->transmitBurstLimit)
        hdd_ht_ops->operation_mode |=
            HT_OP_MODE_TX_BURST_LIMIT;
    if (roam_ht_ops->obssNonHTStaPresent)
        hdd_ht_ops->operation_mode |=
            IEEE80211_HT_OP_MODE_NON_HT_STA_PRSNT;

    /* stbc_param */
    temp_ht_ops = roam_ht_ops->basicSTBCMCS &
            HT_STBC_PARAM_MCS;
    if (temp_ht_ops)
        hdd_ht_ops->stbc_param |= temp_ht_ops;
    if (roam_ht_ops->dualCTSProtection)
        hdd_ht_ops->stbc_param |=
            IEEE80211_HT_STBC_PARAM_DUAL_CTS_PROT;
    if (roam_ht_ops->secondaryBeacon)
        hdd_ht_ops->stbc_param |=
            IEEE80211_HT_STBC_PARAM_STBC_BEACON;
    if (roam_ht_ops->lsigTXOPProtectionFullSupport)
        hdd_ht_ops->stbc_param |=
            IEEE80211_HT_STBC_PARAM_LSIG_TXOP_FULLPROT;
    if (roam_ht_ops->pcoActive)
        hdd_ht_ops->stbc_param |=
            IEEE80211_HT_STBC_PARAM_PCO_ACTIVE;
    if (roam_ht_ops->pcoPhase)
        hdd_ht_ops->stbc_param |=
            IEEE80211_HT_STBC_PARAM_PCO_PHASE;

    /* basic MCs set */
    for (i = 0; i < 16; ++i)
        hdd_ht_ops->basic_set[i] =
            roam_ht_ops->basicMCSSet[i];
}

/**
 * hdd_copy_vht_operation()- copy VHT operations element from roam info to
 *  hdd station context.
 * @hdd_sta_ctx: pointer to hdd station context
 * @roam_info: pointer to roam info
 *
 * Return: None
 */
static void hdd_copy_vht_operation(hdd_station_ctx_t *hdd_sta_ctx,
                                   tCsrRoamInfo *roam_info)
{
    tDot11fIEVHTOperation *roam_vht_ops = &roam_info->vht_operation;
    struct ieee80211_vht_operation *hdd_vht_ops =
                &hdd_sta_ctx->conn_info.vht_operation;

    vos_mem_zero(hdd_vht_ops, sizeof(struct ieee80211_vht_operation));

    hdd_vht_ops->chan_width = roam_vht_ops->chanWidth;
    hdd_vht_ops->center_freq_seg1_idx = roam_vht_ops->chanCenterFreqSeg1;
    hdd_vht_ops->center_freq_seg2_idx = roam_vht_ops->chanCenterFreqSeg2;
    hdd_vht_ops->basic_mcs_set = roam_vht_ops->basicMCSSet;
}


/**
 * hdd_save_bss_info() - save connection info in hdd sta ctx
 * @adapter: Pointer to adapter
 * @roam_info: pointer to roam info
 *
 * Return: None
 */
static void hdd_save_bss_info(hdd_adapter_t *adapter,
                              tCsrRoamInfo *roam_info)
{
    hdd_station_ctx_t *hdd_sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);

    hdd_sta_ctx->conn_info.freq = vos_chan_to_freq(
        hdd_sta_ctx->conn_info.operationChannel);
    if (roam_info->vht_caps.present) {
        hdd_sta_ctx->conn_info.conn_flag.vht_present = true;
        hdd_copy_vht_caps(&hdd_sta_ctx->conn_info.vht_caps,
                          &roam_info->vht_caps);
    } else {
        hdd_sta_ctx->conn_info.conn_flag.vht_present = false;
    }
    if (roam_info->ht_caps.present) {
        hdd_sta_ctx->conn_info.conn_flag.ht_present = true;
        hdd_copy_ht_caps(&hdd_sta_ctx->conn_info.ht_caps, &roam_info->ht_caps);
    } else {
        hdd_sta_ctx->conn_info.conn_flag.ht_present = false;
    }
    if (roam_info->reassoc)
        hdd_sta_ctx->conn_info.roam_count++;
    if (roam_info->hs20vendor_ie.present) {
        hdd_sta_ctx->conn_info.conn_flag.hs20_present = true;
        vos_mem_copy(&hdd_sta_ctx->conn_info.hs20vendor_ie,
            &roam_info->hs20vendor_ie,
            sizeof(roam_info->hs20vendor_ie));
    } else {
        hdd_sta_ctx->conn_info.conn_flag.hs20_present = false;
    }
    if (roam_info->ht_operation.present) {
        hdd_sta_ctx->conn_info.conn_flag.ht_op_present = true;
        hdd_copy_ht_operation(hdd_sta_ctx, roam_info);
    } else {
        hdd_sta_ctx->conn_info.conn_flag.ht_op_present = false;
    }
    if (roam_info->vht_operation.present) {
        hdd_sta_ctx->conn_info.conn_flag.vht_op_present = true;
        hdd_copy_vht_operation(hdd_sta_ctx, roam_info);
    } else {
        hdd_sta_ctx->conn_info.conn_flag.vht_op_present = false;
    }
    /* Cache last connection info */
    vos_mem_copy(&hdd_sta_ctx->cache_conn_info, &hdd_sta_ctx->conn_info,
                 sizeof(connection_info_t));
}

void hdd_connSaveConnectInfo( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo, eCsrRoamBssType eBssType )
{
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   eCsrEncryptionType encryptType = eCSR_ENCRYPT_TYPE_NONE;

   VOS_ASSERT( pRoamInfo );

   if ( pRoamInfo )
   {
      // Save the BSSID for the connection...
      if ( eCSR_BSS_TYPE_INFRASTRUCTURE == eBssType )
      {
          VOS_ASSERT( pRoamInfo->pBssDesc );
          vos_mem_copy(pHddStaCtx->conn_info.bssId, pRoamInfo->bssid,6 );

          // Save the Station ID for this station from the 'Roam Info'.
          //For IBSS mode, staId is assigned in NEW_PEER_IND
          //For reassoc, the staID doesn't change and it may be invalid in this structure
          //so no change here.
          if( !pRoamInfo->fReassocReq )
          {
              pHddStaCtx->conn_info.staId [0]= pRoamInfo->staId;
          }
      }
      else if ( eCSR_BSS_TYPE_IBSS == eBssType )
      {
         vos_mem_copy(pHddStaCtx->conn_info.bssId, pRoamInfo->bssid,sizeof(pRoamInfo->bssid) );
      }
      else
      {
         // can't happen.  We need a valid IBSS or Infra setting in the BSSDescription
         // or we can't function.
         VOS_ASSERT( 0 );
      }

      // notify WMM
      hdd_wmm_connect(pAdapter, pRoamInfo, eBssType);

      if( !pRoamInfo->u.pConnectedProfile )
      {
         VOS_ASSERT( pRoamInfo->u.pConnectedProfile );
      }
      else
      {
          // Get Multicast Encryption Type
          encryptType =  pRoamInfo->u.pConnectedProfile->mcEncryptionType;
          pHddStaCtx->conn_info.mcEncryptionType = encryptType;
          // Get Unicast Encrytion Type
          encryptType =  pRoamInfo->u.pConnectedProfile->EncryptionType;
          pHddStaCtx->conn_info.ucEncryptionType = encryptType;

          pHddStaCtx->conn_info.authType =  pRoamInfo->u.pConnectedProfile->AuthType;

          pHddStaCtx->conn_info.operationChannel = pRoamInfo->u.pConnectedProfile->operationChannel;

          // Save the ssid for the connection
          vos_mem_copy( &pHddStaCtx->conn_info.SSID.SSID, &pRoamInfo->u.pConnectedProfile->SSID, sizeof( tSirMacSSid ) );

          // Save  dot11mode in which STA associated to AP
          pHddStaCtx->conn_info.dot11Mode = pRoamInfo->u.pConnectedProfile->dot11Mode;

          pHddStaCtx->conn_info.rate_flags = pRoamInfo->maxRateFlags;
      }
      hdd_save_bss_info(pAdapter, pRoamInfo);
   }

   // save the connected BssType
   hdd_connSaveConnectedBssType( pHddStaCtx, eBssType );

}

#if defined(WLAN_FEATURE_VOWIFI_11R)
/*
 * Send the 11R key information to the supplicant.
 * Only then can the supplicant generate the PMK-R1.
 * (BTW, the ESE supplicant also needs the Assoc Resp IEs
 * for the same purpose.)
 *
 * Mainly the Assoc Rsp IEs are passed here. For the IMDA
 * this contains the R1KHID, R0KHID and the MDID.
 * For FT, this consists of the Reassoc Rsp FTIEs.
 * This is the Assoc Response.
 */
static void hdd_SendFTAssocResponse(struct net_device *dev, hdd_adapter_t *pAdapter,
                tCsrRoamInfo *pCsrRoamInfo)
{
    union iwreq_data wrqu;
    char *buff;
    unsigned int len = 0;
    u8 *pFTAssocRsp = NULL;

    if (pCsrRoamInfo->nAssocRspLength == 0)
    {
        hddLog(LOGE,
            "%s: pCsrRoamInfo->nAssocRspLength=%d",
            __func__, (int)pCsrRoamInfo->nAssocRspLength);
        return;
    }

    pFTAssocRsp = (u8 *)(pCsrRoamInfo->pbFrames + pCsrRoamInfo->nBeaconLength +
        pCsrRoamInfo->nAssocReqLength);
    if (pFTAssocRsp == NULL)
    {
        hddLog(LOGE, "%s: AssocReq or AssocRsp is NULL", __func__);
        return;
    }

    // pFTAssocRsp needs to point to the IEs
    pFTAssocRsp += FT_ASSOC_RSP_IES_OFFSET;
    hddLog(LOG1, "%s: AssocRsp is now at %02x%02x", __func__,
        (unsigned int)pFTAssocRsp[0],
        (unsigned int)pFTAssocRsp[1]);

    // We need to send the IEs to the supplicant.
    buff = kmalloc(IW_GENERIC_IE_MAX, GFP_ATOMIC);
    if (buff == NULL)
    {
        hddLog(LOGE, "%s: kmalloc unable to allocate memory", __func__);
        return;
    }

    // Send the Assoc Resp, the supplicant needs this for initial Auth.
    len = pCsrRoamInfo->nAssocRspLength - FT_ASSOC_RSP_IES_OFFSET;
    wrqu.data.length = len;
    memset(buff, 0, IW_GENERIC_IE_MAX);
    memcpy(buff, pFTAssocRsp, len);
    wireless_send_event(dev, IWEVASSOCRESPIE, &wrqu, buff);

    kfree(buff);
}
#endif /* WLAN_FEATURE_VOWIFI_11R */

#ifdef WLAN_FEATURE_VOWIFI_11R

/*---------------------------------------------------
 *
 * Send the FTIEs, RIC IEs during FT. This is eventually
 * used to send the FT events to the supplicant
 *
 * At the reception of Auth2 we send the RIC followed
 * by the auth response IEs to the supplicant.
 * Once both are received in the supplicant, an FT
 * event is generated to the supplicant.
 *
 *---------------------------------------------------
 */
void hdd_SendFTEvent(hdd_adapter_t *pAdapter)
{
    tANI_U16 auth_resp_len = 0;
    tANI_U32 ric_ies_length = 0;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

#if defined(KERNEL_SUPPORT_11R_CFG80211)
    struct cfg80211_ft_event_params ftEvent;
    v_U8_t ftIe[DOT11F_IE_FTINFO_MAX_LEN];
    v_U8_t ricIe[DOT11F_IE_RICDESCRIPTOR_MAX_LEN];
    struct net_device *dev = pAdapter->dev;
#else
    char *buff;
    union iwreq_data wrqu;
    tANI_U16 str_len;
#endif

#if defined(KERNEL_SUPPORT_11R_CFG80211)
    vos_mem_zero(ftIe, DOT11F_IE_FTINFO_MAX_LEN);
    vos_mem_zero(ricIe, DOT11F_IE_RICDESCRIPTOR_MAX_LEN);

    sme_GetRICIEs(pHddCtx->hHal, (u8 *)ricIe,
                  DOT11F_IE_RICDESCRIPTOR_MAX_LEN, &ric_ies_length );
    if (ric_ies_length == 0)
    {
        hddLog(LOGW,
              "%s: RIC IEs is of length 0 not sending RIC Information for now",
              __func__);
    }

    ftEvent.ric_ies = ricIe;
    ftEvent.ric_ies_len = ric_ies_length;
    hddLog(LOG1, "%s: RIC IEs is of length %d", __func__, (int)ric_ies_length);

    sme_GetFTPreAuthResponse(pHddCtx->hHal, (u8 *)ftIe,
                DOT11F_IE_FTINFO_MAX_LEN, &auth_resp_len);

    if (auth_resp_len == 0)
    {
        hddLog(LOGE, "%s: AuthRsp FTIES is of length 0", __func__);
        return;
    }

    sme_SetFTPreAuthState(pHddCtx->hHal, TRUE);

    ftEvent.target_ap = ftIe;

    ftEvent.ies = (u8 *)(ftIe + SIR_MAC_ADDR_LENGTH);
    ftEvent.ies_len = auth_resp_len - SIR_MAC_ADDR_LENGTH;

    hddLog(LOG1, "%s ftEvent.ies_len %zu", __FUNCTION__, ftEvent.ies_len);
    hddLog(LOG1, "%s ftEvent.ric_ies_len %zu",
           __FUNCTION__, ftEvent.ric_ies_len );
    hddLog(LOG1, "%s ftEvent.target_ap %2x-%2x-%2x-%2x-%2x-%2x ",
            __FUNCTION__, ftEvent.target_ap[0], ftEvent.target_ap[1],
            ftEvent.target_ap[2], ftEvent.target_ap[3], ftEvent.target_ap[4],
            ftEvent.target_ap[5]);

    (void)cfg80211_ft_event(dev, &ftEvent);

#else
    // We need to send the IEs to the supplicant
    buff = kmalloc(IW_CUSTOM_MAX, GFP_ATOMIC);
    if (buff == NULL)
    {
        hddLog(LOGE, "%s: kmalloc unable to allocate memory", __func__);
        return;
    }
    vos_mem_zero(buff, IW_CUSTOM_MAX);

    // Sme needs to send the RIC IEs first
    str_len = strlcpy(buff, "RIC=", IW_CUSTOM_MAX);
    sme_GetRICIEs( pHddCtx->hHal, (u8 *)&(buff[str_len]),
            (IW_CUSTOM_MAX - str_len), &ric_ies_length );
    if (ric_ies_length == 0)
    {
        hddLog(LOGW,
               "%s: RIC IEs is of length 0 not sending RIC Information for now",
               __func__);
    }
    else
    {
        wrqu.data.length = str_len + ric_ies_length;
        wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buff);
    }

    // Sme needs to provide the Auth Resp
    vos_mem_zero(buff, IW_CUSTOM_MAX);
    str_len = strlcpy(buff, "AUTH=", IW_CUSTOM_MAX);
    sme_GetFTPreAuthResponse(pHddCtx->hHal, (u8 *)&buff[str_len],
                    (IW_CUSTOM_MAX - str_len), &auth_resp_len);

    if (auth_resp_len == 0)
    {
        hddLog(LOGE, "%s: AuthRsp FTIES is of length 0", __func__);
        kfree(buff);
        return;
    }

    wrqu.data.length = str_len + auth_resp_len;
    wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buff);

    kfree(buff);
#endif
}

#endif /* WLAN_FEATURE_VOWIFI_11R */

#ifdef FEATURE_WLAN_ESE

/*
 * Send the ESE required "new AP Channel info" to the supplicant.
 * (This keeps the supplicant "up to date" on the current channel.)
 *
 * The current (new AP) channel information is passed in.
 */
static void hdd_SendNewAPChannelInfo(struct net_device *dev, hdd_adapter_t *pAdapter,
                tCsrRoamInfo *pCsrRoamInfo)
{
    union iwreq_data wrqu;
    tSirBssDescription *descriptor = pCsrRoamInfo->pBssDesc;


    if (descriptor == NULL)
    {
        hddLog(LOGE,
            "%s: pCsrRoamInfo->pBssDesc=%pK",
            __func__, descriptor);
        return;
    }

    // Send the Channel event, the supplicant needs this to generate the Adjacent AP report.
    hddLog(LOGW, "%s: Sending up an SIOCGIWFREQ, channelId=%d", __func__, descriptor->channelId);
    memset(&wrqu, '\0', sizeof(wrqu));
    wrqu.freq.m = descriptor->channelId;
    wrqu.freq.e = 0;
    wrqu.freq.i = 0;
    wireless_send_event(pAdapter->dev, SIOCGIWFREQ, &wrqu, NULL);
}

#endif /* FEATURE_WLAN_ESE */

void hdd_SendUpdateBeaconIEsEvent(hdd_adapter_t *pAdapter, tCsrRoamInfo *pCsrRoamInfo)
{
    union iwreq_data wrqu;
    u8  *pBeaconIes;
    u8 currentLen = 0;
    char *buff;
    int totalIeLen = 0, currentOffset = 0, strLen;

    memset(&wrqu, '\0', sizeof(wrqu));

    if (0 == pCsrRoamInfo->nBeaconLength)
    {
        hddLog(LOGE, "%s: pCsrRoamInfo->nBeaconFrameLength = 0", __func__);
        return;
    }
    pBeaconIes = (u8 *)(pCsrRoamInfo->pbFrames + BEACON_FRAME_IES_OFFSET);
    if (pBeaconIes == NULL)
    {
        hddLog(LOGE, "%s: Beacon IEs is NULL", __func__);
        return;
    }

    // pBeaconIes needs to point to the IEs
    hddLog(LOG1, "%s: Beacon IEs is now at %02x%02x", __func__,
        (unsigned int)pBeaconIes[0],
        (unsigned int)pBeaconIes[1]);
    hddLog(LOG1, "%s: Beacon IEs length = %d", __func__, pCsrRoamInfo->nBeaconLength - BEACON_FRAME_IES_OFFSET);

   // We need to send the IEs to the supplicant.
    buff = kmalloc(IW_CUSTOM_MAX, GFP_ATOMIC);
    if (buff == NULL)
    {
        hddLog(LOGE, "%s: kmalloc unable to allocate memory", __func__);
        return;
    }
    vos_mem_zero(buff, IW_CUSTOM_MAX);

    strLen = strlcpy(buff,"BEACONIEs=", IW_CUSTOM_MAX);
    currentLen = strLen + 1;

    totalIeLen = pCsrRoamInfo->nBeaconLength - BEACON_FRAME_IES_OFFSET;
    do
    {
        /* If the beacon size exceeds max CUSTOM event size, break it into chunks of CUSTOM event
         * max size and send it to supplicant. Changes are done in supplicant to handle this */
        vos_mem_zero(&buff[strLen + 1], IW_CUSTOM_MAX - (strLen + 1));
        currentLen = VOS_MIN(totalIeLen, IW_CUSTOM_MAX - (strLen + 1) - 1);
        vos_mem_copy(&buff[strLen + 1], pBeaconIes+currentOffset, currentLen);
        currentOffset += currentLen;
        totalIeLen -= currentLen;
        wrqu.data.length = strLen + 1 + currentLen;
        if (totalIeLen)
          buff[strLen] = 1;   // This tells supplicant more chunks are pending
        else
          buff[strLen] = 0;   // For last chunk of beacon IE to supplicant

        hddLog(LOG1, "%s: Beacon IEs length to supplicant = %d", __func__, currentLen);
        wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buff);
    } while (totalIeLen > 0);

    kfree(buff);
}

static void hdd_SendAssociationEvent(struct net_device *dev,tCsrRoamInfo *pCsrRoamInfo)
{
    hdd_adapter_t *pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    union iwreq_data wrqu;
    int we_event;
    char *msg;
    int type = -1;

#if defined (WLAN_FEATURE_VOWIFI_11R)
    // Added to find the auth type on the fly at run time
    // rather than with cfg to see if FT is enabled
    hdd_wext_state_t  *pWextState =  WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    tCsrRoamProfile* pRoamProfile = &(pWextState->roamProfile);
#endif

    memset(&wrqu, '\0', sizeof(wrqu));
    wrqu.ap_addr.sa_family = ARPHRD_ETHER;
    we_event = SIOCGIWAP;

    if(eConnectionState_Associated == pHddStaCtx->conn_info.connState)/* Associated */
    {
       /* In case of roaming ; We are not doing disconnect.
        * If disconnect is not being done for roam; We will not
        * decrease count for Active sessions. We should not increase active
        * active session in case of roaming.
        */
       if((pHddStaCtx->ft_carrier_on == FALSE) && !pCsrRoamInfo->fReassocReq)
       {
           wlan_hdd_incr_active_session(pHddCtx, pAdapter->device_mode);
       }
        memcpy(wrqu.ap_addr.sa_data, pCsrRoamInfo->pBssDesc->bssId, sizeof(pCsrRoamInfo->pBssDesc->bssId));
        type = WLAN_STA_ASSOC_DONE_IND;

#ifdef WLAN_FEATURE_P2P_DEBUG
        if(pAdapter->device_mode == WLAN_HDD_P2P_CLIENT)
        {
             if(globalP2PConnectionStatus == P2P_CLIENT_CONNECTING_STATE_1)
             {
                 globalP2PConnectionStatus = P2P_CLIENT_CONNECTED_STATE_1;
                 hddLog(VOS_TRACE_LEVEL_ERROR,"[P2P State] Changing state from "
                                "Connecting state to Connected State for 8-way "
                                "Handshake");
             }
             else if(globalP2PConnectionStatus == P2P_CLIENT_CONNECTING_STATE_2)
             {
                 globalP2PConnectionStatus = P2P_CLIENT_COMPLETED_STATE;
                 hddLog(VOS_TRACE_LEVEL_ERROR,"[P2P State] Changing state from "
                           "Connecting state to P2P Client Connection Completed");
             }
        }
#endif
	hddLog(VOS_TRACE_LEVEL_ERROR, MAC_ADDRESS_STR " connected to "
	       MAC_ADDRESS_STR,
	       MAC_ADDR_ARRAY(pAdapter->macAddressCurrent.bytes),
	       MAC_ADDR_ARRAY(wrqu.ap_addr.sa_data));
        hdd_SendUpdateBeaconIEsEvent(pAdapter, pCsrRoamInfo);

        hdd_manage_delack_timer(pHddCtx);

        /* Send IWEVASSOCRESPIE Event if WLAN_FEATURE_CIQ_METRICS is Enabled Or
         * Send IWEVASSOCRESPIE Event if WLAN_FEATURE_VOWIFI_11R is Enabled
         * and fFTEnable is TRUE */
#ifdef WLAN_FEATURE_VOWIFI_11R
        // Send FT Keys to the supplicant when FT is enabled
        if ((pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_FT_RSN_PSK) ||
            (pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_FT_RSN)
#ifdef FEATURE_WLAN_ESE
            || (pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_CCKM_RSN) ||
            (pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_CCKM_WPA)
#endif
        )
        {
            hdd_SendFTAssocResponse(dev, pAdapter, pCsrRoamInfo);
        }
#endif
    }
    else if (eConnectionState_IbssConnected == pHddStaCtx->conn_info.connState) // IBss Associated
    {
        wlan_hdd_incr_active_session(pHddCtx, pAdapter->device_mode);
        memcpy(wrqu.ap_addr.sa_data, pHddStaCtx->conn_info.bssId, ETH_ALEN);
        type = WLAN_STA_ASSOC_DONE_IND;

        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "wlan: new IBSS connection to " MAC_ADDRESS_STR,
                 MAC_ADDR_ARRAY(pHddStaCtx->conn_info.bssId));
    }
    else /* Not Associated */
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "wlan: disconnected");
        type = WLAN_STA_DISASSOC_DONE_IND;
        memset(wrqu.ap_addr.sa_data,'\0',ETH_ALEN);

        hdd_manage_delack_timer(pHddCtx);
    }
    hdd_dump_concurrency_info(pHddCtx);

    msg = NULL;
    /*During the WLAN uninitialization,supplicant is stopped before the
      driver so not sending the status of the connection to supplicant*/
    if(pHddCtx->isLoadUnloadInProgress == WLAN_HDD_NO_LOAD_UNLOAD_IN_PROGRESS)
    {
        wireless_send_event(dev, we_event, &wrqu, msg);
#ifdef FEATURE_WLAN_ESE
        if(eConnectionState_Associated == pHddStaCtx->conn_info.connState)/* Associated */
        {
            if ( (pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_CCKM_RSN) ||
                (pRoamProfile->AuthType.authType[0] == eCSR_AUTH_TYPE_CCKM_WPA) )
            hdd_SendNewAPChannelInfo(dev, pAdapter, pCsrRoamInfo);
        }
#endif
    }
    send_btc_nlink_msg(type, 0);
}

void hdd_connRemoveConnectInfo( hdd_station_ctx_t *pHddStaCtx )
{
   // Remove staId, bssId and peerMacAddress
   pHddStaCtx->conn_info.staId [ 0 ] = 0;
   vos_mem_zero( &pHddStaCtx->conn_info.bssId, sizeof( v_MACADDR_t ) );
   vos_mem_zero( &pHddStaCtx->conn_info.peerMacAddress[ 0 ], sizeof( v_MACADDR_t ) );

   // Clear all security settings
   pHddStaCtx->conn_info.authType         = eCSR_AUTH_TYPE_OPEN_SYSTEM;
   pHddStaCtx->conn_info.mcEncryptionType = eCSR_ENCRYPT_TYPE_NONE;
   pHddStaCtx->conn_info.ucEncryptionType = eCSR_ENCRYPT_TYPE_NONE;

   vos_mem_zero( &pHddStaCtx->conn_info.Keys, sizeof( tCsrKeys ) );
   vos_mem_zero( &pHddStaCtx->ibss_enc_key, sizeof(tCsrRoamSetKey) );

   // Set not-connected state
   pHddStaCtx->conn_info.connDot11DesiredBssType = eCSR_BSS_TYPE_ANY;

   vos_mem_zero( &pHddStaCtx->conn_info.SSID, sizeof( tCsrSSIDInfo ) );
}

VOS_STATUS hdd_ibss_deinit_tx_rx_sta ( hdd_adapter_t *pAdapter, v_U8_t STAId )
{
   v_U8_t ac;
   /**Track whether OS TX queue has been disabled.*/
   v_BOOL_t txSuspended[NUM_TX_QUEUES];
   v_U8_t tlAC;
   v_U8_t i;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   hdd_ibss_peer_info_t *pPeerInfo;

   if( NULL == pHddStaCtx )
   {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: HDD station context NULL ",__func__);
        return VOS_STATUS_E_FAILURE;
   }

   pPeerInfo = &pHddStaCtx->ibss_peer_info;
   if (FALSE == pPeerInfo->ibssStaInfo[STAId].isUsed)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Deinit station not inited %d", __func__, STAId );
      return VOS_STATUS_E_FAILURE;
   }

   hdd_flush_ibss_tx_queues(pAdapter, STAId);

   for (ac = HDD_LINUX_AC_VO; ac <= HDD_LINUX_AC_BK; ac++)
   {
      tlAC = hdd_QdiscAcToTlAC[ac];
      txSuspended[ac] = pPeerInfo->ibssStaInfo[STAId].txSuspended[tlAC];
   }

   vos_mem_zero(&pPeerInfo->ibssStaInfo[STAId], sizeof(hdd_ibss_station_info_t));

   /* re-init hdd list, since netdev can still open adapter until
    * driver gets unloaded
    */
   for (i = 0; i < NUM_TX_QUEUES; i ++)
   {
      hdd_list_init(&pPeerInfo->ibssStaInfo[STAId].wmm_tx_queue[i],
                    HDD_TX_QUEUE_MAX_LEN);
   }

   for (ac = HDD_LINUX_AC_VO; ac <= HDD_LINUX_AC_BK; ac++)
   {
      if (txSuspended[ac])
      {
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
                    "%s: TX queue re-enabled", __func__);
         netif_wake_subqueue(pAdapter->dev, ac);
      }
   }
   return VOS_STATUS_SUCCESS;
}

static VOS_STATUS hdd_ibss_DeregisterSTA( hdd_adapter_t *pAdapter, tANI_U8 staId )
{
    VOS_STATUS vosStatus;

    vosStatus = WLANTL_ClearSTAClient( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext, staId );
    if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: WLANTL_ClearSTAClient() failed to for staID %d.  "
                   "Status= %d [0x%08X]",
                   __func__, staId, vosStatus, vosStatus );
    }

    vosStatus = hdd_ibss_deinit_tx_rx_sta ( pAdapter, staId );
    if( VOS_STATUS_E_FAILURE == vosStatus )
    {
        VOS_TRACE ( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                    "hdd_ibss_deinit_tx_rx_sta() failed for staID %d. "
                    "Status = %d [0x%08X]",
                    staId, vosStatus, vosStatus );
    }

    return( vosStatus );
}

VOS_STATUS hdd_ibss_init_tx_rx_sta( hdd_adapter_t *pAdapter, v_U8_t STAId, v_MACADDR_t *pmacAddrSTA)
{
   v_U8_t i = 0;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   hdd_ibss_peer_info_t * pPeerInfo = &pHddStaCtx->ibss_peer_info;

   if (pPeerInfo->ibssStaInfo[STAId].isUsed)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                 "%s: Reinit station %d", __func__, STAId );
      return VOS_STATUS_E_FAILURE;
   }

   vos_mem_zero(&pPeerInfo->ibssStaInfo[STAId], sizeof(hdd_ibss_station_info_t));
   for (i = 0; i < NUM_TX_QUEUES; i ++)
   {
      hdd_list_init(&pPeerInfo->ibssStaInfo[STAId].wmm_tx_queue[i], HDD_TX_QUEUE_MAX_LEN);
   }

   pPeerInfo->ibssStaInfo[STAId].isUsed = VOS_TRUE;
   pPeerInfo->ibssStaInfo[STAId].isDeauthInProgress = VOS_FALSE;
   vos_copy_macaddr( &pPeerInfo->ibssStaInfo[STAId].macAddrSTA, pmacAddrSTA);

   return VOS_STATUS_SUCCESS;
}

static VOS_STATUS hdd_ibss_RegisterSTA( hdd_adapter_t *pAdapter,
                                       tCsrRoamInfo *pRoamInfo,
                                       v_U8_t staId,
                                       v_MACADDR_t *pPeerMacAddress,
                                       tSirBssDescription *pBssDesc )
{
   VOS_STATUS vosStatus = VOS_STATUS_E_FAILURE;
   WLAN_STADescType staDesc = {0};
   eCsrEncryptionType connectedCipherAlgo;
   v_BOOL_t  fConnected;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   hdd_ibss_peer_info_t * pPeerInfo = &pHddStaCtx->ibss_peer_info;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);

   if ( pPeerInfo->ibssStaInfo[staId].isUsed )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
                 "clean up old entry for STA %d", staId);
      hdd_ibss_DeregisterSTA( pAdapter, staId );
   }

   staDesc.ucSTAId = staId;
   staDesc.wSTAType = WLAN_STA_IBSS;

   // Note that for IBSS, the STA MAC address and BSSID are goign to be different where
   // in infrastructure, they are the same (BSSID is the MAC address of the AP).  So,
   // for IBSS we have a second field to pass to TL in the STA descriptor that we don't
   // pass when making an Infrastructure connection.
   vos_mem_copy(staDesc.vSTAMACAddress.bytes, pPeerMacAddress->bytes,sizeof(pPeerMacAddress->bytes));
   vos_mem_copy( staDesc.vBSSIDforIBSS.bytes, pHddStaCtx->conn_info.bssId, 6 );
   vos_copy_macaddr( &staDesc.vSelfMACAddress, &pAdapter->macAddressCurrent );

   if (hdd_wmm_is_active(pAdapter))
   {
      staDesc.ucQosEnabled = 1;
   }
   else
   {
      staDesc.ucQosEnabled = 0;
   }
   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
              "HDD SOFTAP register TL QoS_enabled=%d",
              staDesc.ucQosEnabled );

   fConnected = hdd_connGetConnectedCipherAlgo( pHddStaCtx, &connectedCipherAlgo );
   if ( connectedCipherAlgo != eCSR_ENCRYPT_TYPE_NONE )
   {
      staDesc.ucProtectedFrame = 1;
   }
   else
   {
      staDesc.ucProtectedFrame = 0;

   }

   hdd_ibss_init_tx_rx_sta(pAdapter, staId, &staDesc.vSTAMACAddress);

   // UMA is Not ready yet, Xlation will be done by TL
   staDesc.ucSwFrameTXXlation = 1;
   staDesc.ucSwFrameRXXlation = 1;
   staDesc.ucAddRmvLLC = 1;
   // Initialize signatures and state
   staDesc.ucUcastSig  = pRoamInfo->ucastSig;
   staDesc.ucBcastSig  = pRoamInfo->bcastSig;
   staDesc.ucInitState = WLANTL_STA_AUTHENTICATED;

   staDesc.ucIsReplayCheckValid = VOS_FALSE;

   // Register the Station with TL.
   vosStatus = WLANTL_RegisterSTAClient( pHddCtx->pvosContext,
                                         hdd_rx_packet_cbk,
                                         hdd_tx_complete_cbk,
                                         hdd_ibss_tx_fetch_packet_cbk, &staDesc,
                                         pBssDesc->rssi );
   if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "WLANTL_RegisterSTAClient() failed to register.  Status= %d [0x%08X]",
                 vosStatus, vosStatus );
      return vosStatus;
   }

   //Timer value should be in milliseconds
   if ( pHddCtx->cfg_ini->dynSplitscan &&
      ( VOS_TIMER_STATE_RUNNING !=
                      vos_timer_getCurrentState(&pHddCtx->tx_rx_trafficTmr)))
   {
       vos_timer_start(&pHddCtx->tx_rx_trafficTmr,
                        pHddCtx->cfg_ini->trafficMntrTmrForSplitScan);
   }

   pPeerInfo->ibssStaInfo[staId].ucSTAId = staId;
   pPeerInfo->ibssStaInfo[staId].isQosEnabled = staDesc.ucQosEnabled;

   vosStatus = WLANTL_ChangeSTAState( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext, staDesc.ucSTAId,
                                      WLANTL_STA_AUTHENTICATED );

   pPeerInfo->ibssStaInfo[staId].tlSTAState = WLANTL_STA_AUTHENTICATED;
   pHddStaCtx->conn_info.uIsAuthenticated = VOS_TRUE;

   return( vosStatus );
}

/* TODO Revist this function. and data path */
static VOS_STATUS hdd_roamDeregisterSTA( hdd_adapter_t *pAdapter, tANI_U8 staId )
{
    VOS_STATUS vosStatus;

    hdd_disconnect_tx_rx(pAdapter);

    vosStatus = WLANTL_ClearSTAClient( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext, staId );
    if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: WLANTL_ClearSTAClient() failed to for staID %d.  "
                   "Status= %d [0x%08X]",
                   __func__, staId, vosStatus, vosStatus );
    }
    return( vosStatus );
}

/**
 * hdd_print_bss_info() - print bss info
 * @hdd_sta_ctx: pointer to hdd station context
 *
 * Return: None
 */
void hdd_print_bss_info(hdd_station_ctx_t *hdd_sta_ctx)
{
    uint32_t *cap_info;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"WIFI DATA LOGGER");
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"channel: %d",
              hdd_sta_ctx->conn_info.freq);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"dot11mode: %d",
              hdd_sta_ctx->conn_info.dot11Mode);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"AKM: %d",
              hdd_sta_ctx->conn_info.authType);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"ssid: %.*s",
              hdd_sta_ctx->conn_info.SSID.SSID.length,
              hdd_sta_ctx->conn_info.SSID.SSID.ssId);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"roam count: %d",
              hdd_sta_ctx->conn_info.roam_count);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"ant_info: %d",
              hdd_sta_ctx->conn_info.txrate.nss);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"datarate legacy %d",
              hdd_sta_ctx->conn_info.txrate.legacy);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"datarate mcs: %d",
              hdd_sta_ctx->conn_info.txrate.mcs);
    if (hdd_sta_ctx->conn_info.conn_flag.ht_present) {
        cap_info = (uint32_t *)&hdd_sta_ctx->conn_info.ht_caps;
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"ht caps: %x",
                  *cap_info);
    }
    if (hdd_sta_ctx->conn_info.conn_flag.vht_present) {
        cap_info = (uint32_t *)&hdd_sta_ctx->conn_info.vht_caps;
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"vht caps: %x",
                  *cap_info);
    }
    if (hdd_sta_ctx->conn_info.conn_flag.hs20_present)
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"hs20 info: %x",
                  hdd_sta_ctx->conn_info.hs20vendor_ie.release_num);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"signal: %d",
              hdd_sta_ctx->conn_info.signal);
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,"noise: %d",
              hdd_sta_ctx->conn_info.noise);
}

/**
 * hdd_check_and_move_if_sap_is_on_dfs_chan() - move the sap to non dfs channel
 * @hdd_ctx: pointer to hdd context
 * @sta_adapter: pointer to sta adapater
 *
 * This function is used to check if SAP is operating on DFS channel in stand
 * alone mode and move it to non dfs channel
 *
 * Return: void.
 */
static void hdd_check_and_move_if_sap_is_on_dfs_chan(hdd_context_t *hdd_ctx,
        hdd_adapter_t *sta_adapter)
{
    hdd_adapter_t *sap_adapter;
    ptSapContext sap_ctx;
    v_CONTEXT_t vos_ctx;
    eNVChannelEnabledType chan_state;

    if (hdd_is_sta_sap_scc_allowed_on_dfs_chan(hdd_ctx)) {
        sap_adapter = hdd_get_adapter(hdd_ctx, WLAN_HDD_SOFTAP);

        if (!sap_adapter) {
            hddLog(LOG1, FL("SAP not exists, nothing to do"));
            return;
        }

        vos_ctx = hdd_ctx->pvosContext;
        if (!vos_ctx) {
            hddLog(LOGE, FL("vos_ctx is NULL"));
            return;
        }
        sap_ctx = VOS_GET_SAP_CB(vos_ctx);
        if (!sap_ctx) {
            hddLog(LOG1, FL("sap_ctx not exists"));
            return;
        }

        if (sap_ctx->sapsMachine != eSAP_STARTED) {
            hddLog(LOG1, FL("SAP is not in eSAP_STARTED state"));
            return;
        }

        chan_state = vos_nv_getChannelEnabledState(sap_ctx->channel);

        hddLog(LOG1, "SAP is operating on channel (%hu), chan_state %d",
                sap_ctx->channel, chan_state);
        if (vos_nv_getChannelEnabledState(sap_ctx->channel) !=
                NV_CHANNEL_DFS) {
            hddLog(LOG1, "SAP is on non DFS channel. nothing to do");
            return;
        }

        hddLog(LOG1, "Schedule workqueue to move the SAP to non DFS channel");
        schedule_delayed_work(&hdd_ctx->ecsa_chan_change_work,
                            msecs_to_jiffies(ECSA_DFS_CHAN_CHANGE_DEFER_TIME));
    }
}

static eHalStatus hdd_DisConnectHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                                            tANI_U32 roamId, eRoamCmdStatus roamStatus,
                                            eCsrRoamResult roamResult )
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    VOS_STATUS vstatus;
    struct net_device *dev = pAdapter->dev;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    v_U8_t sta_id;
    v_BOOL_t sendDisconInd = TRUE;

    // Sanity check
    if(dev == NULL)
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
          "%s: net_dev is released return", __func__);
        return eHAL_STATUS_FAILURE;
    }

    // notify apps that we can't pass traffic anymore
    hddLog(VOS_TRACE_LEVEL_INFO, FL("Disabling queues"));
    netif_tx_disable(dev);
    netif_carrier_off(dev);
    //TxTimeoutCount need to reset in case of disconnect handler
    pAdapter->hdd_stats.hddTxRxStats.continuousTxTimeoutCount = 0;

    wlan_hdd_check_and_stop_mon(pAdapter, false);

    INIT_COMPLETION(pAdapter->disconnect_comp_var);
    /* If only STA mode is on */
    if((pHddCtx->concurrency_mode <= 1) &&
       (pHddCtx->no_of_open_sessions[WLAN_HDD_INFRA_STATION] <= 1))
    {
        pHddCtx->isAmpAllowed = VOS_TRUE;
    }

    /* Need to apply spin lock before decreasing active sessions
     * as there can be chance for double decrement if context switch
     * Calls wlan_hdd_disconnect.
     */

    spin_lock_bh(&pAdapter->lock_for_active_session);

    /* HDD has initiated disconnect, do not send disconnect indication
     * to kernel. Sending disconnected event to kernel for userspace
     * initiated disconnect will be handled by diconnect handler call
     * to cfg80211_disconnected
     */
    if ((eConnectionState_Disconnecting == pHddStaCtx->conn_info.connState) ||
        (eConnectionState_NotConnected == pHddStaCtx->conn_info.connState) ||
        (eConnectionState_Connecting == pHddStaCtx->conn_info.connState))
    {
       VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   FL(" HDD has initiated a disconnect, no need to send"
                   " disconnect indication to kernel"));
       sendDisconInd = FALSE;
    }
    else if (eConnectionState_Associated == pHddStaCtx->conn_info.connState)
    {
       VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
            FL("Set HDD connState to eConnectionState_Disconnecting from %d "),
                       pHddStaCtx->conn_info.connState);
       hdd_connSetConnectionState( pHddStaCtx, eConnectionState_Disconnecting );
       wlan_hdd_decr_active_session(pHddCtx, pAdapter->device_mode);
    }
    spin_unlock_bh(&pAdapter->lock_for_active_session);
    vos_flush_delayed_work(&pHddCtx->ecsa_chan_change_work);
    hdd_clearRoamProfileIe( pAdapter );

    hdd_wmm_init( pAdapter );

    // indicate 'disconnect' status to wpa_supplicant...
    hdd_SendAssociationEvent(dev,pRoamInfo);
    /* indicate disconnected event to nl80211 */
    if(roamStatus != eCSR_ROAM_IBSS_LEAVE)
    {
        /*During the WLAN uninitialization,supplicant is stopped before the
            driver so not sending the status of the connection to supplicant*/
        if(pHddCtx->isLoadUnloadInProgress == WLAN_HDD_NO_LOAD_UNLOAD_IN_PROGRESS)
        {
            if (sendDisconInd)
                hddLog(VOS_TRACE_LEVEL_INFO_HIGH,
                       "%s: sent disconnected event to nl80211",
                       __func__);
#ifdef WLAN_FEATURE_P2P_DEBUG
            if(pAdapter->device_mode == WLAN_HDD_P2P_CLIENT)
            {
                if(globalP2PConnectionStatus == P2P_CLIENT_CONNECTED_STATE_1)
                {
                    globalP2PConnectionStatus = P2P_CLIENT_DISCONNECTED_STATE;
                    hddLog(VOS_TRACE_LEVEL_ERROR,"[P2P State] 8 way Handshake completed "
                          "and moved to disconnected state");
                }
                else if(globalP2PConnectionStatus == P2P_CLIENT_COMPLETED_STATE)
                {
                    globalP2PConnectionStatus = P2P_NOT_ACTIVE;
                    hddLog(VOS_TRACE_LEVEL_ERROR,"[P2P State] P2P Client is removed "
                          "and moved to inactive state");
                }
            }
#endif
               if ((roamStatus == eCSR_ROAM_LOSTLINK) &&
                    !pRoamInfo->reasonCode)
                   wlan_hdd_get_frame_logs(pAdapter,
                                WLAN_HDD_GET_FRAME_LOG_CMD_SEND_AND_CLEAR);
            /*Only send indication to kernel if not initiated by kernel*/
            if ( sendDisconInd )
            {
               /* To avoid wpa_supplicant sending "HANGED" CMD to ICS UI */
               if ( eCSR_ROAM_LOSTLINK == roamStatus )
               {
                    wlan_hdd_cfg80211_indicate_disconnect(dev, false,
                                                   pRoamInfo->reasonCode);
               }
               else
               {
                   wlan_hdd_cfg80211_indicate_disconnect(dev, false,
                                                   WLAN_REASON_UNSPECIFIED);
               }
            }

            if (pAdapter->device_mode == WLAN_HDD_P2P_CLIENT)
            {
                hddLog(LOG1,
                       FL("P2P client is getting removed and we are tryig to re-enable TDLS"));
                wlan_hdd_tdls_reenable(pHddCtx);
            }

            //If the Device Mode is Station
            // and the P2P Client is Connected
            //Enable BMPS

            // In case of JB, as Change-Iface may or maynot be called for p2p0
            // Enable BMPS/IMPS in case P2P_CLIENT disconnected
            if(VOS_STATUS_SUCCESS == hdd_issta_p2p_clientconnected(pHddCtx))
            {
               //Enable BMPS only of other Session is P2P Client
               hdd_context_t *pHddCtx = NULL;
               v_CONTEXT_t pVosContext = vos_get_global_context( VOS_MODULE_ID_HDD, NULL );

               if (NULL != pVosContext)
               {
                   pHddCtx = vos_get_context( VOS_MODULE_ID_HDD, pVosContext);

                   if(NULL != pHddCtx)
                   {
                       //Only P2P Client is there Enable Bmps back
                       if((0 == pHddCtx->no_of_open_sessions[VOS_STA_SAP_MODE]) &&
                          (0 == pHddCtx->no_of_open_sessions[VOS_P2P_GO_MODE]))
                       {
                           if (pHddCtx->hdd_wlan_suspended)
                           {
                               hdd_set_pwrparams(pHddCtx);
                           }
                           hdd_enable_bmps_imps(pHddCtx);
                      }
                   }
               }
            }
        }
    }

     hdd_wmm_adapter_clear(pAdapter);
     /* Clear PER based roam stats */
#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
     if (sme_IsFeatureSupportedByFW(PER_BASED_ROAMING) &&
         (WLAN_HDD_INFRA_STATION == pAdapter->device_mode) &&
         pHddCtx->cfg_ini && pHddCtx->cfg_ini->isPERRoamEnabled &&
         pHddCtx->cfg_ini->isPERRoamRxPathEnabled)
         sme_unset_per_roam_rxconfig(pHddCtx->hHal);
#endif
#if defined(WLAN_FEATURE_VOWIFI_11R)
     sme_FTReset(WLAN_HDD_GET_HAL_CTX(pAdapter));
#endif

    if (eCSR_ROAM_IBSS_LEAVE == roamStatus)
    {
        v_U8_t i;

        sta_id = IBSS_BROADCAST_STAID;
        vstatus = hdd_ibss_DeregisterSTA( pAdapter, sta_id );
        if ( !VOS_IS_STATUS_SUCCESS(vstatus ) )
        {
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    FL("hdd_roamDeregisterSTA() failed to for staID %d.  "
                     "Status= %d [0x%x]"),
                     sta_id, status, status );

            status = eHAL_STATUS_FAILURE;
        }
        pHddCtx->sta_to_adapter[sta_id] = NULL;

        /*Clear all the peer sta register with TL.*/
        for (i =0; i < HDD_MAX_NUM_IBSS_STA; i++ )
        {
            if (0 != pHddStaCtx->conn_info.staId[i])
            {
               sta_id = pHddStaCtx->conn_info.staId[i];

               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                     FL("Deregister StaID %d"),sta_id);
               vstatus = hdd_ibss_DeregisterSTA( pAdapter, sta_id );
               if ( !VOS_IS_STATUS_SUCCESS(vstatus ) )
               {
                   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     FL("hdd_roamDeregisterSTA() failed to for staID %d.  "
                      "Status= %d [0x%x]"),
                       sta_id, status, status );
                   status = eHAL_STATUS_FAILURE;
               }

               vstatus = hdd_sta_id_hash_remove_entry(pAdapter,
                           sta_id, &pHddStaCtx->conn_info.peerMacAddress[i]);
               if (vstatus != VOS_STATUS_SUCCESS) {
                   hddLog(VOS_TRACE_LEVEL_ERROR,
                             FL("Not able to remove staid hash %d"),
                             sta_id);
                   status = eHAL_STATUS_FAILURE;
               } else {
                   hddLog(VOS_TRACE_LEVEL_INFO,
                         FL("ibss station removed sta_id %d mac:"
                         MAC_ADDRESS_STR), sta_id,
                         MAC_ADDR_ARRAY(pHddStaCtx->conn_info.peerMacAddress[i].bytes));
               }

               /*set the staid and peer mac as 0, all other reset are
                * done in hdd_connRemoveConnectInfo.
                */
               pHddStaCtx->conn_info.staId[i]= 0;
               vos_mem_zero( &pHddStaCtx->conn_info.peerMacAddress[i], sizeof( v_MACADDR_t ) );

               if (sta_id < (WLAN_MAX_STA_COUNT + 3))
                    pHddCtx->sta_to_adapter[sta_id] = NULL;
            }
        }

    }
    else
    {
       sta_id = pHddStaCtx->conn_info.staId[0];
       /* clear scan cache for Link Lost */
       if (pRoamInfo && !pRoamInfo->reasonCode &&
           (eCSR_ROAM_LOSTLINK == roamStatus)) {
           wlan_hdd_cfg80211_update_bss_list(pAdapter,
                               pHddStaCtx->conn_info.bssId);
           sme_remove_bssid_from_scan_list(pHddCtx->hHal,
                               pHddStaCtx->conn_info.bssId);
        }

       //We should clear all sta register with TL, for now, only one.
       vstatus = hdd_roamDeregisterSTA( pAdapter, sta_id );
       if ( !VOS_IS_STATUS_SUCCESS(vstatus ) )
       {
           VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  FL("hdd_roamDeregisterSTA() failed to for staID %d.  "
                  "Status= %d [0x%x]"),
                  sta_id, status, status );

           status = eHAL_STATUS_FAILURE;
       }

       pHddCtx->sta_to_adapter[sta_id] = NULL;
    }

#ifdef WLAN_FEATURE_LINK_LAYER_STATS
    if (VOS_STATUS_SUCCESS !=
            WLANTL_ClearInterfaceStats((WLAN_HDD_GET_CTX(pAdapter))->pvosContext,
                pHddStaCtx->conn_info.staId[0], WIFI_STATS_IFACE_AC))
    {
        hddLog(VOS_TRACE_LEVEL_ERROR, "%s:"
                "WLANTL_ClearInterfaceStats Failed", __func__);
    }
    pAdapter->hdd_stats.hddTxRxStats.txMcast[WLANTL_AC_VO] = 0;
    pAdapter->hdd_stats.hddTxRxStats.txMcast[WLANTL_AC_VI] = 0;
    pAdapter->hdd_stats.hddTxRxStats.txMcast[WLANTL_AC_BE] = 0;
    pAdapter->hdd_stats.hddTxRxStats.txMcast[WLANTL_AC_BK] = 0;
#endif /* WLAN_FEATURE_LINK_LAYER_STATS */
    pAdapter->dad = false;

    // Clear saved connection information in HDD
    hdd_connRemoveConnectInfo( pHddStaCtx );

    /*
     * eConnectionState_Connecting state mean that connection is in progress so
     * no need to set state to eConnectionState_NotConnected
     */
    if ((eConnectionState_Connecting != pHddStaCtx->conn_info.connState)) {
         hddLog(LOG1, FL("Set HDD connState to eConnectionState_NotConnected"));
         hdd_connSetConnectionState(pHddStaCtx, eConnectionState_NotConnected);
    }
#ifdef WLAN_FEATURE_GTK_OFFLOAD
    if ((WLAN_HDD_INFRA_STATION == pAdapter->device_mode) ||
        (WLAN_HDD_P2P_CLIENT == pAdapter->device_mode))
    {
        memset(&pHddStaCtx->gtkOffloadReqParams, 0,
              sizeof (tSirGtkOffloadParams));
        pHddStaCtx->gtkOffloadReqParams.ulFlags = GTK_OFFLOAD_DISABLE;
    }
#endif

#ifdef FEATURE_WLAN_TDLS
    if (eCSR_ROAM_IBSS_LEAVE != roamStatus)
    {
        wlan_hdd_tdls_disconnection_callback(pAdapter);
    }
#endif

    //Unblock anyone waiting for disconnect to complete
    complete(&pAdapter->disconnect_comp_var);
    hdd_print_bss_info(pHddStaCtx);

    hdd_check_and_move_if_sap_is_on_dfs_chan(pHddCtx, pAdapter);
    return( status );
}

static void hdd_postTLPacketPendingInd(hdd_adapter_t *pAdapter,
                                       v_U8_t staId)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    v_SINT_t i;
    v_SIZE_t size;
    VOS_STATUS status;
    v_BOOL_t granted = VOS_FALSE;

    if ((pAdapter->device_mode == WLAN_HDD_INFRA_STATION) ||
        (pAdapter->device_mode == WLAN_HDD_P2P_CLIENT) ||
        (pAdapter->device_mode == WLAN_HDD_P2P_DEVICE))
    {
        //Indicate to TL that there is pending data if a queue is non empty
        for (i = WLANTL_AC_HIGH_PRIO; i>=0; --i)
        {
            size = 0;
            hdd_list_size(&pAdapter->wmm_tx_queue[i], &size);
            if (size > 0)
            {
               if (i != WLANTL_AC_HIGH_PRIO)
               {
                  if (VOS_FALSE ==
                      pAdapter->hddWmmStatus.wmmAcStatus[i].wmmAcAccessAllowed)
                  {
                     hdd_wmm_acquire_access(pAdapter,
                                           (WLANTL_ACEnumType)i, &granted);
                     pAdapter->psbChanged |= (1 << i);
                  }
                  else
                     granted = VOS_TRUE;
               }

               if (granted || (i == WLANTL_AC_HIGH_PRIO))
               {
                  status = WLANTL_STAPktPending(pHddCtx->pvosContext,
                                                staId, (WLANTL_ACEnumType)i);
                  if (!VOS_IS_STATUS_SUCCESS(status))
                  {
                     VOS_TRACE(VOS_MODULE_ID_TL, VOS_TRACE_LEVEL_INFO_HIGH,
                               "%s: Failure in indicating pkt to TL for QID=%d",
                               __func__, i);
                  }
               }
            }
        }
    }
}

static VOS_STATUS hdd_roamRegisterSTA( hdd_adapter_t *pAdapter,
                                       tCsrRoamInfo *pRoamInfo,
                                       v_U8_t staId,
                                       v_MACADDR_t *pPeerMacAddress,
                                       tSirBssDescription *pBssDesc )
{
   VOS_STATUS vosStatus = VOS_STATUS_E_FAILURE;
   WLAN_STADescType staDesc = {0};
   eCsrEncryptionType connectedCipherAlgo;
   v_BOOL_t  fConnected;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   hdd_config_t *cfg_param = pHddCtx->cfg_ini;

   if ( NULL == pBssDesc)
   {
       return VOS_STATUS_E_FAILURE;
   }
   // Get the Station ID from the one saved during the assocation.
   staDesc.ucSTAId = staId;

   staDesc.wSTAType = WLAN_STA_INFRA;

   // grab the bssid from the connection info in the adapter structure and hand that
   // over to TL when registering.
   vos_mem_copy( staDesc.vSTAMACAddress.bytes, pHddStaCtx->conn_info.bssId,
                 sizeof(pHddStaCtx->conn_info.bssId) );

   vos_copy_macaddr( &staDesc.vSelfMACAddress, &pAdapter->macAddressCurrent );

   // set the QoS field appropriately
   if (hdd_wmm_is_active(pAdapter))
   {
      staDesc.ucQosEnabled = 1;
   }
   else
   {
      staDesc.ucQosEnabled = 0;
   }

   fConnected = hdd_connGetConnectedCipherAlgo( pHddStaCtx, &connectedCipherAlgo );
   if ( connectedCipherAlgo != eCSR_ENCRYPT_TYPE_NONE )
   {
      staDesc.ucProtectedFrame = 1;
   }
   else
   {
      staDesc.ucProtectedFrame = 0;

   }

#ifdef FEATURE_WLAN_ESE
   staDesc.ucIsEseSta = pRoamInfo->isESEAssoc;
#endif //FEATURE_WLAN_ESE

#ifdef VOLANS_ENABLE_SW_REPLAY_CHECK
   /* check whether replay check is valid for the station or not */
   if( (eCSR_ENCRYPT_TYPE_TKIP == connectedCipherAlgo) || (eCSR_ENCRYPT_TYPE_AES == connectedCipherAlgo))
   {
       /* Encryption mode is either TKIP or AES
          and replay check is valid for only these
          two encryption modes */
       staDesc.ucIsReplayCheckValid = VOS_TRUE;
       VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                 "HDD register TL ucIsReplayCheckValid %d: Replay check is needed for station", staDesc.ucIsReplayCheckValid);
   }

   else
   {
      /* For other encryption modes replay check is
         not needed */
        staDesc.ucIsReplayCheckValid = VOS_FALSE;
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                 "HDD register TL ucIsReplayCheckValid %d", staDesc.ucIsReplayCheckValid);
   }
#endif

#ifdef FEATURE_WLAN_WAPI
   hddLog(LOG1, "%s: WAPI STA Registered: %d", __func__, pAdapter->wapi_info.fIsWapiSta);
   if (pAdapter->wapi_info.fIsWapiSta)
   {
      staDesc.ucIsWapiSta = 1;
   }
   else
   {
      staDesc.ucIsWapiSta = 0;
   }
#endif /* FEATURE_WLAN_WAPI */

   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                 "HDD register TL Sec_enabled= %d.", staDesc.ucProtectedFrame );

   // UMA is Not ready yet, Xlation will be done by TL
   staDesc.ucSwFrameTXXlation = 1;
   staDesc.ucSwFrameRXXlation = 1;
   staDesc.ucAddRmvLLC = 1;
   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "HDD register TL QoS_enabled=%d",
              staDesc.ucQosEnabled );
   // Initialize signatures and state
   staDesc.ucUcastSig  = pRoamInfo->ucastSig;
   staDesc.ucBcastSig  = pRoamInfo->bcastSig;
   staDesc.ucInitState = pRoamInfo->fAuthRequired ?
      WLANTL_STA_CONNECTED : WLANTL_STA_AUTHENTICATED;
   // Register the Station with TL...
   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "%s: HDD register TL ucInitState=%d", __func__, staDesc.ucInitState );
   vosStatus = WLANTL_RegisterSTAClient( pHddCtx->pvosContext,
                                         hdd_rx_packet_cbk,
                                         hdd_tx_complete_cbk,
                                         hdd_tx_fetch_packet_cbk, &staDesc,
                                         pBssDesc->rssi );

   if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                 "WLANTL_RegisterSTAClient() failed to register.  Status= %d [0x%08X]",
                 vosStatus, vosStatus );
      return vosStatus;
   }

   if ( cfg_param->dynSplitscan &&
      ( VOS_TIMER_STATE_RUNNING !=
                      vos_timer_getCurrentState(&pHddCtx->tx_rx_trafficTmr)))
   {
       vos_timer_start(&pHddCtx->tx_rx_trafficTmr,
                       cfg_param->trafficMntrTmrForSplitScan);
   }

   // if (WPA), tell TL to go to 'connected' and after keys come to the driver,
   // then go to 'authenticated'.  For all other authentication types
   // (those that donot require upper layer authentication) we can put
   // TL directly into 'authenticated' state.
   if (staDesc.wSTAType != WLAN_STA_IBSS)
      VOS_ASSERT( fConnected );

   if ( !pRoamInfo->fAuthRequired )
   {
      // Connections that do not need Upper layer auth, transition TL directly
      // to 'Authenticated' state.
      vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext, staDesc.ucSTAId,
                                         WLANTL_STA_AUTHENTICATED );

      pHddStaCtx->conn_info.uIsAuthenticated = VOS_TRUE;

      hdd_postTLPacketPendingInd(pAdapter, staDesc.ucSTAId);
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                 "ULA auth StaId= %d. Changing TL state to CONNECTED"
                 "at Join time", pHddStaCtx->conn_info.staId[0] );
      vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext, staDesc.ucSTAId,
                                      WLANTL_STA_CONNECTED );
      pHddStaCtx->conn_info.uIsAuthenticated = VOS_FALSE;
   }
   return( vosStatus );
}

static void hdd_SendReAssocEvent(struct net_device *dev, hdd_adapter_t *pAdapter,
    tCsrRoamInfo *pCsrRoamInfo, v_U8_t *reqRsnIe, tANI_U32 reqRsnLength)
{
    unsigned int len = 0;
    u8 *pFTAssocRsp = NULL;
    v_U8_t *rspRsnIe = kmalloc(IW_GENERIC_IE_MAX, GFP_KERNEL);
    tANI_U32 rspRsnLength = 0;
    struct cfg80211_bss* bss;
    tCsrRoamConnectedProfile roam_profile;
    struct ieee80211_channel *chan;
    tHalHandle hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);

    if (!rspRsnIe) {
        hddLog(LOGE, "%s: Unable to allocate RSN IE", __func__);
        return;
    }

    if (pCsrRoamInfo == NULL) {
        hddLog(LOGE, "%s: Invalid CSR roam info", __func__);
        goto done;
    }

    if (pCsrRoamInfo->nAssocRspLength == 0) {
        hddLog(LOGE, "%s: Invalid assoc response length", __func__);
        goto done;
    }

    pFTAssocRsp = (u8 *)(pCsrRoamInfo->pbFrames + pCsrRoamInfo->nBeaconLength +
                    pCsrRoamInfo->nAssocReqLength);
    if (pFTAssocRsp == NULL)
        goto done;

    //pFTAssocRsp needs to point to the IEs
    pFTAssocRsp += FT_ASSOC_RSP_IES_OFFSET;
    hddLog(LOG1, "%s: AssocRsp is now at %02x%02x", __func__,
                    (unsigned int)pFTAssocRsp[0],
                    (unsigned int)pFTAssocRsp[1]);

    // Send the Assoc Resp, the supplicant needs this for initial Auth.
    len = pCsrRoamInfo->nAssocRspLength - FT_ASSOC_RSP_IES_OFFSET;
    rspRsnLength = len;
    memcpy(rspRsnIe, pFTAssocRsp, len);
    memset(rspRsnIe + len, 0, IW_GENERIC_IE_MAX - len);

    chan = ieee80211_get_channel(pAdapter->wdev.wiphy,
             (int) pCsrRoamInfo->pBssDesc->channelId);
    vos_mem_zero(&roam_profile, sizeof(tCsrRoamConnectedProfile));
    sme_RoamGetConnectProfile(hHal, pAdapter->sessionId, &roam_profile);
    bss = hdd_get_bss_entry(pAdapter->wdev.wiphy,
              chan, pCsrRoamInfo->bssid,
              &roam_profile.SSID.ssId[0],
              roam_profile.SSID.length);
    hdd_send_roamed_ind(dev, bss,
                    reqRsnIe, reqRsnLength,
                    rspRsnIe, rspRsnLength);

done:
    kfree(rspRsnIe);
}

void hdd_PerformRoamSetKeyComplete(hdd_adapter_t *pAdapter)
{
    eHalStatus halStatus = eHAL_STATUS_SUCCESS;
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    tCsrRoamInfo roamInfo;
    roamInfo.fAuthRequired = FALSE;
    vos_mem_copy(roamInfo.bssid,
                 pHddStaCtx->roam_info.bssid,
                 WNI_CFG_BSSID_LEN);
    vos_mem_copy(roamInfo.peerMac,
                 pHddStaCtx->roam_info.peerMac,
                 WNI_CFG_BSSID_LEN);

    halStatus = hdd_RoamSetKeyCompleteHandler(pAdapter,
                                  &roamInfo,
                                  pHddStaCtx->roam_info.roamId,
                                  pHddStaCtx->roam_info.roamStatus,
                                  eCSR_ROAM_RESULT_AUTHENTICATED);
    if (halStatus != eHAL_STATUS_SUCCESS)
    {
        hddLog(LOGE, "%s: Set Key complete failure", __func__);
    }
    pHddStaCtx->roam_info.deferKeyComplete = FALSE;
}

/**
 * hdd_schedule_ecsa_chan_change_work() - schedule ecsa chan change work
 * @hal: hal context
 * @sta_session_id: sta session id
 *
 * Return: void.
 */
static void
hdd_schedule_ecsa_chan_change_work(hdd_context_t *hdd_ctx,
                                   uint8_t sta_session_id)
{
   v_TIME_t conn_start_time;
   int32_t time_diff;

   conn_start_time = sme_get_connect_strt_time(hdd_ctx->hHal, sta_session_id);
   time_diff = vos_timer_get_system_time() - conn_start_time;

   if (time_diff < 0)
       time_diff = ECSA_SCC_CHAN_CHANGE_DEFER_INTERVAL;
   else if (time_diff > ECSA_SCC_CHAN_CHANGE_DEFER_INTERVAL)
       time_diff = 0;
   else
       time_diff = ECSA_SCC_CHAN_CHANGE_DEFER_INTERVAL - time_diff;

   hddLog(LOG1, FL("schedule ecsa_chan_change_work after %d ms"), time_diff);
   schedule_delayed_work(&hdd_ctx->ecsa_chan_change_work,
                          msecs_to_jiffies(time_diff));
}

static eHalStatus hdd_AssociationCompletionHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                                                    tANI_U32 roamId, eRoamCmdStatus roamStatus,
                                                    eCsrRoamResult roamResult )
{
    struct net_device *dev = pAdapter->dev;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    hdd_adapter_t *pHostapdAdapter = NULL;
    VOS_STATUS vosStatus = VOS_STATUS_SUCCESS;
    v_U8_t reqRsnIe[DOT11F_IE_RSN_MAX_LEN];
    tANI_U32 reqRsnLength = DOT11F_IE_RSN_MAX_LEN;
#if  defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR) || defined (WLAN_FEATURE_VOWIFI_11R)
    int ft_carrier_on = FALSE;
#endif
    int status;
    v_BOOL_t hddDisconInProgress = FALSE;

    /* HDD has initiated disconnect, do not send connect result indication
     * to kernel as it will be handled by __cfg80211_disconnect.
     */
    if (((eConnectionState_Disconnecting == pHddStaCtx->conn_info.connState) ||
        (eConnectionState_NotConnected == pHddStaCtx->conn_info.connState)) &&
        ((eCSR_ROAM_RESULT_ASSOCIATED == roamResult) ||
        (eCSR_ROAM_ASSOCIATION_FAILURE == roamStatus)))
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
               FL(" Disconnect from HDD in progress "));
        hddDisconInProgress  = TRUE;
    }

    if ( eCSR_ROAM_RESULT_ASSOCIATED == roamResult )
    {
        if ( !hddDisconInProgress )
        {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: Set HDD connState to eConnectionState_Associated",
                   __func__);
             hdd_connSetConnectionState( pHddStaCtx, eConnectionState_Associated );
        }

        pAdapter->maxRateFlags = pRoamInfo->maxRateFlags;
        // Save the connection info from CSR...
        hdd_connSaveConnectInfo( pAdapter, pRoamInfo, eCSR_BSS_TYPE_INFRASTRUCTURE );

#ifdef WLAN_FEATURE_ROAM_SCAN_OFFLOAD
        if (sme_IsFeatureSupportedByFW(PER_BASED_ROAMING) &&
            (WLAN_HDD_INFRA_STATION == pAdapter->device_mode) &&
            !hddDisconInProgress &&
            pHddCtx->cfg_ini && pHddCtx->cfg_ini->isPERRoamEnabled &&
            pHddCtx->cfg_ini->isPERRoamRxPathEnabled)
            sme_set_per_roam_rxconfig(pHddCtx->hHal,
                 pHddStaCtx->conn_info.staId[0],
                 pHddCtx->cfg_ini->rateDownThreshold,
                 pHddCtx->cfg_ini->rateUpThreshold,
                 pHddCtx->cfg_ini->PERroamTriggerPercent,
                 pHddCtx->cfg_ini->PERroamRxPktsThreshold,
                 pHddCtx->cfg_ini->waitPeriodForNextPERScan);
#endif

#ifdef FEATURE_WLAN_WAPI
        if ( pRoamInfo->u.pConnectedProfile->AuthType == eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE ||
                pRoamInfo->u.pConnectedProfile->AuthType == eCSR_AUTH_TYPE_WAPI_WAI_PSK )
        {
            pAdapter->wapi_info.fIsWapiSta = 1;
        }
        else
        {
            pAdapter->wapi_info.fIsWapiSta = 0;
        }
#endif  /* FEATURE_WLAN_WAPI */

        // IKJB42MAIN-1244, Motorola, a19091 - START
        if(pAdapter->device_mode == WLAN_HDD_INFRA_STATION)
            pAdapter->needs_v6_set = eANI_BOOLEAN_TRUE;
        // IKJB42MAIN-1244, Motorola, a19091 - END

        // indicate 'connect' status to userspace
        hdd_SendAssociationEvent(dev,pRoamInfo);


        // Initialize the Linkup event completion variable
        INIT_COMPLETION(pAdapter->linkup_event_var);

        /*
           Sometimes Switching ON the Carrier is taking time to activate the device properly. Before allowing any
           packet to go up to the application, device activation has to be ensured for proper queue mapping by the
           kernel. we have registered net device notifier for device change notification. With this we will come to
           know that the device is getting activated properly.
           */
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
        if (pHddStaCtx->ft_carrier_on == FALSE && !hddDisconInProgress )
        {
#endif
            // Enable Linkup Event Servicing which allows the net device notifier to set the linkup event variable
            pAdapter->isLinkUpSvcNeeded = TRUE;

            // Enable Linkup Event Servicing which allows the net device notifier to set the linkup event variable
            pAdapter->isLinkUpSvcNeeded = TRUE;

            // Switch on the Carrier to activate the device
            netif_carrier_on(dev);

            // Wait for the Link to up to ensure all the queues are set properly by the kernel
            status = wait_for_completion_interruptible_timeout(&pAdapter->linkup_event_var,
                                                   msecs_to_jiffies(ASSOC_LINKUP_TIMEOUT));
            if(!status)
            {
                hddLog(VOS_TRACE_LEVEL_WARN, "%s: Warning:ASSOC_LINKUP_TIMEOUT", __func__);
            }

            // Disable Linkup Event Servicing - no more service required from the net device notifier call
            pAdapter->isLinkUpSvcNeeded = FALSE;
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
        }
        else {
            pHddStaCtx->ft_carrier_on = FALSE;
            ft_carrier_on = TRUE;
        }
#endif
        /* Check for STAID */
        if( (WLAN_MAX_STA_COUNT + 3) > pRoamInfo->staId )
            pHddCtx->sta_to_adapter[pRoamInfo->staId] = pAdapter;
        else
            hddLog(VOS_TRACE_LEVEL_ERROR, "%s: Wrong Staid: %d", __func__, pRoamInfo->staId);

        //For reassoc, the station is already registered, all we need is to change the state
        //of the STA in TL.
        //If authentication is required (WPA/WPA2/DWEP), change TL to CONNECTED instead of AUTHENTICATED
        //pRoamInfo->fReassocReq will be set only for the reassoc to same ap
        if( !pRoamInfo->fReassocReq )
        {
            struct cfg80211_bss *bss;
#ifdef WLAN_FEATURE_VOWIFI_11R
            u8 *pFTAssocRsp = NULL;
            unsigned int assocRsplen = 0;
            u8 *pFTAssocReq = NULL;
            unsigned int assocReqlen = 0;
            struct ieee80211_channel *chan;
#endif
            v_U8_t rspRsnIe[DOT11F_IE_RSN_MAX_LEN];
            tANI_U32 rspRsnLength = DOT11F_IE_RSN_MAX_LEN;

            /* add bss_id to cfg80211 data base */
            bss = wlan_hdd_cfg80211_update_bss_db(pAdapter, pRoamInfo);
            if (NULL == bss)
            {
                hddLog(LOGE,
                     FL("Not able to create BSS entry"));
                netif_carrier_off(dev);
                if (!hddDisconInProgress) {
                   /*
                    * Here driver was not able to update cfg80211 database
                    * this can happen if connected channel is not valid,
                    * i.e reg domain was changed during connection.
                    * Queue disconnect for the session if disconnect is
                    * not in progress.
                    */
                    hddLog(LOGE, FL("Disconnecting..."));
                    sme_RoamDisconnect( WLAN_HDD_GET_HAL_CTX(pAdapter),
                                 pAdapter->sessionId,
                                 eCSR_DISCONNECT_REASON_UNSPECIFIED);
                }
                return eHAL_STATUS_FAILURE;
            }
#ifdef WLAN_FEATURE_VOWIFI_11R
            if(pRoamInfo->u.pConnectedProfile->AuthType == eCSR_AUTH_TYPE_FT_RSN ||
                pRoamInfo->u.pConnectedProfile->AuthType == eCSR_AUTH_TYPE_FT_RSN_PSK )
            {

                //Association Response
                pFTAssocRsp = (u8 *)(pRoamInfo->pbFrames + pRoamInfo->nBeaconLength +
                                    pRoamInfo->nAssocReqLength);
                if (pFTAssocRsp != NULL)
                {
                    // pFTAssocRsp needs to point to the IEs
                    pFTAssocRsp += FT_ASSOC_RSP_IES_OFFSET;
                    hddLog(LOG1, "%s: AssocRsp is now at %02x%02x", __func__,
                                        (unsigned int)pFTAssocRsp[0],
                                        (unsigned int)pFTAssocRsp[1]);
                    assocRsplen = pRoamInfo->nAssocRspLength - FT_ASSOC_RSP_IES_OFFSET;
                }
                else
                {
                    hddLog(LOGE, "%s:AssocRsp is NULL", __func__);
                    assocRsplen = 0;
                }

                //Association Request
                pFTAssocReq = (u8 *)(pRoamInfo->pbFrames +
                                     pRoamInfo->nBeaconLength);
                if (pFTAssocReq != NULL)
                {
                    if(!ft_carrier_on)
                    {
                         // pFTAssocReq needs to point to the IEs
                        pFTAssocReq += FT_ASSOC_REQ_IES_OFFSET;
                        hddLog(LOG1, "%s: pFTAssocReq is now at %02x%02x", __func__,
                                              (unsigned int)pFTAssocReq[0],
                                              (unsigned int)pFTAssocReq[1]);
                        assocReqlen = pRoamInfo->nAssocReqLength - FT_ASSOC_REQ_IES_OFFSET;
                    }
                    else
                    {
                        /* This should contain only the FTIEs */
                        assocReqlen = pRoamInfo->nAssocReqLength;
                    }
                }
                else
                {
                    hddLog(LOGE, "%s:AssocReq is NULL", __func__);
                    assocReqlen = 0;
                }

                if(ft_carrier_on)
                {
                    if ( !hddDisconInProgress )
                    {
                        struct cfg80211_bss *roam_bss;

                        hddLog(LOG1, "%s ft_carrier_on is %d, sending roamed "
                                 "indication", __FUNCTION__, ft_carrier_on);
                        chan = ieee80211_get_channel(pAdapter->wdev.wiphy,
                                         (int)pRoamInfo->pBssDesc->channelId);
                        hddLog(LOG1, "assocReqlen %d assocRsplen %d", assocReqlen,
                                         assocRsplen);
                        if (pHddCtx->cfg_ini &&
                            pHddCtx->cfg_ini->gEnableRoamDelayStats)
                        {
                            vos_record_roam_event(e_HDD_SEND_REASSOC_RSP, NULL, 0);
                        }
                        roam_bss = hdd_get_bss_entry(pAdapter->wdev.wiphy,
                               chan, pRoamInfo->bssid,
                               pRoamInfo->u.pConnectedProfile->SSID.ssId,
                               pRoamInfo->u.pConnectedProfile->SSID.length);
                        hdd_send_roamed_ind(dev, roam_bss,
                               pFTAssocReq, assocReqlen,
                               pFTAssocRsp, assocRsplen);
                    }
                    if (sme_GetFTPTKState(WLAN_HDD_GET_HAL_CTX(pAdapter)))
                    {
                        sme_SetFTPTKState(WLAN_HDD_GET_HAL_CTX(pAdapter), FALSE);
                        pRoamInfo->fAuthRequired = FALSE;

                        vos_mem_copy(pHddStaCtx->roam_info.bssid,
                                     pRoamInfo->bssid,
                                     HDD_MAC_ADDR_LEN);
                        vos_mem_copy(pHddStaCtx->roam_info.peerMac,
                                     pRoamInfo->peerMac,
                                     HDD_MAC_ADDR_LEN);
                        pHddStaCtx->roam_info.roamId = roamId;
                        pHddStaCtx->roam_info.roamStatus = roamStatus;
                        pHddStaCtx->roam_info.deferKeyComplete = TRUE;
                    }
                }
                else if ( !hddDisconInProgress )
                {
                    hddLog(LOG1, "%s ft_carrier_on is %d, sending connect "
                                 "indication", __FUNCTION__, ft_carrier_on);
                    hdd_connect_result(dev, pRoamInfo->bssid, pRoamInfo,
                                       pFTAssocReq, assocReqlen,
                                       pFTAssocRsp, assocRsplen,
                                       WLAN_STATUS_SUCCESS,
                                       GFP_KERNEL);
                }
            }
            else
#endif
            {
                /* wpa supplicant expecting WPA/RSN IE in connect result */
                csrRoamGetWpaRsnReqIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                        pAdapter->sessionId,
                        &reqRsnLength,
                        reqRsnIe);

                csrRoamGetWpaRsnRspIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                        pAdapter->sessionId,
                        &rspRsnLength,
                        rspRsnIe);
                if ( !hddDisconInProgress )
                {
#if  defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
                    if(ft_carrier_on)
                    {
                        hdd_SendReAssocEvent(dev, pAdapter, pRoamInfo, reqRsnIe, reqRsnLength);
                        if (pHddCtx->cfg_ini &&
                            pHddCtx->cfg_ini->gEnableRoamDelayStats)
                        {
                            vos_record_roam_event(e_HDD_SEND_REASSOC_RSP, NULL, 0);
                        }
                    }
                    else
#endif /* FEATURE_WLAN_ESE */

                    {
                         hddLog(VOS_TRACE_LEVEL_INFO,
                            "%s: sending connect indication to nl80211:"
                            " for bssid " MAC_ADDRESS_STR
                            " result:%d and Status:%d",
                            __func__, MAC_ADDR_ARRAY(pRoamInfo->bssid),
                            roamResult, roamStatus);
                         /* inform connect result to nl80211 */
                         hdd_connect_result(dev, pRoamInfo->bssid,
                                pRoamInfo,
                                reqRsnIe, reqRsnLength,
                                rspRsnIe, rspRsnLength,
                                WLAN_STATUS_SUCCESS,
                                GFP_KERNEL);
                    }
                }
            }
            if ( !hddDisconInProgress )
            {
                cfg80211_put_bss(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
                           pHddCtx->wiphy,
#endif
                             bss);
                // Register the Station with TL after associated...
                vosStatus = hdd_roamRegisterSTA( pAdapter,
                    pRoamInfo,
                    pHddStaCtx->conn_info.staId[ 0 ],
                    NULL,
                    pRoamInfo->pBssDesc );
            }
        }
        else
        {
            /* wpa supplicant expecting WPA/RSN IE in connect result */
            /*  in case of reassociation also need to indicate it to supplicant */
            csrRoamGetWpaRsnReqIE(WLAN_HDD_GET_HAL_CTX(pAdapter),
                    pAdapter->sessionId,
                    &reqRsnLength,
                    reqRsnIe);

            hdd_SendReAssocEvent(dev, pAdapter, pRoamInfo, reqRsnIe, reqRsnLength);
            //Reassoc successfully
            if( pRoamInfo->fAuthRequired )
            {
                vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext,
                                                   pHddStaCtx->conn_info.staId[ 0 ],
                                                   WLANTL_STA_CONNECTED );
                pHddStaCtx->conn_info.uIsAuthenticated = VOS_FALSE;
            }
            else
            {
                VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                          "%s: staId: %d Changing TL state to AUTHENTICATED",
                          __func__, pHddStaCtx->conn_info.staId[ 0 ] );
                vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext,
                                                   pHddStaCtx->conn_info.staId[ 0 ],
                                                   WLANTL_STA_AUTHENTICATED );
                pHddStaCtx->conn_info.uIsAuthenticated = VOS_TRUE;
                hdd_postTLPacketPendingInd(pAdapter,
                                           pHddStaCtx->conn_info.staId[0]);
            }
        }

        if ( VOS_IS_STATUS_SUCCESS( vosStatus ) )
        {
            // perform any WMM-related association processing
            hdd_wmm_assoc(pAdapter, pRoamInfo, eCSR_BSS_TYPE_INFRASTRUCTURE);
        }
        else
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                    "Cannot register STA with TL.  Failed with vosStatus = %d [%08X]",
                    vosStatus, vosStatus );
        }
#ifdef WLAN_FEATURE_11W
        vos_mem_zero( &pAdapter->hdd_stats.hddPmfStats,
                      sizeof(pAdapter->hdd_stats.hddPmfStats) );
#endif

        // Start the Queue
        if ( !hddDisconInProgress )
        {
            hddLog(VOS_TRACE_LEVEL_INFO, FL("Enabling queues"));
            netif_tx_wake_all_queues(dev);
        }
        if (pHddCtx->cfg_ini && pHddCtx->cfg_ini->gEnableRoamDelayStats)
        {
            vos_record_roam_event(e_HDD_ENABLE_TX_QUEUE, NULL, 0);
        }
#ifdef FEATURE_WLAN_TDLS
        wlan_hdd_tdls_connection_callback(pAdapter);
#endif
    }
    else
    {
        hdd_context_t* pHddCtx = (hdd_context_t*)pAdapter->pHddCtx;

        hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
        if (pRoamInfo)
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    "wlan: connection failed with " MAC_ADDRESS_STR " result:%d and Status:%d",
                    MAC_ADDR_ARRAY(pRoamInfo->bssid), roamResult, roamStatus);
        else
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "wlan: connection failed with " MAC_ADDRESS_STR " result:%d and Status:%d",
                    MAC_ADDR_ARRAY(pWextState->req_bssId),
                    roamResult, roamStatus);
        if (!pHddStaCtx->get_mgmt_log_sent) {
            pHddStaCtx->get_mgmt_log_sent = TRUE;
            wlan_hdd_get_frame_logs(pAdapter,
                                WLAN_HDD_GET_FRAME_LOG_CMD_SEND_AND_CLEAR);
        }

        if ((eCSR_ROAM_RESULT_SCAN_FOR_SSID_FAILURE == roamResult) ||
           (pRoamInfo &&
           ((eSIR_SME_JOIN_TIMEOUT_RESULT_CODE == pRoamInfo->statusCode) ||
           (eSIR_SME_AUTH_TIMEOUT_RESULT_CODE == pRoamInfo->statusCode) ||
           (eSIR_SME_ASSOC_TIMEOUT_RESULT_CODE == pRoamInfo->statusCode)))) {
             wlan_hdd_cfg80211_update_bss_list(pAdapter,
                    pRoamInfo ? pRoamInfo->bssid : pWextState->req_bssId);
             sme_remove_bssid_from_scan_list(pHddCtx->hHal,
                    pRoamInfo ? pRoamInfo->bssid : pWextState->req_bssId);
        }
        /* Set connection state to eConnectionState_NotConnected only when CSR
         * has completed operation - with a ASSOCIATION_FAILURE status
         */
        if ( eCSR_ROAM_ASSOCIATION_FAILURE == roamStatus &&  !hddDisconInProgress )
        {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: Set HDD connState to eConnectionState_NotConnected",
                   __func__);
            hdd_connSetConnectionState( pHddStaCtx, eConnectionState_NotConnected);
        }
        if((pHddCtx->concurrency_mode <= 1) &&
           (pHddCtx->no_of_open_sessions[WLAN_HDD_INFRA_STATION] <=1))
        {
            pHddCtx->isAmpAllowed = VOS_TRUE;
        }

        //If the Device Mode is Station
        // and the P2P Client is Connected
        //Enable BMPS

        // In case of JB, as Change-Iface may or maynot be called for p2p0
        // Enable BMPS/IMPS in case P2P_CLIENT disconnected
        if(((WLAN_HDD_INFRA_STATION == pAdapter->device_mode) ||
            (WLAN_HDD_P2P_CLIENT == pAdapter->device_mode)) &&
            (vos_concurrent_open_sessions_running()))
        {
           //Enable BMPS only of other Session is P2P Client
           hdd_context_t *pHddCtx = NULL;
           v_CONTEXT_t pVosContext = vos_get_global_context( VOS_MODULE_ID_HDD, NULL );

           if (NULL != pVosContext)
           {
               pHddCtx = vos_get_context( VOS_MODULE_ID_HDD, pVosContext);

               if(NULL != pHddCtx)
               {
                    //Only P2P Client is there Enable Bmps back
                    if((0 == pHddCtx->no_of_open_sessions[VOS_STA_SAP_MODE]) &&
                       (0 == pHddCtx->no_of_open_sessions[VOS_P2P_GO_MODE]))
                    {
                         if (pHddCtx->hdd_wlan_suspended)
                         {
                             hdd_set_pwrparams(pHddCtx);
                         }
                         hdd_enable_bmps_imps(pHddCtx);
                    }
               }
           }
        }

        /* CR465478: Only send up a connection failure result when CSR has
         * completed operation - with a ASSOCIATION_FAILURE status.*/
        if ( eCSR_ROAM_ASSOCIATION_FAILURE == roamStatus &&  !hddDisconInProgress )
        {

            if (pAdapter->device_mode == WLAN_HDD_P2P_CLIENT)
            {
                hddLog(LOG1,
                       FL("Assoication Failure for P2P client and we are trying to re-enable TDLS"));
                wlan_hdd_tdls_reenable(pHddCtx);
            }

            if (pRoamInfo) {
                hddLog(VOS_TRACE_LEVEL_ERROR,
                     "%s: send connect failure to nl80211:"
                     " for bssid " MAC_ADDRESS_STR
                     " result:%d and Status:%d reasonCode %d" ,
                     __func__, MAC_ADDR_ARRAY(pRoamInfo->bssid),
                     roamResult, roamStatus, pRoamInfo->reasonCode);
                pHddStaCtx->conn_info.assoc_status_code =
                    pRoamInfo->statusCode;
             } else {
                 hddLog(VOS_TRACE_LEVEL_ERROR,
                     "%s: connect failed:"
                     " for bssid " MAC_ADDRESS_STR
                     " result:%d and Status:%d" ,
                     __func__, MAC_ADDR_ARRAY(pWextState->req_bssId),
                     roamResult, roamStatus);
             }

            /* inform association failure event to nl80211 */
            if ( eCSR_ROAM_RESULT_ASSOC_FAIL_CON_CHANNEL == roamResult )
            {
               if (pRoamInfo)
                   hdd_connect_result(dev, pRoamInfo->bssid, NULL,
                        NULL, 0, NULL, 0,
                        WLAN_STATUS_ASSOC_DENIED_UNSPEC,
                        GFP_KERNEL);
               else
                   hdd_connect_result(dev, pWextState->req_bssId, NULL,
                        NULL, 0, NULL, 0,
                        WLAN_STATUS_ASSOC_DENIED_UNSPEC,
                        GFP_KERNEL);
            }
            else
            {
                if (pRoamInfo)
                    hdd_connect_result(dev, pRoamInfo->bssid, NULL,
                        NULL, 0, NULL, 0,
                        pRoamInfo->reasonCode ?
                        pRoamInfo->reasonCode :
                        WLAN_STATUS_UNSPECIFIED_FAILURE,
                        GFP_KERNEL);
                else
                    hdd_connect_result(dev, pWextState->req_bssId, NULL,
                        NULL, 0, NULL, 0,
                        WLAN_STATUS_UNSPECIFIED_FAILURE,
                        GFP_KERNEL);
            }
            /*Clear the roam profile*/
            hdd_clearRoamProfileIe( pAdapter );
        }

        hdd_wmm_init( pAdapter );

        if (pRoamInfo)
        {
            WLANTL_AssocFailed(pRoamInfo->staId);
        }

        hddLog(VOS_TRACE_LEVEL_INFO, FL("Disabling queues"));
        netif_tx_disable(dev);
        netif_carrier_off(dev);

    }

    if (eCSR_ROAM_RESULT_ASSOCIATED == roamResult)
    {
        pHostapdAdapter = hdd_get_adapter(pHddCtx, WLAN_HDD_SOFTAP);
        if (pHostapdAdapter != NULL)
        {
             /* Restart SAP if its operating channel is different
              * from AP channel.
              */
             if (pHostapdAdapter->sessionCtx.ap.operatingChannel !=
                (int)pRoamInfo->pBssDesc->channelId)
             {
                hddLog(VOS_TRACE_LEVEL_INFO,"Restart Sap as SAP channel is %d "
                       "and STA channel is %d", pHostapdAdapter->sessionCtx.ap.operatingChannel,
                       (int)pRoamInfo->pBssDesc->channelId);
                if (pHddCtx->cfg_ini && pHddCtx->cfg_ini->force_scc_with_ecsa)
                {
                    hdd_schedule_ecsa_chan_change_work(pHddCtx,
                                                       pAdapter->sessionId);
                }
                else
                {
                    hdd_hostapd_stop(pHostapdAdapter->dev);
                    if (pHddCtx->cfg_ini &&
                        pHddCtx->cfg_ini->enable_sap_auth_offload)
                       hdd_force_scc_restart_sap(pHostapdAdapter,
                             pHddCtx, (int)pRoamInfo->pBssDesc->channelId);
                }

             }
        }
    }
    else if (roamStatus == eCSR_ROAM_ASSOCIATION_FAILURE)
    {
        hdd_check_and_move_if_sap_is_on_dfs_chan(pHddCtx, pAdapter);
    }
    return eHAL_STATUS_SUCCESS;
}

/**============================================================================
 *
  @brief hdd_RoamIbssIndicationHandler() - Here we update the status of the
  Ibss when we receive information that we have started/joined an ibss session

  ===========================================================================*/
static void hdd_RoamIbssIndicationHandler( hdd_adapter_t *pAdapter,
                                           tCsrRoamInfo *pRoamInfo,
                                           tANI_U32 roamId,
                                           eRoamCmdStatus roamStatus,
                                           eCsrRoamResult roamResult )
{
   hdd_context_t *pHddCtx = (hdd_context_t*)pAdapter->pHddCtx;
   v_MACADDR_t broadcastMacAddr = VOS_MAC_ADDR_BROADCAST_INITIALIZER;
   struct cfg80211_bss *bss;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   hddLog(VOS_TRACE_LEVEL_INFO, "%s: %s: id %d, status %d, result %d",
          __func__, pAdapter->dev->name, roamId, roamStatus, roamResult);

   switch( roamResult )
   {
      // both IBSS Started and IBSS Join should come in here.
      case eCSR_ROAM_RESULT_IBSS_STARTED:
      case eCSR_ROAM_RESULT_IBSS_JOIN_SUCCESS:
      case eCSR_ROAM_RESULT_IBSS_COALESCED:
      {
         if (NULL == pRoamInfo)
         {
            VOS_ASSERT(0);
            return;
         }

         /* When IBSS Started comes from CSR, we need to move
          * connection state to IBSS Disconnected (meaning no peers
          * are in the IBSS).
          */
         VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: Set HDD connState to eConnectionState_IbssDisconnected",
                   __func__);
         hdd_connSetConnectionState( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter),
                                     eConnectionState_IbssDisconnected );
         /*notify wmm */
         hdd_wmm_connect(pAdapter, pRoamInfo, eCSR_BSS_TYPE_IBSS);
         pHddCtx->sta_to_adapter[IBSS_BROADCAST_STAID] = pAdapter;

         if (pRoamInfo->pBssDesc)
         {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0))
             struct ieee80211_channel *chan;
             int chan_no;
             unsigned int freq;
#endif
             hdd_ibss_RegisterSTA (pAdapter, pRoamInfo,
                          IBSS_BROADCAST_STAID,
                          &broadcastMacAddr, pRoamInfo->pBssDesc);

            /* we created the IBSS, notify supplicant */
            hddLog(VOS_TRACE_LEVEL_INFO, "%s: %s: created ibss "
                   MAC_ADDRESS_STR,
                   __func__, pAdapter->dev->name,
                   MAC_ADDR_ARRAY(pRoamInfo->pBssDesc->bssId));

            /* we must first give cfg80211 the BSS information */
            bss = wlan_hdd_cfg80211_update_bss_db(pAdapter, pRoamInfo);
            if (NULL == bss)
            {
               hddLog(VOS_TRACE_LEVEL_ERROR,
                      "%s: %s: unable to create IBSS entry",
                      __func__, pAdapter->dev->name);
               return;
            }
#ifdef WLAN_FEATURE_RMC
            netif_carrier_on(pAdapter->dev);
            hddLog(VOS_TRACE_LEVEL_INFO, FL("Enabling queues"));
            netif_tx_start_all_queues(pAdapter->dev);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0))
            chan_no = pRoamInfo->pBssDesc->channelId;

            if (chan_no <= 14)
                freq = ieee80211_channel_to_frequency(chan_no,
                                                      HDD_NL80211_BAND_2GHZ);
            else
                freq = ieee80211_channel_to_frequency(chan_no,
                                                      HDD_NL80211_BAND_5GHZ);

            chan = ieee80211_get_channel(pAdapter->wdev.wiphy, freq);

            if (chan)
                cfg80211_ibss_joined(pAdapter->dev, bss->bssid,
                                     chan, GFP_KERNEL);
            else
                hddLog(LOGE, FL("%s: chanId: %d, can't find channel"),
                       pAdapter->dev->name,
                       (int)pRoamInfo->pBssDesc->channelId);
#else
            cfg80211_ibss_joined(pAdapter->dev, bss->bssid, GFP_KERNEL);
#endif
            cfg80211_put_bss(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
                             pHddCtx->wiphy,
#endif
                             bss);
         }
         else
         {
             VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                       "%s: NULL Bss Desc",__func__);
         }

         /* Set Broadcast key again in case IBSS_COALESCED as DEL BSS,
          * in IBSS_COALESCED will remove the BC key.
          */
         if ((eCSR_ROAM_RESULT_IBSS_COALESCED == roamResult) &&
             ( eCSR_ENCRYPT_TYPE_WEP40_STATICKEY
                                           == pHddStaCtx->ibss_enc_key.encType
               ||eCSR_ENCRYPT_TYPE_WEP104_STATICKEY
                                           == pHddStaCtx->ibss_enc_key.encType
               ||eCSR_ENCRYPT_TYPE_TKIP == pHddStaCtx->ibss_enc_key.encType
               ||eCSR_ENCRYPT_TYPE_AES == pHddStaCtx->ibss_enc_key.encType ))
         {
             u8 grpmacaddr[WNI_CFG_BSSID_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
             VOS_STATUS vosStatus;

             pHddStaCtx->ibss_enc_key.keyDirection = eSIR_TX_RX;

             memcpy(&pHddStaCtx->ibss_enc_key.peerMac,
                            grpmacaddr, WNI_CFG_BSSID_LEN);
             hddLog(VOS_TRACE_LEVEL_INFO,
                       FL(" SET GTK in case of COALESCED"));
             vosStatus = sme_RoamSetKey( WLAN_HDD_GET_HAL_CTX(pAdapter),
                      pAdapter->sessionId, &pHddStaCtx->ibss_enc_key, &roamId );
             if ( VOS_STATUS_SUCCESS != vosStatus )
             {
                hddLog(VOS_TRACE_LEVEL_ERROR,
                       FL("sme_RoamSetKey failed, returned %d"),vosStatus);
             }
         }
         break;
      }

      case eCSR_ROAM_RESULT_IBSS_START_FAILED:
      {
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: %s: unable to create IBSS",
                __func__, pAdapter->dev->name);
         break;
      }

      default:
         hddLog(VOS_TRACE_LEVEL_ERROR, "%s: %s: unexpected result %d",
                __func__, pAdapter->dev->name, (int)roamResult);
         break;
   }

   return;
}

/**============================================================================
 *
  @brief roamSaveIbssStation() - Save the IBSS peer MAC address in the adapter.
  This information is passed to iwconfig later. The peer that joined
  last is passed as information to iwconfig.
  If we add HDD_MAX_NUM_IBSS_STA or less STA we return success else we
  return FALSE.

  ===========================================================================*/
static int roamSaveIbssStation(hdd_adapter_t *pAdapter, v_U8_t staId, v_MACADDR_t *peerMacAddress)
{
   int fSuccess = FALSE;
   int idx = 0;
   VOS_STATUS status;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   for ( idx = 0; idx < HDD_MAX_NUM_IBSS_STA; idx++ )
   {
      if ( 0 == pHddStaCtx->conn_info.staId[ idx ] )
      {
         pHddStaCtx->conn_info.staId[ idx ] = staId;

         vos_copy_macaddr( &pHddStaCtx->conn_info.peerMacAddress[ idx ], peerMacAddress );

         fSuccess = TRUE;
         break;
      }
   }

   status = hdd_sta_id_hash_add_entry(pAdapter, staId, peerMacAddress);
   if (status != VOS_STATUS_SUCCESS) {
       hddLog(VOS_TRACE_LEVEL_ERROR,
                 FL("Not able to add staid hash %d"), staId);
       return FALSE;
   }

   hddLog(VOS_TRACE_LEVEL_INFO,
             FL("New station added sta_id %d mac:"
             MAC_ADDRESS_STR), staId,
             MAC_ADDR_ARRAY(peerMacAddress->bytes));

   return( fSuccess );
}
/**============================================================================
 *
  @brief roamRemoveIbssStation() - Remove the IBSS peer MAC address in the adapter.
  If we remove HDD_MAX_NUM_IBSS_STA or less STA we return success else we
  return FALSE.

  ===========================================================================*/
static int roamRemoveIbssStation( hdd_adapter_t *pAdapter, v_U8_t staId )
{
   int fSuccess = FALSE;
   int idx = 0;
   v_U8_t  valid_idx   = 0;
   v_U8_t  del_idx   = 0;
   v_U8_t  empty_slots = 0;
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   VOS_STATUS status;

   for ( idx = 0; idx < HDD_MAX_NUM_IBSS_STA; idx++ )
   {
      if ( staId == pHddStaCtx->conn_info.staId[ idx ] )
      {
         pHddStaCtx->conn_info.staId[ idx ] = 0;
         status = hdd_sta_id_hash_remove_entry(pAdapter,
                  staId, &pHddStaCtx->conn_info.peerMacAddress[idx]);
         if (status != VOS_STATUS_SUCCESS) {
             hddLog(VOS_TRACE_LEVEL_ERROR,
                      FL("Not able to remove staid hash %d"), staId );
             fSuccess = FALSE;
         } else {
             hddLog(VOS_TRACE_LEVEL_INFO,
                   FL("station removed sta_id %d mac:"
                   MAC_ADDRESS_STR), staId,
                   MAC_ADDR_ARRAY(pHddStaCtx->conn_info.peerMacAddress[idx].bytes));

             vos_zero_macaddr( &pHddStaCtx->conn_info.peerMacAddress[ idx ] );

             fSuccess = TRUE;
             // Note the deleted Index, if its 0 we need special handling
             del_idx = idx;
             empty_slots++;
         }
      }
      else
      {
         if (pHddStaCtx->conn_info.staId[idx] != 0)
         {
            valid_idx = idx;
         }
         else
         {
            // Found an empty slot
            empty_slots++;
         }
      }
   }

   if (HDD_MAX_NUM_IBSS_STA == empty_slots)
   {
      // Last peer departed, set the IBSS state appropriately
      pHddStaCtx->conn_info.connState = eConnectionState_IbssDisconnected;
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "Last IBSS Peer Departed!!!" );
   }

   // Find next active staId, to have a valid sta trigger for TL.
   if (fSuccess == TRUE)
   {
      if (del_idx == 0)
      {
         if (pHddStaCtx->conn_info.staId[valid_idx] != 0)
         {
            pHddStaCtx->conn_info.staId[0] = pHddStaCtx->conn_info.staId[valid_idx];
            vos_copy_macaddr( &pHddStaCtx->conn_info.peerMacAddress[ 0 ],
               &pHddStaCtx->conn_info.peerMacAddress[ valid_idx ]);

            pHddStaCtx->conn_info.staId[valid_idx] = 0;
            vos_zero_macaddr( &pHddStaCtx->conn_info.peerMacAddress[ valid_idx ] );
         }
      }
   }
   return( fSuccess );
}

/**============================================================================
 *
  @brief roamIbssConnectHandler() : We update the status of the IBSS to
  connected in this function.

  ===========================================================================*/
static eHalStatus roamIbssConnectHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo )
{
   struct cfg80211_bss *bss;
   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: IBSS Connect Indication from SME!!! "
                   "Set HDD connState to eConnectionState_IbssConnected",
                   __func__);
   // Set the internal connection state to show 'IBSS Connected' (IBSS with a partner stations)...
   hdd_connSetConnectionState( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter), eConnectionState_IbssConnected );

   // Save the connection info from CSR...
   hdd_connSaveConnectInfo( pAdapter, pRoamInfo, eCSR_BSS_TYPE_IBSS );

   // Send the bssid address to the wext.
   hdd_SendAssociationEvent(pAdapter->dev, pRoamInfo);
   /* add bss_id to cfg80211 data base */
   bss = wlan_hdd_cfg80211_update_bss_db(pAdapter, pRoamInfo);
   if (NULL == bss)
   {
      hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: %s: unable to create IBSS entry",
             __func__, pAdapter->dev->name);
      return eHAL_STATUS_FAILURE;
   }
   cfg80211_put_bss(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
                    WLAN_HDD_GET_CTX(pAdapter)->wiphy,
#endif
                    bss);

   return( eHAL_STATUS_SUCCESS );
}

/**============================================================================
 *
  @brief hdd_RoamSetKeyCompleteHandler() - Update the security parameters.

  ===========================================================================*/
static eHalStatus hdd_RoamSetKeyCompleteHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                                                 tANI_U32 roamId, eRoamCmdStatus roamStatus,
                                                 eCsrRoamResult roamResult )
{
   eCsrEncryptionType connectedCipherAlgo;
   v_BOOL_t fConnected   = FALSE;
   VOS_STATUS vosStatus    = VOS_STATUS_E_FAILURE;
   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   WLANTL_STAStateType prevTLState = WLANTL_STA_INIT;
   ENTER();

   if (NULL == pRoamInfo)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "pRoamInfo is NULL");
       return eHAL_STATUS_FAILURE;
   }
   // if ( WPA ), tell TL to go to 'authenticated' after the keys are set.
   // then go to 'authenticated'.  For all other authentication types (those that do
   // not require upper layer authentication) we can put TL directly into 'authenticated'
   // state.
   VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
       "Set Key completion roamStatus =%d roamResult=%d " MAC_ADDRESS_STR,
       roamStatus, roamResult, MAC_ADDR_ARRAY(pRoamInfo->peerMac));

   fConnected = hdd_connGetConnectedCipherAlgo( pHddStaCtx, &connectedCipherAlgo );
   if( fConnected )
   {
      if ( WLAN_HDD_IBSS == pAdapter->device_mode )
      {
         v_U8_t staId;

         v_MACADDR_t broadcastMacAddr = VOS_MAC_ADDR_BROADCAST_INITIALIZER;

         if ( 0 == memcmp( pRoamInfo->peerMac,
                      &broadcastMacAddr, VOS_MAC_ADDR_SIZE ) )
         {
            vosStatus = WLANTL_STAPtkInstalled( pHddCtx->pvosContext,
                                                IBSS_BROADCAST_STAID);
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                "WLAN TL STA GTK Installed for STAID=%d", IBSS_BROADCAST_STAID);
            pHddStaCtx->roam_info.roamingState = HDD_ROAM_STATE_NONE;
         }
         else
         {
            vosStatus = hdd_Ibss_GetStaId(pHddStaCtx,
                              (v_MACADDR_t*)pRoamInfo->peerMac,
                              &staId);
            if ( VOS_STATUS_SUCCESS == vosStatus )
            {
               VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
                "WLAN TL STA Ptk Installed for STAID=%d", staId);
               vosStatus = WLANTL_STAPtkInstalled( pHddCtx->pvosContext,
                                                  staId);
               pHddStaCtx->roam_info.roamingState = HDD_ROAM_STATE_NONE;
            }
         }
      }
      else
      {
            WLANTL_GetSTAState(pHddCtx->pvosContext,
                               pHddStaCtx->conn_info.staId[0],
                               &prevTLState);
         // TODO: Considering getting a state machine in HDD later.
         // This routine is invoked twice. 1)set PTK 2)set GTK.
         // The folloing if statement will be TRUE when setting GTK.
         // At this time we don't handle the state in detail.
         // Related CR: 174048 - TL not in authenticated state
         if ( ( eCSR_ROAM_RESULT_AUTHENTICATED == roamResult ) &&
             (pRoamInfo != NULL) && !pRoamInfo->fAuthRequired )
         {

            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED, "Key set "
                      "for StaId=%d. Changing TL state to AUTHENTICATED from"
                      " state:%d", pHddStaCtx->conn_info.staId[0], prevTLState);

            // Connections that do not need Upper layer authentication,
            // transition TL to 'Authenticated' state after the keys are set.
            vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext,
                                               pHddStaCtx->conn_info.staId[ 0 ],
                                               WLANTL_STA_AUTHENTICATED );

            pHddStaCtx->conn_info.uIsAuthenticated = VOS_TRUE;

            if (WLANTL_STA_AUTHENTICATED != prevTLState)
                hdd_postTLPacketPendingInd(pAdapter,
                                           pHddStaCtx->conn_info.staId[0]);
            //Need to call offload because when roaming happen at that time fwr
            //clean offload info as part of the DelBss
            // No need to configure offload if host was not suspended
            spin_lock(&pHddCtx->filter_lock);
            if(pHddCtx->hdd_wlan_suspended)
            {
                spin_unlock(&pHddCtx->filter_lock);
                hdd_ReConfigSuspendDataClearedDuringRoaming(pHddCtx);
            }
            else
            {
                spin_unlock(&pHddCtx->filter_lock);
            }
            if (pHddCtx->cfg_ini->gEnableRoamDelayStats)
            {
                vos_record_roam_event(e_HDD_SET_GTK_RSP, NULL, 0);
            }
         }
         else
         {
            vosStatus = WLANTL_STAPtkInstalled( pHddCtx->pvosContext,
                                                pHddStaCtx->conn_info.staId[ 0 ]);

            /* In case of  OSEN move TL to 'Authenticated' after PTK is set */
            if (pWextState->roamProfile.bOSENAssociation == VOS_TRUE)
            {
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED, "PTK set"
                      " for StaId=%d. Due to OSEN, Changing TL state to"
                      "AUTHENTICATED from state:%d",
                      pHddStaCtx->conn_info.staId[0], prevTLState);

                vosStatus = WLANTL_ChangeSTAState( pHddCtx->pvosContext,
                                               pHddStaCtx->conn_info.staId[ 0 ],
                                               WLANTL_STA_AUTHENTICATED );

                pHddStaCtx->conn_info.uIsAuthenticated = VOS_TRUE;

                if (WLANTL_STA_AUTHENTICATED != prevTLState)
                    hdd_postTLPacketPendingInd(pAdapter,
                                               pHddStaCtx->conn_info.staId[0]);
            }



            if (pHddCtx->cfg_ini->gEnableRoamDelayStats)
            {
                vos_record_roam_event(e_HDD_SET_PTK_RSP, (void *)pRoamInfo->peerMac, 6);
            }
         }

         pHddStaCtx->roam_info.roamingState = HDD_ROAM_STATE_NONE;
      }
   }
   else
   {
      // possible disassoc after issuing set key and waiting set key complete
      pHddStaCtx->roam_info.roamingState = HDD_ROAM_STATE_NONE;
   }

   EXIT();
   return( eHAL_STATUS_SUCCESS );
}
/**============================================================================
 *
  @brief hdd_RoamMicErrorIndicationHandler() - This function indicates the Mic failure to the supplicant.
  ===========================================================================*/
static eHalStatus hdd_RoamMicErrorIndicationHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
                                                 tANI_U32 roamId, eRoamCmdStatus roamStatus,                                                                              eCsrRoamResult roamResult )
{
   hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

   if( eConnectionState_Associated == pHddStaCtx->conn_info.connState &&
      TKIP_COUNTER_MEASURE_STOPED == pHddStaCtx->WextState.mTKIPCounterMeasures )
   {
      struct iw_michaelmicfailure msg;
      union iwreq_data wreq;
      memset(&msg, '\0', sizeof(msg));
      msg.src_addr.sa_family = ARPHRD_ETHER;
      memcpy(msg.src_addr.sa_data, pRoamInfo->u.pMICFailureInfo->taMacAddr, sizeof(pRoamInfo->u.pMICFailureInfo->taMacAddr));
      hddLog(LOG1, "MIC MAC " MAC_ADDRESS_STR,
             MAC_ADDR_ARRAY(msg.src_addr.sa_data));

      if(pRoamInfo->u.pMICFailureInfo->multicast == eSIR_TRUE)
         msg.flags = IW_MICFAILURE_GROUP;
      else
         msg.flags = IW_MICFAILURE_PAIRWISE;
      memset(&wreq, 0, sizeof(wreq));
      wreq.data.length = sizeof(msg);
      wireless_send_event(pAdapter->dev, IWEVMICHAELMICFAILURE, &wreq, (char *)&msg);
      /* inform mic failure to nl80211 */
      cfg80211_michael_mic_failure(pAdapter->dev,
              pRoamInfo->u.pMICFailureInfo->taMacAddr,
              ((pRoamInfo->u.pMICFailureInfo->multicast == eSIR_TRUE) ?
               NL80211_KEYTYPE_GROUP :
               NL80211_KEYTYPE_PAIRWISE),
              pRoamInfo->u.pMICFailureInfo->keyId,
              pRoamInfo->u.pMICFailureInfo->TSC,
              GFP_KERNEL);

   }

   return( eHAL_STATUS_SUCCESS );
}

/**============================================================================
 *
  @brief roamRoamConnectStatusUpdateHandler() - The Ibss connection status is
  updated regularly here in this function.

  ===========================================================================*/
static eHalStatus roamRoamConnectStatusUpdateHandler( hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo,
   tANI_U32 roamId, eRoamCmdStatus roamStatus,
   eCsrRoamResult roamResult )
{
   VOS_STATUS vosStatus;

   hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   switch( roamResult )
   {
      case eCSR_ROAM_RESULT_IBSS_NEW_PEER:
      {
         hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
         struct station_info *staInfo;

         VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "IBSS New Peer indication from SME with peerMac " MAC_ADDRESS_STR " BSSID: " MAC_ADDRESS_STR " and stationID= %d",
                    MAC_ADDR_ARRAY(pRoamInfo->peerMac),
                    MAC_ADDR_ARRAY(pHddStaCtx->conn_info.bssId),
                    pRoamInfo->staId );

         pHddCtx->sta_to_adapter[pRoamInfo->staId] = pAdapter;

         pHddCtx->sta_to_adapter[IBSS_BROADCAST_STAID] = pAdapter;
         WLANTL_UpdateSTABssIdforIBSS(pHddCtx->pvosContext,
                      IBSS_BROADCAST_STAID,pHddStaCtx->conn_info.bssId);

         // Register the Station with TL for the new peer.
         vosStatus = hdd_ibss_RegisterSTA( pAdapter,
                                          pRoamInfo,
                                          pRoamInfo->staId,
                                          (v_MACADDR_t *)pRoamInfo->peerMac,
                                          pRoamInfo->pBssDesc );
         if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
         {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
               "Cannot register STA with TL for IBSS.  Failed with vosStatus = %d [%08X]",
               vosStatus, vosStatus );
         }
         if (!roamSaveIbssStation(pAdapter,
               pRoamInfo->staId,
               (v_MACADDR_t *)pRoamInfo->peerMac))
         {
            hddLog(LOGW, FL("Not Able to add sta in sta hash"));
            break;
         }
         pHddStaCtx->ibss_sta_generation++;

         staInfo = vos_mem_malloc(sizeof(*staInfo));
         if (staInfo == NULL) {
             VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                       "memory allocation for station_info failed");
             return eHAL_STATUS_FAILED_ALLOC;
         }

         memset(staInfo, 0, sizeof(*staInfo));
         staInfo->filled = 0;
         staInfo->generation = pHddStaCtx->ibss_sta_generation;

         cfg80211_new_sta(pAdapter->dev,
                      (const u8 *)pRoamInfo->peerMac,
                      staInfo, GFP_KERNEL);
         vos_mem_free(staInfo);

         if ( eCSR_ENCRYPT_TYPE_WEP40_STATICKEY == pHddStaCtx->ibss_enc_key.encType
            ||eCSR_ENCRYPT_TYPE_WEP104_STATICKEY == pHddStaCtx->ibss_enc_key.encType
            ||eCSR_ENCRYPT_TYPE_TKIP == pHddStaCtx->ibss_enc_key.encType
            ||eCSR_ENCRYPT_TYPE_AES == pHddStaCtx->ibss_enc_key.encType )
         {
            pHddStaCtx->ibss_enc_key.keyDirection = eSIR_TX_RX;

            memcpy(&pHddStaCtx->ibss_enc_key.peerMac,
                              pRoamInfo->peerMac, WNI_CFG_BSSID_LEN);

            VOS_TRACE( VOS_MODULE_ID_HDD,
               VOS_TRACE_LEVEL_INFO_HIGH, "New peer joined set PTK encType=%d",
               pHddStaCtx->ibss_enc_key.encType);

            vosStatus = sme_RoamSetKey( WLAN_HDD_GET_HAL_CTX(pAdapter),
               pAdapter->sessionId, &pHddStaCtx->ibss_enc_key, &roamId );

            if ( VOS_STATUS_SUCCESS != vosStatus )
            {
               hddLog(VOS_TRACE_LEVEL_ERROR,
                       "%s: sme_RoamSetKey failed, returned %d",
                       __func__, vosStatus);
               return VOS_STATUS_E_FAILURE;
            }
         }
         netif_carrier_on(pAdapter->dev);
         hddLog(VOS_TRACE_LEVEL_INFO, FL("Enabling queues"));
         netif_tx_start_all_queues(pAdapter->dev);
         break;
      }

      case eCSR_ROAM_RESULT_IBSS_CONNECT:
      {

         roamIbssConnectHandler( pAdapter, pRoamInfo );

         break;
      }
      case eCSR_ROAM_RESULT_IBSS_PEER_DEPARTED:
      {
         hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

         if ( !roamRemoveIbssStation(pAdapter, pRoamInfo->staId ) )
         {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                    "IBSS peer departed by cannot find peer in our registration table with TL" );
         }

          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "IBSS Peer Departed from SME with peerMac " MAC_ADDRESS_STR " BSSID: " MAC_ADDRESS_STR " and stationID= %d",
                    MAC_ADDR_ARRAY(pRoamInfo->peerMac),
                    MAC_ADDR_ARRAY(pHddStaCtx->conn_info.bssId),
                    pRoamInfo->staId );

         hdd_ibss_DeregisterSTA( pAdapter, pRoamInfo->staId );

         pHddCtx->sta_to_adapter[pRoamInfo->staId] = NULL;
         pHddStaCtx->ibss_sta_generation++;

         cfg80211_del_sta(pAdapter->dev,
                         (const u8 *)&pRoamInfo->peerMac,
                         GFP_KERNEL);
         break;
      }
      case eCSR_ROAM_RESULT_IBSS_INACTIVE:
      {
          VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
                    "Received eCSR_ROAM_RESULT_IBSS_INACTIVE from SME");
         // Stop only when we are inactive
         hddLog(VOS_TRACE_LEVEL_INFO, FL("Disabling queues"));
         netif_tx_disable(pAdapter->dev);
         netif_carrier_off(pAdapter->dev);
         VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   "%s: Set HDD connState to eConnectionState_NotConnected",
                   __func__);
         hdd_connSetConnectionState( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter), eConnectionState_NotConnected );

         // Send the bssid address to the wext.
         hdd_SendAssociationEvent(pAdapter->dev, pRoamInfo);
         // clean up data path
         hdd_disconnect_tx_rx(pAdapter);
         break;
      }
      default:
         break;

   }

   return( eHAL_STATUS_SUCCESS );
}

#ifdef FEATURE_WLAN_TDLS
/**============================================================================
 *
  @brief hdd_roamRegisterTDLSSTA() - Construct the staDesc and register with
  TL the new STA. This is called as part of ADD_STA in the TDLS setup
  Return: VOS_STATUS

  ===========================================================================*/
VOS_STATUS hdd_roamRegisterTDLSSTA(hdd_adapter_t *pAdapter,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
                                   const tANI_U8 *peerMac,
#else
                                   tANI_U8 *peerMac,
#endif
                                   tANI_U16 staId, tANI_U8 ucastSig)
{
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    v_CONTEXT_t pVosContext = (WLAN_HDD_GET_CTX(pAdapter))->pvosContext;
    VOS_STATUS vosStatus = VOS_STATUS_E_FAILURE;
    WLAN_STADescType staDesc = {0};
    eCsrEncryptionType connectedCipherAlgo = eCSR_ENCRYPT_TYPE_UNKNOWN;
    v_BOOL_t fConnected   = FALSE;
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    hdd_config_t *cfg_param = pHddCtx->cfg_ini;

    fConnected = hdd_connGetConnectedCipherAlgo( pHddStaCtx, &connectedCipherAlgo );
    if (!fConnected) {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     "%s not connected. ignored", __func__);
        return VOS_FALSE;
    }

    /*
     * TDLS sta in BSS should be set as STA type TDLS and STA MAC should
     * be peer MAC, here we are wokrking on direct Link
     */
    staDesc.ucSTAId = staId ;

    staDesc.wSTAType = WLAN_STA_TDLS ;

    vos_mem_copy( staDesc.vSTAMACAddress.bytes, peerMac,
                                         sizeof(tSirMacAddr) );

    vos_mem_copy(staDesc.vBSSIDforIBSS.bytes, pHddStaCtx->conn_info.bssId,6 );
    vos_copy_macaddr( &staDesc.vSelfMACAddress, &pAdapter->macAddressCurrent );

    /* set the QoS field appropriately ..*/
    (hdd_wmm_is_active(pAdapter)) ? (staDesc.ucQosEnabled = 1)
                                          : (staDesc.ucQosEnabled = 0) ;

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH, "HDD register \
                                TL QoS_enabled=%d", staDesc.ucQosEnabled );

    staDesc.ucProtectedFrame = (connectedCipherAlgo != eCSR_ENCRYPT_TYPE_NONE) ;

    VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_MED,
               "HDD register TL Sec_enabled= %d.", staDesc.ucProtectedFrame );

    /*
     * UMA is ready we inform TL  to do frame translation.
     */
    staDesc.ucSwFrameTXXlation = 1;
    staDesc.ucSwFrameRXXlation = 1;
    staDesc.ucAddRmvLLC = 1;

    /* Initialize signatures and state */
    staDesc.ucUcastSig  = ucastSig ;

    /* tdls Direct Link do not need bcastSig */
    staDesc.ucBcastSig  = 0 ;

#ifdef VOLANS_ENABLE_SW_REPLAY_CHECK
    if(staDesc.ucProtectedFrame)
        staDesc.ucIsReplayCheckValid = VOS_TRUE;
    else
        staDesc.ucIsReplayCheckValid = VOS_FALSE;
#endif

    staDesc.ucInitState = WLANTL_STA_CONNECTED ;

   /* Register the Station with TL...  */
    vosStatus = WLANTL_RegisterSTAClient( pVosContext,
                                          hdd_rx_packet_cbk,
                                          hdd_tx_complete_cbk,
                                          hdd_tx_fetch_packet_cbk, &staDesc, 0 );

    if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                   "%s: WLANTL_RegisterSTAClient() failed to register.  "
                   "Status= %d [0x%08X]", __func__, vosStatus, vosStatus );
         return vosStatus;
    }

    if ( cfg_param->dynSplitscan &&
       ( VOS_TIMER_STATE_RUNNING !=
                        vos_timer_getCurrentState(&pHddCtx->tx_rx_trafficTmr)) )
    {
        vos_timer_start(&pHddCtx->tx_rx_trafficTmr,
                        cfg_param->trafficMntrTmrForSplitScan);
    }
    return( vosStatus );
}

VOS_STATUS hdd_roamDeregisterTDLSSTA( hdd_adapter_t *pAdapter, tANI_U8 staId )
{
    VOS_STATUS vosStatus;
    vosStatus = WLANTL_ClearSTAClient( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext, staId );
    if ( !VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                   "%s: WLANTL_ClearSTAClient() failed to for staID %d.  "
                   "Status= %d [0x%08X]",
                   __func__, staId, vosStatus, vosStatus );
    }
    return( vosStatus );
}


/*
 * HDD interface between SME and TL to ensure TDLS client registration with
 * TL in case of new TDLS client is added and deregistration at the time
 * TDLS client is deleted.
 */

eHalStatus hdd_RoamTdlsStatusUpdateHandler(hdd_adapter_t *pAdapter,
                                             tCsrRoamInfo *pRoamInfo,
                                              tANI_U32 roamId,
                                                eRoamCmdStatus roamStatus,
                                                  eCsrRoamResult roamResult)
{
    hdd_context_t *pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    eHalStatus status = eHAL_STATUS_FAILURE ;
    tANI_U8 staIdx;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
              ("hdd_tdlsStatusUpdate: %s staIdx %d " MAC_ADDRESS_STR),
              roamResult == eCSR_ROAM_RESULT_ADD_TDLS_PEER ?
              "ADD_TDLS_PEER" :
              roamResult == eCSR_ROAM_RESULT_DELETE_TDLS_PEER ?
              "DEL_TDLS_PEER" :
              roamResult == eCSR_ROAM_RESULT_TEARDOWN_TDLS_PEER_IND ?
              "DEL_TDLS_PEER_IND" :
              roamResult == eCSR_ROAM_RESULT_DELETE_ALL_TDLS_PEER_IND ?
              "DEL_ALL_TDLS_PEER_IND" :
              roamResult == eCSR_ROAM_RESULT_UPDATE_TDLS_PEER ?
              "UPDATE_TDLS_PEER" :
              roamResult == eCSR_ROAM_RESULT_LINK_ESTABLISH_REQ_RSP ?
              "LINK_ESTABLISH_REQ_RSP" :
              roamResult == eCSR_ROAM_RESULT_CHANNEL_SWITCH_REQ_RSP ?
              "CHANNEL_SWITCH_REQ_RSP" : "UNKNOWN",
              pRoamInfo->staId, MAC_ADDR_ARRAY(pRoamInfo->peerMac));
    switch( roamResult )
    {
        case eCSR_ROAM_RESULT_ADD_TDLS_PEER:
        {
            if(eSIR_SME_SUCCESS != pRoamInfo->statusCode)
            {
                hddTdlsPeer_t *curr_peer;

                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     ("%s: Add Sta is failed. %d"),__func__, pRoamInfo->statusCode);
                wlan_hdd_tdls_check_bmps(pAdapter);

                mutex_lock(&pHddCtx->tdls_lock);
                curr_peer = wlan_hdd_tdls_find_peer(pAdapter,
                                      pRoamInfo->peerMac, FALSE);
                if (NULL != curr_peer)
                    curr_peer->link_status = eTDLS_LINK_TEARING;
                else
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                              "%s %d curr_peer is Null", __func__,__LINE__);
                mutex_unlock(&pHddCtx->tdls_lock);
            }
            else
            {

                /* check if there is available index for this new TDLS STA */
                for ( staIdx = 0; staIdx < HDD_MAX_NUM_TDLS_STA; staIdx++ )
                {
                    if (0 == pHddCtx->tdlsConnInfo[staIdx].staId )
                    {
                        pHddCtx->tdlsConnInfo[staIdx].sessionId = pRoamInfo->sessionId;
                        pHddCtx->tdlsConnInfo[staIdx].staId = pRoamInfo->staId;

                        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                                  ("TDLS: STA IDX at %d is %d "
                                  "of mac " MAC_ADDRESS_STR),
                                  staIdx, pHddCtx->tdlsConnInfo[staIdx].staId,
                                  MAC_ADDR_ARRAY(pRoamInfo->peerMac));

                        vos_copy_macaddr(&pHddCtx->tdlsConnInfo[staIdx].peerMac,
                                         (v_MACADDR_t *)pRoamInfo->peerMac) ;
                        status = eHAL_STATUS_SUCCESS ;
                        break ;
                    }
                }
                if (staIdx < HDD_MAX_NUM_TDLS_STA)
                {
                    if (-1 == wlan_hdd_tdls_set_sta_id(pAdapter, pRoamInfo->peerMac, pRoamInfo->staId)) {
                        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                                     "wlan_hdd_tdls_set_sta_id() failed");
                        return VOS_FALSE;
                    }

                    (WLAN_HDD_GET_CTX(pAdapter))->sta_to_adapter[pRoamInfo->staId] = pAdapter;
                    /* store the ucast signature , if required for further reference. */

                    wlan_hdd_tdls_set_signature( pAdapter, pRoamInfo->peerMac, pRoamInfo->ucastSig );
                    /* start TDLS client registration with TL */
                    status = hdd_roamRegisterTDLSSTA( pAdapter,
                                                      pRoamInfo->peerMac,
                                                      pRoamInfo->staId,
                                                      pRoamInfo->ucastSig);
                    wlan_hdd_tdls_increment_peer_count(pAdapter);
                }
                else
                {
                    status = eHAL_STATUS_FAILURE;
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    "%s: no available slot in conn_info. staId %d cannot be stored", __func__, pRoamInfo->staId);
                }
                pAdapter->tdlsAddStaStatus = status;
            }
            complete(&pAdapter->tdls_add_station_comp);
            break ;
        }
        case eCSR_ROAM_RESULT_UPDATE_TDLS_PEER:
        {
            if (eSIR_SME_SUCCESS != pRoamInfo->statusCode)
            {
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     "%s: Add Sta is failed. %d", __func__, pRoamInfo->statusCode);
            }
            /* store the ucast signature which will be used later when
             * registering to TL
             */
            pAdapter->tdlsAddStaStatus = pRoamInfo->statusCode;
            complete(&pAdapter->tdls_add_station_comp);
            break;
        }
        case eCSR_ROAM_RESULT_LINK_ESTABLISH_REQ_RSP:
        {
            if (eSIR_SME_SUCCESS != pRoamInfo->statusCode)
            {
                hddTdlsPeer_t *curr_peer;

                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                     "%s: Link Establish Request failed. %d", __func__, pRoamInfo->statusCode);

                mutex_lock(&pHddCtx->tdls_lock);
                curr_peer = wlan_hdd_tdls_find_peer(pAdapter,
                                       pRoamInfo->peerMac, FALSE);
                if (curr_peer)
                    curr_peer->link_status = eTDLS_LINK_TEARING;
                else
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                              "%s %d curr_peer is Null",__func__,__LINE__);
                mutex_unlock(&pHddCtx->tdls_lock);
            }
            complete(&pAdapter->tdls_link_establish_req_comp);
            break;
        }
        case eCSR_ROAM_RESULT_CHANNEL_SWITCH_REQ_RSP:
        {
            if (eSIR_SME_SUCCESS != pRoamInfo->statusCode)
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                          "%s: Channel switch request failed. %d", __func__,
                          pRoamInfo->statusCode);
            else
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                          "%s: Channel switch request Success", __func__);
            break;
        }
        case eCSR_ROAM_RESULT_DELETE_TDLS_PEER:
        {
            hddTdlsPeer_t *curr_peer;
            for ( staIdx = 0; staIdx < HDD_MAX_NUM_TDLS_STA; staIdx++ )
            {
                if ((pHddCtx->tdlsConnInfo[staIdx].sessionId == pRoamInfo->sessionId) &&
                    pRoamInfo->staId == pHddCtx->tdlsConnInfo[staIdx].staId)
                {
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                                   ("HDD: del STA IDX = %x"), pRoamInfo->staId) ;

                    mutex_lock(&pHddCtx->tdls_lock);
                    curr_peer = wlan_hdd_tdls_find_peer(pAdapter,
                                                  pRoamInfo->peerMac, FALSE);
                    if (NULL != curr_peer)
                    {
                       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                                 " Current status for peer" MAC_ADDRESS_STR "is %d",
                                 MAC_ADDR_ARRAY(pRoamInfo->peerMac), curr_peer->link_status);
                       if (TDLS_IS_CONNECTED(curr_peer) ||
                          (eTDLS_LINK_CONNECTING == curr_peer->link_status))
                       {
                           mutex_unlock(&pHddCtx->tdls_lock);
                           hdd_roamDeregisterTDLSSTA ( pAdapter, pRoamInfo->staId );
                       }
                       else
                           mutex_unlock(&pHddCtx->tdls_lock);

                       wlan_hdd_tdls_decrement_peer_count(pAdapter);
                    }
                    else
                        mutex_unlock(&pHddCtx->tdls_lock);

                    mutex_lock(&pHddCtx->tdls_lock);
                    wlan_hdd_tdls_reset_peer(pAdapter, pRoamInfo->peerMac);
                    mutex_unlock(&pHddCtx->tdls_lock);

                    pHddCtx->tdlsConnInfo[staIdx].staId = 0 ;
                    pHddCtx->tdlsConnInfo[staIdx].sessionId = 255;
                    vos_mem_zero(&pHddCtx->tdlsConnInfo[staIdx].peerMac,
                                               sizeof(v_MACADDR_t)) ;
                    wlan_hdd_tdls_check_bmps(pAdapter);
                    status = eHAL_STATUS_SUCCESS ;
                    break ;
                }
            }
            complete(&pAdapter->tdls_del_station_comp);
        }
        break ;
        case eCSR_ROAM_RESULT_TEARDOWN_TDLS_PEER_IND:
        {
            hddTdlsPeer_t *curr_peer;
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                       "%s: Sending teardown to supplicant with reason code %u",
                       __func__, pRoamInfo->reasonCode);

#ifdef CONFIG_TDLS_IMPLICIT
            mutex_lock(&pHddCtx->tdls_lock);
            curr_peer = wlan_hdd_tdls_find_peer(pAdapter, pRoamInfo->peerMac,
                                                FALSE);
            wlan_hdd_tdls_indicate_teardown(pAdapter, curr_peer, pRoamInfo->reasonCode);
            hdd_send_wlan_tdls_teardown_event(eTDLS_TEARDOWN_BSS_DISCONNECT,
                                                          curr_peer->peerMac);
            mutex_unlock(&pHddCtx->tdls_lock);
#endif
            status = eHAL_STATUS_SUCCESS ;
            break ;
        }
        case eCSR_ROAM_RESULT_DELETE_ALL_TDLS_PEER_IND:
        {
            /* 0 staIdx is assigned to AP we dont want to touch that */
            for ( staIdx = 0; staIdx < HDD_MAX_NUM_TDLS_STA; staIdx++ )
            {
                if ((pHddCtx->tdlsConnInfo[staIdx].sessionId == pRoamInfo->sessionId) &&
                    pHddCtx->tdlsConnInfo[staIdx].staId)
                {
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                              ("hdd_tdlsStatusUpdate: staIdx %d " MAC_ADDRESS_STR),
                                pHddCtx->tdlsConnInfo[staIdx].staId,
                                MAC_ADDR_ARRAY(pHddCtx->tdlsConnInfo[staIdx].peerMac.bytes));

                    mutex_lock(&pHddCtx->tdls_lock);
                    wlan_hdd_tdls_reset_peer(pAdapter, pHddCtx->tdlsConnInfo[staIdx].peerMac.bytes);
                    mutex_unlock(&pHddCtx->tdls_lock);

                    hdd_roamDeregisterTDLSSTA ( pAdapter,  pHddCtx->tdlsConnInfo[staIdx].staId );
                    wlan_hdd_tdls_decrement_peer_count(pAdapter);

                    vos_mem_zero(&pHddCtx->tdlsConnInfo[staIdx].peerMac,
                                               sizeof(v_MACADDR_t)) ;
                    pHddCtx->tdlsConnInfo[staIdx].staId = 0 ;
                    pHddCtx->tdlsConnInfo[staIdx].sessionId = 255;

                    status = eHAL_STATUS_SUCCESS ;
                }
            }
            wlan_hdd_tdls_check_bmps(pAdapter);
            break ;
        }
        default:
        {
            break ;
        }
    }

    return status ;
}
#endif

void iw_full_power_cbfn (void *pContext, eHalStatus status)
{
    hdd_adapter_t *pAdapter = (hdd_adapter_t *)pContext;
    hdd_context_t *pHddCtx = NULL;
    int ret;

    ENTER();

    if ((NULL == pAdapter) || (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic))
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
             "%s: Bad param, pAdapter [%pK]",
               __func__, pAdapter);
        return;
    }

    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
    {
        return;
    }

    if (pHddCtx->cfg_ini->fIsBmpsEnabled)
    {
        sme_RequestBmps(WLAN_HDD_GET_HAL_CTX(pAdapter), NULL, NULL);
    }

    EXIT();
}

eHalStatus hdd_smeRoamCallback( void *pContext, tCsrRoamInfo *pRoamInfo, tANI_U32 roamId,
                                eRoamCmdStatus roamStatus, eCsrRoamResult roamResult )
{
    eHalStatus halStatus = eHAL_STATUS_SUCCESS;
    hdd_adapter_t *pAdapter = (hdd_adapter_t *)pContext;
    hdd_wext_state_t *pWextState = NULL;
    hdd_station_ctx_t *pHddStaCtx = NULL;
    VOS_STATUS status = VOS_STATUS_SUCCESS;
    struct cfg80211_bss *bss_status;
    hdd_context_t *pHddCtx = NULL;

    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO_HIGH,
            "CSR Callback: status= %d result= %d roamID=%d",
                    roamStatus, roamResult, roamId );

    /*Sanity check*/
    if ((NULL == pAdapter) || (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic))
    {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
          "invalid adapter or adapter has invalid magic");
       return eHAL_STATUS_FAILURE;
    }

    pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

    if ((NULL == pWextState) || (NULL == pHddStaCtx))
    {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
          "invalid WEXT state or HDD station context");
       return eHAL_STATUS_FAILURE;
    }

    MTRACE(vos_trace(VOS_MODULE_ID_HDD, TRACE_CODE_HDD_RX_SME_MSG,
                               pAdapter->sessionId, roamStatus));
    switch( roamStatus )
    {
        case eCSR_ROAM_SESSION_OPENED:
            set_bit(SME_SESSION_OPENED, &pAdapter->event_flags);
            complete(&pAdapter->session_open_comp_var);
            break;

#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
            /* We did pre-auth,then we attempted a 11r or ese reassoc.
             * reassoc failed due to failure, timeout, reject from ap
             * in any case tell the OS, our carrier is off and mark
             * interface down */
        case eCSR_ROAM_FT_REASSOC_FAILED:
            hddLog(LOGE, FL("Reassoc Failed with roamStatus: %d roamResult: %d SessionID: %d"),
                          roamStatus, roamResult, pAdapter->sessionId);
            sme_resetCoexEevent(WLAN_HDD_GET_HAL_CTX(pAdapter));
            halStatus = hdd_DisConnectHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
            /* Check if Mcast/Bcast Filters are set, if yes clear the filters here */
            if ((WLAN_HDD_GET_CTX(pAdapter))->hdd_mcastbcast_filter_set == TRUE) {
                    (WLAN_HDD_GET_CTX(pAdapter))->hdd_mcastbcast_filter_set = FALSE;
            }
            pHddStaCtx->ft_carrier_on = FALSE;
            pHddStaCtx->hdd_ReassocScenario = FALSE;
            break;

        case eCSR_ROAM_FT_START:
            // When we roam for EsE and 11r, we dont want the
            // OS to be informed that the link is down. So mark
            // the link ready for ft_start. After this the
            // eCSR_ROAM_SHOULD_ROAM will be received.
            // Where in we will not mark the link down
            // Also we want to stop tx at this point when we will be
            // doing disassoc at this time. This saves 30-60 msec
            // after reassoc.
            {
                struct net_device *dev = pAdapter->dev;
                hddLog(VOS_TRACE_LEVEL_INFO, FL("Disabling queues"));
                netif_tx_disable(dev);
                pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
                if (pHddCtx->cfg_ini->gEnableRoamDelayStats)
                {
                    vos_record_roam_event(e_HDD_DISABLE_TX_QUEUE, NULL, 0);
                }
                /*
                 * Deregister this STA with TL, but do not flush the packets
                 * for this STA from wmm_tx_queue. Since there is no valid STA
                 * for these packets they will not be transmitted. Eventually
                 * after the reassociation is successful, these packets will be
                 * transmitted after registering STA with TL again. This ensures
                 * that driver does not drop packets during roaming.
                 */
                status = WLANTL_ClearSTAClient(pHddCtx->pvosContext,
                                               pHddStaCtx->conn_info.staId[0]);
                if (!VOS_IS_STATUS_SUCCESS(status))
                {
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                              FL("WLANTL_ClearSTAClient failed for staID %d."
                              "Status= %d [0x%x]"), pHddStaCtx->conn_info.staId[0],
                              status, status);
                    halStatus = eHAL_STATUS_FAILURE;
                }
            }
            pHddStaCtx->ft_carrier_on = TRUE;
            pHddStaCtx->hdd_ReassocScenario = VOS_TRUE;
            break;
#endif

        case eCSR_ROAM_SHOULD_ROAM:
           // Dont need to do anything
            {
                struct net_device *dev = pAdapter->dev;
                hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
                // notify apps that we can't pass traffic anymore
                hddLog(VOS_TRACE_LEVEL_INFO, FL("Disabling queues"));
                netif_tx_disable(dev);
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
                if (pHddStaCtx->ft_carrier_on == FALSE)
                {
#endif
                    netif_carrier_off(dev);
#if  defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR)
                }
#endif

#if  !(defined (WLAN_FEATURE_VOWIFI_11R) || defined (FEATURE_WLAN_ESE) || defined(FEATURE_WLAN_LFR))
                //We should clear all sta register with TL, for now, only one.
                status = hdd_roamDeregisterSTA( pAdapter, pHddStaCtx->conn_info.staId [0] );
                if ( !VOS_IS_STATUS_SUCCESS(status ) )
                {
                    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                        FL("hdd_roamDeregisterSTA() failed to for staID %d.  Status= %d [0x%x]"),
                                        pHddStaCtx->conn_info.staId[0], status, status );
                    halStatus = eHAL_STATUS_FAILURE;
                }
#endif
            }
           break;
        case eCSR_ROAM_LOSTLINK_DETECTED:
             {
                 if (wlan_hdd_check_and_stop_mon(pAdapter, false))
                     halStatus = eHAL_STATUS_FAILURE;
             }
             break;
        case eCSR_ROAM_LOSTLINK:
        case eCSR_ROAM_DISASSOCIATED:
            {
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                        "****eCSR_ROAM_DISASSOCIATED****");
                sme_resetCoexEevent(WLAN_HDD_GET_HAL_CTX(pAdapter));
                halStatus = hdd_DisConnectHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
                /* Check if Mcast/Bcast Filters are set, if yes clear the filters here */
                pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
                if (pHddCtx->hdd_mcastbcast_filter_set == TRUE)
                {
                    hdd_conf_mcastbcast_filter(pHddCtx, FALSE);

                    if (VOS_TRUE == pHddCtx->sus_res_mcastbcast_filter_valid) {
                        pHddCtx->configuredMcastBcastFilter =
                            pHddCtx->sus_res_mcastbcast_filter;
                        pHddCtx->sus_res_mcastbcast_filter_valid = VOS_FALSE;
                    }

                    hddLog(VOS_TRACE_LEVEL_INFO,
                           "offload: disassociation happening, restoring configuredMcastBcastFilter");
                    hddLog(VOS_TRACE_LEVEL_INFO,"McastBcastFilter = %d",
                           pHddCtx->configuredMcastBcastFilter);
                    hddLog(VOS_TRACE_LEVEL_INFO,
                           "offload: already called mcastbcast filter");
                    (WLAN_HDD_GET_CTX(pAdapter))->hdd_mcastbcast_filter_set = FALSE;
                }
#ifdef WLAN_FEATURE_PACKET_FILTERING
                /* Call to clear any MC Addr List filter applied after
                 * successful connection.
                 */
                wlan_hdd_set_mc_addr_list(pAdapter, FALSE);
#endif
            }
            break;
        case eCSR_ROAM_IBSS_LEAVE:
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                    "****eCSR_ROAM_IBSS_LEAVE****");
            halStatus = hdd_DisConnectHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
            break;
        case eCSR_ROAM_ASSOCIATION_COMPLETION:
            VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                    "****eCSR_ROAM_ASSOCIATION_COMPLETION****");
            // To Do - address probable memory leak with WEP encryption upon successful association
            if (eCSR_ROAM_RESULT_ASSOCIATED != roamResult)
            {
               //Clear saved connection information in HDD
               hdd_connRemoveConnectInfo( WLAN_HDD_GET_STATION_CTX_PTR(pAdapter) );
            }
            halStatus = hdd_AssociationCompletionHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );

            break;
        case eCSR_ROAM_ASSOCIATION_FAILURE:
            halStatus = hdd_AssociationCompletionHandler( pAdapter,
                    pRoamInfo, roamId, roamStatus, roamResult );
            break;
        case eCSR_ROAM_IBSS_IND:
            hdd_RoamIbssIndicationHandler( pAdapter, pRoamInfo, roamId,
                                           roamStatus, roamResult );
            break;

        case eCSR_ROAM_CONNECT_STATUS_UPDATE:
            halStatus = roamRoamConnectStatusUpdateHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
            break;

        case eCSR_ROAM_MIC_ERROR_IND:
            halStatus = hdd_RoamMicErrorIndicationHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
            break;
        case eCSR_ROAM_LOST_LINK_PARAMS_IND:
            {
                /*
                 * The RSSI will be subtracted from 100 as FW is sending the RSSI by
                 * adding the 100 value.
                 */
                pAdapter->rssi_on_disconnect = pRoamInfo->u.pLostLinkParams->rssi - 100;
                VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                    "%s : Rssi on Disconnect : %d",
                    __func__, pAdapter->rssi_on_disconnect);
                break;
            }
        case eCSR_ROAM_SET_KEY_COMPLETE:
            {
                hdd_context_t* pHddCtx = (hdd_context_t*)pAdapter->pHddCtx;

                if((pHddCtx) &&
                   (TRUE == pHddCtx->hdd_wlan_suspended) &&
                   ((eCSR_ROAM_RESULT_NONE == roamResult)||
                     (pRoamInfo && pRoamInfo->is11rAssoc)))
                {
                    /* Send DTIM period to the FW; only if the wlan is already
                       in suspend. This is the case with roaming (reassoc),
                       DELETE_BSS_REQ zeroes out Modulated/Dynamic DTIM sent in
                       previous suspend_wlan. Sending SET_POWER_PARAMS_REQ
                       before the ENTER_BMPS_REQ ensures Listen Interval is
                       regained back to LI * Modulated DTIM */
                    hdd_set_pwrparams(pHddCtx);

                    /* At this point, device should not be in BMPS;
                       if due to unexpected scenario, if we are in BMPS,
                       then trigger Exit and Enter BMPS to take DTIM period
                       effective */
                    if (BMPS == pmcGetPmcState(pHddCtx->hHal))
                    {
                        hddLog( LOGE, FL("Not expected: device is already in BMPS mode, Exit & Enter BMPS again!"));

                        sme_RequestFullPower(WLAN_HDD_GET_HAL_CTX(pAdapter),
                                         iw_full_power_cbfn, pAdapter,
                                         eSME_FULL_PWR_NEEDED_BY_HDD);
                    }
                }

                if ((pHddCtx) &&
                    (FULL_POWER == pmcGetPmcState(pHddCtx->hHal)) &&
                    (VOS_TRUE == pHddStaCtx->hdd_ReassocScenario) &&
                    ((eCSR_ROAM_RESULT_NONE == roamResult) ||
                      (pRoamInfo && pRoamInfo->is11rAssoc)))
                {
                    hddLog( LOG1, FL("Device in full power."
                           "Stop and start traffic timer for roaming"));
                    pmcStopTrafficTimer(pHddCtx->hHal);
                    if (pmcStartTrafficTimer(pHddCtx->hHal,
                        TRAFFIC_TIMER_ROAMING) != eHAL_STATUS_SUCCESS)
                    {
                       hddLog(LOGP, FL("Cannot start traffic timer"));
                    }
                }

                halStatus = hdd_RoamSetKeyCompleteHandler( pAdapter, pRoamInfo, roamId, roamStatus, roamResult );
                if ((eCSR_ROAM_RESULT_NONE == roamResult) ||
                     (pRoamInfo && pRoamInfo->is11rAssoc))
                    pHddStaCtx->hdd_ReassocScenario = FALSE;
            }
            break;
#ifdef WLAN_FEATURE_VOWIFI_11R
        case eCSR_ROAM_FT_RESPONSE:
            hdd_SendFTEvent(pAdapter);
            break;
#endif
#if defined(FEATURE_WLAN_LFR) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0))
        case eCSR_ROAM_PMK_NOTIFY:
           if (eCSR_AUTH_TYPE_RSN == pHddStaCtx->conn_info.authType)
           {
               /* Notify the supplicant of a new candidate */
               halStatus = wlan_hdd_cfg80211_pmksa_candidate_notify(pAdapter, pRoamInfo, 1, false);
           }
           break;
#endif

#ifdef FEATURE_WLAN_LFR_METRICS
        case eCSR_ROAM_PREAUTH_INIT_NOTIFY:
           /* This event is to notify pre-auth initiation */
           if (VOS_STATUS_SUCCESS !=
               wlan_hdd_cfg80211_roam_metrics_preauth(pAdapter, pRoamInfo))
           {
               halStatus = eHAL_STATUS_FAILURE;
           }
           break;
        case eCSR_ROAM_PREAUTH_STATUS_SUCCESS:
           /* This event will notify pre-auth completion in case of success */
           if (VOS_STATUS_SUCCESS !=
               wlan_hdd_cfg80211_roam_metrics_preauth_status(pAdapter,
                                                             pRoamInfo, 1))
           {
               halStatus = eHAL_STATUS_FAILURE;
           }
           break;
        case eCSR_ROAM_PREAUTH_STATUS_FAILURE:
           /* This event will notify pre-auth completion in case of failure. */
           if (VOS_STATUS_SUCCESS !=
               wlan_hdd_cfg80211_roam_metrics_preauth_status(pAdapter,
                                                             pRoamInfo, 0))
           {
               halStatus = eHAL_STATUS_FAILURE;
           }
           break;
        case eCSR_ROAM_HANDOVER_SUCCESS:
           /* This event is to notify handover success.
              It will be only invoked on success */
           if (VOS_STATUS_SUCCESS !=
               wlan_hdd_cfg80211_roam_metrics_handover(pAdapter, pRoamInfo))
           {
               halStatus = eHAL_STATUS_FAILURE;
           }
           break;
#endif
        case eCSR_ROAM_REMAIN_CHAN_READY:
            hdd_remainChanReadyHandler( pAdapter );
            break;
        case eCSR_ROAM_SEND_ACTION_CNF:
            hdd_sendActionCnf( pAdapter,
               (roamResult == eCSR_ROAM_RESULT_NONE) ? TRUE : FALSE );
            break;
#ifdef FEATURE_WLAN_TDLS
        case eCSR_ROAM_TDLS_STATUS_UPDATE:
            halStatus = hdd_RoamTdlsStatusUpdateHandler( pAdapter, pRoamInfo,
                                                roamId, roamStatus, roamResult );
            break ;
        case eCSR_ROAM_RESULT_MGMT_TX_COMPLETE_IND:
            wlan_hdd_tdls_mgmt_completion_callback(pAdapter, pRoamInfo->reasonCode);
            break;
#endif
#ifdef WLAN_FEATURE_11W
       case eCSR_ROAM_UNPROT_MGMT_FRAME_IND:
            hdd_indicateUnprotMgmtFrame(pAdapter, pRoamInfo->nFrameLength,
                                         pRoamInfo->pbFrames,
                                         pRoamInfo->frameType);
            break;
#endif
#if defined(FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
       case eCSR_ROAM_TSM_IE_IND:
            hdd_indicateTsmIe(pAdapter, pRoamInfo->tsmIe.tsid,
                pRoamInfo->tsmIe.state, pRoamInfo->tsmIe.msmt_interval);
           break;

       case eCSR_ROAM_CCKM_PREAUTH_NOTIFY:
          {
              if (eCSR_AUTH_TYPE_CCKM_WPA == pHddStaCtx->conn_info.authType ||
                  eCSR_AUTH_TYPE_CCKM_RSN == pHddStaCtx->conn_info.authType)
              {
                  hdd_indicateCckmPreAuth(pAdapter, pRoamInfo);
              }
              break;
          }

       case eCSR_ROAM_ESE_ADJ_AP_REPORT_IND:
         {
             hdd_indicateEseAdjApRepInd(pAdapter, pRoamInfo);
             break;
         }

       case eCSR_ROAM_ESE_BCN_REPORT_IND:
         {
            hdd_indicateEseBcnReportInd(pAdapter, pRoamInfo);
            break;
         }
#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */
       case eCSR_ROAM_UPDATE_MAX_RATE_IND:
         {
            pAdapter->maxRateFlags = roamResult;
            break;
         }
       case eCSR_ROAM_UPDATE_SCAN_RESULT:
         if (pRoamInfo && pRoamInfo->pBssDesc) {
              bss_status = wlan_hdd_cfg80211_inform_bss_frame(pAdapter,
                                pRoamInfo->pBssDesc);
              if (bss_status)
                  cfg80211_put_bss(
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0)) || defined(WITH_BACKPORTS)
                                   (WLAN_HDD_GET_CTX(pAdapter))->wiphy,
#endif
                                   bss_status);
              else
                  hddLog(LOG1, FL("UPDATE_SCAN_RESULT returned NULL"));
         }
       case eCSR_ROAM_STA_CHANNEL_SWITCH:
         {
             hdd_adapter_t *pHostapdAdapter = NULL;
             pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
             pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);

             if (!pHddCtx || !pHddStaCtx) {
                 hddLog(LOG1, FL("Invalid pHddCtx or pHddStaCtx"));
                 break;
             }

             hddLog(LOG1, FL("eCSR_ROAM_STA_CHANNEL_SWITCH: new channel %hu"),
                     pRoamInfo->chan_info.chan_id);

             pHddStaCtx->conn_info.operationChannel =
                 pRoamInfo->chan_info.chan_id;

             pHostapdAdapter = hdd_get_adapter(pHddCtx, WLAN_HDD_SOFTAP);
             if (pHostapdAdapter &&
                    (test_bit(SOFTAP_BSS_STARTED,
                    &pHostapdAdapter->event_flags)))
             {
                 /* Restart SAP if its operating channel is different
                  * from AP channel.
                  */
                 hddLog(VOS_TRACE_LEVEL_INFO,"SAP chan %d, STA chan %d, force_scc_with_ecsa %d",
                         pHostapdAdapter->sessionCtx.ap.operatingChannel,
                         pRoamInfo->chan_info.chan_id,
                         pHddCtx->cfg_ini->force_scc_with_ecsa);
                 if ((pHddCtx->cfg_ini->force_scc_with_ecsa ) &&
                         (pHostapdAdapter->sessionCtx.ap.operatingChannel !=
                          pRoamInfo->chan_info.chan_id))
                 {
                     schedule_delayed_work(&pHddCtx->ecsa_chan_change_work, 0);
                 }
                 else
                     hddLog(LOG1, FL("SAP restart not required"));
             } else
                 hddLog(LOG1, FL("SAP not active, nothing to do"));
         }
         break;
       default:
            break;
    }
    return( halStatus );
}
eCsrAuthType hdd_TranslateRSNToCsrAuthType( u_int8_t auth_suite[4])
{
    eCsrAuthType auth_type;
    // is the auth type supported?
    if ( memcmp(auth_suite , ccpRSNOui01, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_RSN;
    } else
    if (memcmp(auth_suite , ccpRSNOui02, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_RSN_PSK;
    } else
#ifdef WLAN_FEATURE_VOWIFI_11R
    if (memcmp(auth_suite , ccpRSNOui04, 4) == 0)
    {
        // Check for 11r FT Authentication with PSK
        auth_type = eCSR_AUTH_TYPE_FT_RSN_PSK;
    } else
    if (memcmp(auth_suite , ccpRSNOui03, 4) == 0)
    {
        // Check for 11R FT Authentication with 802.1X
        auth_type = eCSR_AUTH_TYPE_FT_RSN;
    } else
#endif
#ifdef FEATURE_WLAN_ESE
    if (memcmp(auth_suite , ccpRSNOui06, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_CCKM_RSN;
    } else
#endif /* FEATURE_WLAN_ESE */
#ifdef WLAN_FEATURE_11W
    if (memcmp(auth_suite , ccpRSNOui07, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_RSN_PSK_SHA256;
    } else
    if (memcmp(auth_suite , ccpRSNOui08, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_RSN_8021X_SHA256;
    } else
#endif
    {
        auth_type = eCSR_AUTH_TYPE_UNKNOWN;
    }
    return auth_type;
}

eCsrAuthType
hdd_TranslateWPAToCsrAuthType(u_int8_t auth_suite[4])
{
    eCsrAuthType auth_type;
    // is the auth type supported?
    if ( memcmp(auth_suite , ccpWpaOui01, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_WPA;
    } else
    if (memcmp(auth_suite , ccpWpaOui02, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_WPA_PSK;
    } else
#ifdef FEATURE_WLAN_ESE
    if (memcmp(auth_suite , ccpWpaOui06, 4) == 0)
    {
        auth_type = eCSR_AUTH_TYPE_CCKM_WPA;
    } else
#endif /* FEATURE_WLAN_ESE */
    {
        auth_type = eCSR_AUTH_TYPE_UNKNOWN;
    }
    hddLog(LOG1, FL("auth_type: %d"), auth_type);
    return auth_type;
}

eCsrEncryptionType
hdd_TranslateRSNToCsrEncryptionType(u_int8_t cipher_suite[4])
{
    eCsrEncryptionType cipher_type;
    // is the cipher type supported?
    if ( memcmp(cipher_suite , ccpRSNOui04, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_AES;
    }
    else if (memcmp(cipher_suite , ccpRSNOui02, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_TKIP;
    }
    else if (memcmp(cipher_suite , ccpRSNOui00, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_NONE;
    }
    else if (memcmp(cipher_suite , ccpRSNOui01, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;
    }
    else if (memcmp(cipher_suite , ccpRSNOui05, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;
    }
    else
    {
        cipher_type = eCSR_ENCRYPT_TYPE_FAILED;
    }
    hddLog(LOG1, FL("cipher_type: %d"), cipher_type);
    return cipher_type;
}
/* To find if the MAC address is NULL */
static tANI_U8 hdd_IsMACAddrNULL (tANI_U8 *macAddr, tANI_U8 length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        if (0x00 != (macAddr[i]))
        {
            return FALSE;
        }
    }
    return TRUE;
} /****** end hdd_IsMACAddrNULL() ******/

eCsrEncryptionType
hdd_TranslateWPAToCsrEncryptionType(u_int8_t cipher_suite[4])
{
    eCsrEncryptionType cipher_type;
    // is the cipher type supported?
    if ( memcmp(cipher_suite , ccpWpaOui04, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_AES;
    } else
    if (memcmp(cipher_suite , ccpWpaOui02, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_TKIP;
    } else
    if (memcmp(cipher_suite , ccpWpaOui00, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_NONE;
    } else
    if (memcmp(cipher_suite , ccpWpaOui01, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;
    } else
    if (memcmp(cipher_suite , ccpWpaOui05, 4) == 0)
    {
        cipher_type = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;
    } else
    {
        cipher_type = eCSR_ENCRYPT_TYPE_FAILED;
    }
    hddLog(LOG1, FL("cipher_type: %d"), cipher_type);
    return cipher_type;
}

static tANI_S32 hdd_ProcessGENIE(hdd_adapter_t *pAdapter,
                struct ether_addr *pBssid,
                eCsrEncryptionType *pEncryptType,
                eCsrEncryptionType *mcEncryptType,
                eCsrAuthType *pAuthType,
#ifdef WLAN_FEATURE_11W
                u_int8_t *pMfpRequired,
                u_int8_t *pMfpCapable,
#endif
                u_int16_t gen_ie_len,
                u_int8_t *gen_ie)
{
    tHalHandle halHandle = WLAN_HDD_GET_HAL_CTX(pAdapter);
    eHalStatus result;
    tDot11fIERSN dot11RSNIE = {0};
    tDot11fIEWPA dot11WPAIE = {0};
    tANI_U32 i;
    tANI_U32 status;
    tANI_U8 *pRsnIe;
    tANI_U16 RSNIeLen;
    tPmkidCacheInfo PMKIDCache[4]; // Local transfer memory
    v_BOOL_t updatePMKCache = FALSE;

    /* Clear struct of tDot11fIERSN and tDot11fIEWPA specifically setting present
       flag to 0 */
    memset( &dot11WPAIE, 0 , sizeof(tDot11fIEWPA) );
    memset( &dot11RSNIE, 0 , sizeof(tDot11fIERSN) );

    // Type check
    if ( gen_ie[0] ==  DOT11F_EID_RSN)
    {
        // Validity checks
        if ((gen_ie_len < DOT11F_IE_RSN_MIN_LEN ) ||
                (gen_ie_len > DOT11F_IE_RSN_MAX_LEN) )
        {
            hddLog(LOGE, "%s: Invalid DOT11F RSN IE length :%d\n",
                   __func__, gen_ie_len);
            return -EINVAL;
        }
        // Skip past the EID byte and length byte
        pRsnIe = gen_ie + 2;
        RSNIeLen = gen_ie_len - 2;
        // Unpack the RSN IE
        status = sme_unpack_rsn_ie(halHandle,
                                   pRsnIe,
                                   RSNIeLen,
                                   &dot11RSNIE);
        if (DOT11F_FAILED(status))
        {
            hddLog(LOGE,
                       FL("Parse failure in hdd_ProcessGENIE (0x%08x)"),
                       status);
            return -EINVAL;
        }
        hddLog(LOG1, FL("%s: gp_cipher_suite_present: %d"),
                __func__, dot11RSNIE.gp_cipher_suite_present);

        // Copy out the encryption and authentication types
        hddLog(LOG1, FL("%s: pairwise cipher suite count: %d"),
                __func__, dot11RSNIE.pwise_cipher_suite_count );
        hddLog(LOG1, FL("%s: authentication suite count: %d"),
                __func__, dot11RSNIE.akm_suite_cnt);
        /*Here we have followed the apple base code,
          but probably I suspect we can do something different*/
        //dot11RSNIE.akm_suite_count
        // Just translate the FIRST one
        *pAuthType =  hdd_TranslateRSNToCsrAuthType(dot11RSNIE.akm_suite[0]);
        //dot11RSNIE.pwise_cipher_suite_count
        *pEncryptType = hdd_TranslateRSNToCsrEncryptionType(dot11RSNIE.pwise_cipher_suites[0]);
        //dot11RSNIE.gp_cipher_suite_count
        *mcEncryptType = hdd_TranslateRSNToCsrEncryptionType(dot11RSNIE.gp_cipher_suite);
#ifdef WLAN_FEATURE_11W
        *pMfpRequired = (dot11RSNIE.RSN_Cap[0] >> 6) & 0x1 ;
        *pMfpCapable = (dot11RSNIE.RSN_Cap[0] >> 7) & 0x1 ;
#endif
        // Set the PMKSA ID Cache for this interface
        for (i=0; i<dot11RSNIE.pmkid_count; i++)
        {
            if ( pBssid == NULL)
            {
                hddLog(LOGE, "%s: pBssid passed is NULL", __func__);
                break;
            }
            if ( hdd_IsMACAddrNULL( (u_char *) pBssid->ether_addr_octet , 6))
            {
                hddLog(LOGE, "%s: Invalid MAC adrr", __func__);
                break;
            }
            updatePMKCache = TRUE;
            // For right now, I assume setASSOCIATE() has passed in the bssid.
            vos_mem_copy(PMKIDCache[i].BSSID,
                            pBssid, ETHER_ADDR_LEN);
            vos_mem_copy(PMKIDCache[i].PMKID,
                            dot11RSNIE.pmkid[i],
                            CSR_RSN_PMKID_SIZE);
        }

        if (updatePMKCache)
        {
            // Calling csrRoamSetPMKIDCache to configure the PMKIDs into the cache
            hddLog(LOG1, FL("%s: Calling csrRoamSetPMKIDCache with cache entry %d."),
                                                                            __func__, i );
            // Finally set the PMKSA ID Cache in CSR
            result = sme_RoamSetPMKIDCache(halHandle,pAdapter->sessionId,
                                           PMKIDCache,
                                           dot11RSNIE.pmkid_count,
                                           FALSE);
        }
    }
    else if (gen_ie[0] == DOT11F_EID_WPA)
    {
        // Validity checks
        if ((gen_ie_len < DOT11F_IE_WPA_MIN_LEN ) ||
                    (gen_ie_len > DOT11F_IE_WPA_MAX_LEN))
        {
            hddLog(LOGE, "%s: Invalid DOT11F WPA IE length :%d\n",
                   __func__, gen_ie_len);
            return -EINVAL;
        }
        // Skip past the EID byte and length byte - and four byte WiFi OUI
        pRsnIe = gen_ie + 2 + 4;
        RSNIeLen = gen_ie_len - (2 + 4);
        // Unpack the WPA IE
        status = dot11fUnpackIeWPA((tpAniSirGlobal) halHandle,
                               pRsnIe, RSNIeLen, &dot11WPAIE);
        if (!DOT11F_SUCCEEDED(status))
        {
            hddLog(LOGE,
                   FL("Invalid RSN IE: parse status %d"), status);
            return -EINVAL;
        }

        // Copy out the encryption and authentication types
        hddLog(LOG1, FL("%s: WPA unicast cipher suite count: %d"),
               __func__, dot11WPAIE.unicast_cipher_count );
        hddLog(LOG1, FL("%s: WPA authentication suite count: %d"),
               __func__, dot11WPAIE.auth_suite_count);
        //dot11WPAIE.auth_suite_count
        // Just translate the FIRST one
        *pAuthType =  hdd_TranslateWPAToCsrAuthType(dot11WPAIE.auth_suites[0]);
        //dot11WPAIE.unicast_cipher_count
        *pEncryptType = hdd_TranslateWPAToCsrEncryptionType(dot11WPAIE.unicast_ciphers[0]);
        //dot11WPAIE.unicast_cipher_count
        *mcEncryptType = hdd_TranslateWPAToCsrEncryptionType(dot11WPAIE.multicast_cipher);
    }
    else
    {
        hddLog(LOGW, FL("gen_ie[0]: %d"), gen_ie[0]);
        return -EINVAL;
    }
    return 0;
}

/**
 * hdd_set_def_rsne_override() - set default encryption type and auth type
 * in profile.
 * @roam_profile: pointer to adapter
 * @auth_type: pointer to auth type
 *
 * Set default value of encryption type and auth type in profile to
 * search the AP using filter, as in force_rsne_override the RSNIE can be
 * currupt and we might not get the proper encryption type and auth type
 * while parsing the RSNIE.
 *
 * Return: void
 */
static void hdd_set_def_rsne_override(
                tCsrRoamProfile *roam_profile, eCsrAuthType *auth_type)
{
       hddLog( LOG1, FL("Set def values in roam profile"));
       roam_profile->MFPCapable = roam_profile->MFPEnabled;
       roam_profile->EncryptionType.numEntries = 2;
       roam_profile->mcEncryptionType.numEntries = 2;
       /* Use the cipher type in the RSN IE */
       roam_profile->EncryptionType.encryptionType[0] = eCSR_ENCRYPT_TYPE_AES;
       roam_profile->EncryptionType.encryptionType[1] = eCSR_ENCRYPT_TYPE_TKIP;
       roam_profile->mcEncryptionType.encryptionType[0] =
              eCSR_ENCRYPT_TYPE_AES;
       roam_profile->mcEncryptionType.encryptionType[1] =
              eCSR_ENCRYPT_TYPE_TKIP;
       *auth_type = eCSR_AUTH_TYPE_RSN_PSK;
}

int hdd_SetGENIEToCsr( hdd_adapter_t *pAdapter, eCsrAuthType *RSNAuthType)
{
    hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    v_U32_t status = 0;
    eCsrEncryptionType RSNEncryptType;
    eCsrEncryptionType mcRSNEncryptType;
    hdd_context_t *hdd_ctx;
#ifdef WLAN_FEATURE_11W
    u_int8_t RSNMfpRequired = 0;
    u_int8_t RSNMfpCapable = 0;
#endif
    struct ether_addr   bSsid;   // MAC address of assoc peer
    // MAC address of assoc peer
    // But, this routine is only called when we are NOT associated.
    vos_mem_copy(bSsid.ether_addr_octet,
            pWextState->roamProfile.BSSIDs.bssid,
            sizeof(bSsid.ether_addr_octet));
    if (pWextState->WPARSNIE[0] == DOT11F_EID_RSN || pWextState->WPARSNIE[0] == DOT11F_EID_WPA)
    {
        //continue
    }
    else
    {
        return 0;
    }

    // The actual processing may eventually be more extensive than this.
    // Right now, just consume any PMKIDs that are  sent in by the app.
    status = hdd_ProcessGENIE(pAdapter,
            &bSsid,   // MAC address of assoc peer
            &RSNEncryptType,
            &mcRSNEncryptType,
            RSNAuthType,
#ifdef WLAN_FEATURE_11W
            &RSNMfpRequired,
            &RSNMfpCapable,
#endif
            pWextState->WPARSNIE[1]+2,
            pWextState->WPARSNIE);

    if (status == 0)
    {
        // Now copy over all the security attributes you have parsed out
        pWextState->roamProfile.EncryptionType.numEntries = 1;
        pWextState->roamProfile.mcEncryptionType.numEntries = 1;

        pWextState->roamProfile.EncryptionType.encryptionType[0] = RSNEncryptType; // Use the cipher type in the RSN IE
        pWextState->roamProfile.mcEncryptionType.encryptionType[0] = mcRSNEncryptType;

        if ( (WLAN_HDD_IBSS == pAdapter->device_mode) &&
             ((eCSR_ENCRYPT_TYPE_AES == mcRSNEncryptType) ||
             (eCSR_ENCRYPT_TYPE_TKIP == mcRSNEncryptType)))
        {
           /*For wpa none supplicant sends the WPA IE with unicast cipher as
             eCSR_ENCRYPT_TYPE_NONE ,where as the multicast cipher as
             either AES/TKIP based on group cipher configuration
             mentioned in the wpa_supplicant.conf.*/

           /*Set the unicast cipher same as multicast cipher*/
           pWextState->roamProfile.EncryptionType.encryptionType[0]
                                                     = mcRSNEncryptType;
        }

#ifdef WLAN_FEATURE_11W
        hddLog( LOG1, FL("RSNMfpRequired = %d, RSNMfpCapable = %d"),
                                       RSNMfpRequired, RSNMfpCapable);
        pWextState->roamProfile.MFPRequired = RSNMfpRequired;
        pWextState->roamProfile.MFPCapable = RSNMfpCapable;
#endif
        hddLog( LOG1,
          FL("CSR AuthType = %d, EncryptionType = %d mcEncryptionType = %d"),
          *RSNAuthType, RSNEncryptType, mcRSNEncryptType);
        return 0;
    }

    hdd_ctx = WLAN_HDD_GET_CTX(pAdapter);
    if (hdd_ctx->force_rsne_override &&
        (pWextState->WPARSNIE[0] == DOT11F_EID_RSN)) {
            hddLog(LOG1,
              FL("Test mode enabled set def Auth and enc type. RSN IE passed in connect req:"));
            vos_trace_hex_dump(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_WARN,
                               pWextState->roamProfile.pRSNReqIE,
                               pWextState->roamProfile.nRSNReqIELength);

            pWextState->roamProfile.force_rsne_override = true;
            hddLog(LOG1,
                   FL("MFPEnabled %d"), pWextState->roamProfile.MFPEnabled);
            /*
             * Reset MFPEnabled if testmode RSNE passed doesnt have MFPR
             * or MFPC bit set
             */
            if (pWextState->roamProfile.MFPEnabled &&
                !(pWextState->roamProfile.MFPRequired ||
                  pWextState->roamProfile.MFPCapable)) {
                    hddLog( LOG1,FL("Reset MFPEnabled"));
                    pWextState->roamProfile.MFPEnabled = 0;
            }

            /* If parsing failed set the def value for the roam profile */
            if (status)
                hdd_set_def_rsne_override(&pWextState->roamProfile,
                                          RSNAuthType);
    }
    return status;
}
int hdd_set_csr_auth_type ( hdd_adapter_t  *pAdapter, eCsrAuthType RSNAuthType)
{
    hdd_wext_state_t *pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    tCsrRoamProfile* pRoamProfile = &(pWextState->roamProfile);
    hdd_station_ctx_t *pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    ENTER();

    pRoamProfile->AuthType.numEntries = 1;
    hddLog( LOG1, "%s: pHddStaCtx->conn_info.authType = %d", __func__, pHddStaCtx->conn_info.authType);

    switch( pHddStaCtx->conn_info.authType)
    {
       case eCSR_AUTH_TYPE_OPEN_SYSTEM:
       case eCSR_AUTH_TYPE_AUTOSWITCH:
#ifdef FEATURE_WLAN_ESE
       case eCSR_AUTH_TYPE_CCKM_WPA:
       case eCSR_AUTH_TYPE_CCKM_RSN:
#endif
        if (pWextState->wpaVersion & IW_AUTH_WPA_VERSION_DISABLED) {

           pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_OPEN_SYSTEM ;
        } else
        if (pWextState->wpaVersion & IW_AUTH_WPA_VERSION_WPA) {

#ifdef FEATURE_WLAN_ESE
            if ((RSNAuthType == eCSR_AUTH_TYPE_CCKM_WPA) &&
                ((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)
                 == IW_AUTH_KEY_MGMT_802_1X)) {
                hddLog( LOG1, "%s: set authType to CCKM WPA. AKM also 802.1X.", __func__);
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_CCKM_WPA;
            } else
            if ((RSNAuthType == eCSR_AUTH_TYPE_CCKM_WPA)) {
                hddLog( LOG1, "%s: Last chance to set authType to CCKM WPA.", __func__);
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_CCKM_WPA;
            } else
#endif
            if((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)
                    == IW_AUTH_KEY_MGMT_802_1X) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_WPA;
            } else
            if ((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_PSK)
                    == IW_AUTH_KEY_MGMT_PSK) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_WPA_PSK;
            } else {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_WPA_NONE;
            }
        }
        if (pWextState->wpaVersion & IW_AUTH_WPA_VERSION_WPA2) {
#ifdef FEATURE_WLAN_ESE
            if ((RSNAuthType == eCSR_AUTH_TYPE_CCKM_RSN) &&
                ((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)
                 == IW_AUTH_KEY_MGMT_802_1X)) {
                hddLog( LOG1, "%s: set authType to CCKM RSN. AKM also 802.1X.", __func__);
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_CCKM_RSN;
            } else
            if ((RSNAuthType == eCSR_AUTH_TYPE_CCKM_RSN)) {
                hddLog( LOG1, "%s: Last chance to set authType to CCKM RSN.", __func__);
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_CCKM_RSN;
            } else
#endif

#ifdef WLAN_FEATURE_VOWIFI_11R
            if ((RSNAuthType == eCSR_AUTH_TYPE_FT_RSN) &&
                ((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)
                 == IW_AUTH_KEY_MGMT_802_1X)) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_FT_RSN;
            }else
            if ((RSNAuthType == eCSR_AUTH_TYPE_FT_RSN_PSK) &&
                ((pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_PSK)
                 == IW_AUTH_KEY_MGMT_PSK)) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_FT_RSN_PSK;
            } else
#endif

#ifdef WLAN_FEATURE_11W
            if (RSNAuthType == eCSR_AUTH_TYPE_RSN_PSK_SHA256) {
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_RSN_PSK_SHA256;
            } else
            if (RSNAuthType == eCSR_AUTH_TYPE_RSN_8021X_SHA256) {
                pRoamProfile->AuthType.authType[0] =
                                            eCSR_AUTH_TYPE_RSN_8021X_SHA256;
            } else
#endif

            if( (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)
                    == IW_AUTH_KEY_MGMT_802_1X) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_RSN;
            } else
            if ( (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_PSK)
                    == IW_AUTH_KEY_MGMT_PSK) {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_RSN_PSK;
            } else {
               pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_UNKNOWN;
            }
        }
        break;

       case eCSR_AUTH_TYPE_SHARED_KEY:

          pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_SHARED_KEY;
          break;
        default:

#ifdef FEATURE_WLAN_ESE
           hddLog( LOG1, "%s: In default, unknown auth type.", __func__);
#endif /* FEATURE_WLAN_ESE */
           pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_UNKNOWN;
           break;
    }

    hddLog( LOG1, "%s Set roam Authtype to %d",
            __func__, pWextState->roamProfile.AuthType.authType[0]);

   EXIT();
    return 0;
}

/**
 * hdd_rx_fwd_eapol() - forward cached eapol frames
 * @vosContext : pointer to vos global context
 * @pVosPacket: pointer to vos packet
 *
 * Return: None
 *
 */
void hdd_assoc_registerFwdEapolCB(void *pContext)
{
     WLANTL_RegisterFwdEapol(pContext, hdd_rx_fwd_eapol);
}

/**---------------------------------------------------------------------------

  \brief __iw_set_essid() -
   This function sets the ssid received from wpa_supplicant
   to the CSR roam profile.

  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/

int __iw_set_essid(struct net_device *dev,
                   struct iw_request_info *info,
                   union iwreq_data *wrqu, char *extra)
{
    v_U32_t status = 0;
    hdd_wext_state_t *pWextState;
    hdd_adapter_t *pAdapter;
    hdd_context_t *pHddCtx;
    v_U32_t roamId;
    tCsrRoamProfile          *pRoamProfile;
    eCsrAuthType RSNAuthType;
    tHalHandle hHal;
    hdd_station_ctx_t *pHddStaCtx;
    int ret = 0;

    ENTER();
    pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
    if (NULL == pAdapter)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Adapter is NULL",__func__);
        return -EINVAL;
    }

    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
    {
        return ret;
    }

    hHal = WLAN_HDD_GET_HAL_CTX(pAdapter);
    if (NULL == hHal)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Hal Context is NULL",__func__);
        return -EINVAL;
    }
    pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    if (NULL == pHddStaCtx)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: STA Context is NULL",__func__);
        return -EINVAL;
    }
    pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    if (NULL == pWextState)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWextState is NULL",__func__);
        return -EINVAL;
    }
    if(pWextState->mTKIPCounterMeasures == TKIP_COUNTER_MEASURE_STARTED) {
        hddLog(VOS_TRACE_LEVEL_INFO_HIGH, "%s :Counter measure is in progress", __func__);
        return -EBUSY;
    }
    if( SIR_MAC_MAX_SSID_LENGTH < wrqu->essid.length )
        return -EINVAL;
    pRoamProfile = &pWextState->roamProfile;

    if (wlan_hdd_check_and_stop_mon(pAdapter, true))
        return -EINVAL;

    /*Try disconnecting if already in connected state*/
    status = wlan_hdd_try_disconnect(pAdapter);
    if (0 > status)
    {
      hddLog(VOS_TRACE_LEVEL_ERROR, FL("Failed to disconnect the existing"
            " connection"));
      return -EALREADY;
    }
    /** wpa_supplicant 0.8.x, wext driver uses */
    /** when cfg80211 defined, wpa_supplicant wext driver uses
      zero-length, null-string ssid for force disconnection.
      after disconnection (if previously connected) and cleaning ssid,
      driver MUST return success */
    if ( 0 == wrqu->essid.length ) {
        return 0;
    }

    status = hdd_wmm_get_uapsd_mask(pAdapter,
                                    &pWextState->roamProfile.uapsd_mask);
    if (VOS_STATUS_SUCCESS != status)
    {
       pWextState->roamProfile.uapsd_mask = 0;
    }
    pWextState->roamProfile.SSIDs.numOfSSIDs = 1;

    pWextState->roamProfile.SSIDs.SSIDList->SSID.length = wrqu->essid.length;

    vos_mem_zero(pWextState->roamProfile.SSIDs.SSIDList->SSID.ssId, sizeof(pWextState->roamProfile.SSIDs.SSIDList->SSID.ssId));
    vos_mem_copy((void *)(pWextState->roamProfile.SSIDs.SSIDList->SSID.ssId), extra, wrqu->essid.length);
    if (IW_AUTH_WPA_VERSION_WPA == pWextState->wpaVersion ||
        IW_AUTH_WPA_VERSION_WPA2 == pWextState->wpaVersion ) {

        //set gen ie
        hdd_SetGENIEToCsr(pAdapter, &RSNAuthType);

        //set auth
        hdd_set_csr_auth_type(pAdapter, RSNAuthType);
    }
#ifdef FEATURE_WLAN_WAPI
    hddLog(LOG1, "%s: Setting WAPI AUTH Type and Encryption Mode values", __func__);
    if (pAdapter->wapi_info.nWapiMode)
    {
        switch (pAdapter->wapi_info.wapiAuthMode)
        {
            case WAPI_AUTH_MODE_PSK:
            {
                hddLog(LOG1, "%s: WAPI AUTH TYPE: PSK: %d", __func__, pAdapter->wapi_info.wapiAuthMode);
                pRoamProfile->AuthType.numEntries = 1;
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_WAPI_WAI_PSK;
                break;
            }
            case WAPI_AUTH_MODE_CERT:
            {
                hddLog(LOG1, "%s: WAPI AUTH TYPE: CERT: %d", __func__, pAdapter->wapi_info.wapiAuthMode);
                pRoamProfile->AuthType.numEntries = 1;
                pRoamProfile->AuthType.authType[0] = eCSR_AUTH_TYPE_WAPI_WAI_CERTIFICATE;
                break;
            }
        } // End of switch
        if ( pAdapter->wapi_info.wapiAuthMode == WAPI_AUTH_MODE_PSK ||
             pAdapter->wapi_info.wapiAuthMode == WAPI_AUTH_MODE_CERT)
        {
            hddLog(LOG1, "%s: WAPI PAIRWISE/GROUP ENCRYPTION: WPI", __func__);
            pRoamProfile->EncryptionType.numEntries = 1;
            pRoamProfile->EncryptionType.encryptionType[0] = eCSR_ENCRYPT_TYPE_WPI;
            pRoamProfile->mcEncryptionType.numEntries = 1;
            pRoamProfile->mcEncryptionType.encryptionType[0] = eCSR_ENCRYPT_TYPE_WPI;
        }
    }
#endif /* FEATURE_WLAN_WAPI */
    /* if previous genIE is not NULL, update AssocIE */
    if (0 != pWextState->genIE.length)
    {
        memset( &pWextState->assocAddIE, 0, sizeof(pWextState->assocAddIE) );
        memcpy( pWextState->assocAddIE.addIEdata, pWextState->genIE.addIEdata,
            pWextState->genIE.length);
        pWextState->assocAddIE.length = pWextState->genIE.length;
        pWextState->roamProfile.pAddIEAssoc = pWextState->assocAddIE.addIEdata;
        pWextState->roamProfile.nAddIEAssocLength = pWextState->assocAddIE.length;

        /* clear previous genIE after use it */
        memset( &pWextState->genIE, 0, sizeof(pWextState->genIE) );
    }

    /* assumes it is not WPS Association by default, except when pAddIEAssoc has WPS IE */
    pWextState->roamProfile.bWPSAssociation = FALSE;

    if (NULL != wlan_hdd_get_wps_ie_ptr(pWextState->roamProfile.pAddIEAssoc,
        pWextState->roamProfile.nAddIEAssocLength))
        pWextState->roamProfile.bWPSAssociation = TRUE;


    // Disable auto BMPS entry by PMC until DHCP is done
    sme_SetDHCPTillPowerActiveFlag(WLAN_HDD_GET_HAL_CTX(pAdapter), TRUE);

    pWextState->roamProfile.csrPersona = pAdapter->device_mode;
    (WLAN_HDD_GET_CTX(pAdapter))->isAmpAllowed = VOS_FALSE;

    if ( eCSR_BSS_TYPE_START_IBSS == pRoamProfile->BSSType )
    {
        hdd_select_cbmode(pAdapter,
            (WLAN_HDD_GET_CTX(pAdapter))->cfg_ini->AdHocChannel5G);
    }
   /*
    * Change conn_state to connecting before sme_RoamConnect(),
    * because sme_RoamConnect() has a direct path to call
    * hdd_smeRoamCallback(), which will change the conn_state
    * If direct path, conn_state will be accordingly changed
    * to NotConnected or Associated by either
    * hdd_AssociationCompletionHandler() or hdd_DisConnectHandler()
    * in sme_RoamCallback()
    * if sme_RomConnect is to be queued,
    * Connecting state will remain until it is completed.
    *
    * If connection state is not changed,
    * connection state will remain in eConnectionState_NotConnected state.
    * In hdd_AssociationCompletionHandler, "hddDisconInProgress" is set to true
    * if conn state is eConnectionState_NotConnected.
    * If "hddDisconInProgress" is set to true then cfg80211 layer is not
    * informed of connect result indication which is an issue.
    */
    if (WLAN_HDD_INFRA_STATION == pAdapter->device_mode ||
            WLAN_HDD_P2P_CLIENT == pAdapter->device_mode)
    {
        VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                   FL("Set HDD connState to eConnectionState_Connecting"));
        hdd_connSetConnectionState(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter),
                                                 eConnectionState_Connecting);
    }
    status = sme_RoamConnect( hHal,pAdapter->sessionId,
                         &(pWextState->roamProfile), &roamId);

    if ((eHAL_STATUS_SUCCESS != status) &&
        (WLAN_HDD_INFRA_STATION == pAdapter->device_mode ||
        WLAN_HDD_P2P_CLIENT == pAdapter->device_mode))
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               FL("sme_RoamConnect (session %d) failed with status %d. -> NotConnected"),
                            pAdapter->sessionId, status);
            /* change back to NotAssociated */
        hdd_connSetConnectionState(WLAN_HDD_GET_STATION_CTX_PTR(pAdapter),
                                             eConnectionState_NotConnected);
    }
    pRoamProfile->ChannelInfo.ChannelList = NULL;
    pRoamProfile->ChannelInfo.numOfChannels = 0;

    EXIT();
    return status;
}

int iw_set_essid(struct net_device *dev,
                 struct iw_request_info *info,
                 union iwreq_data *wrqu, char *extra)
{
    int ret;

    vos_ssr_protect(__func__);
    ret = __iw_set_essid(dev, info, wrqu, extra);
    vos_ssr_unprotect(__func__);

    return ret;
}

/**---------------------------------------------------------------------------

  \brief __iw_get_essid() -
   This function returns the essid to the wpa_supplicant.

  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
int __iw_get_essid(struct net_device *dev,
                   struct iw_request_info *info,
                   struct iw_point *dwrq, char *extra)
{
   hdd_adapter_t *pAdapter;
   hdd_context_t *pHddCtx;
   hdd_wext_state_t *wextBuf;
   hdd_station_ctx_t *pHddStaCtx;
   int ret = 0;

   ENTER();

   pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   if (NULL == pAdapter)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Adapter is NULL",__func__);
       return -EINVAL;
   }

   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   ret = wlan_hdd_validate_context(pHddCtx);
   if (0 != ret)
   {
       return ret;
   }

   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   if (NULL == pHddStaCtx)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: STA Context is NULL",__func__);
       return -EINVAL;
   }

   wextBuf = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
   if (NULL == wextBuf)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: wextBuf is NULL",__func__);
       return -EINVAL;
   }

   if((pHddStaCtx->conn_info.connState == eConnectionState_Associated &&
     wextBuf->roamProfile.SSIDs.SSIDList->SSID.length > 0) ||
      ((pHddStaCtx->conn_info.connState == eConnectionState_IbssConnected ||
        pHddStaCtx->conn_info.connState == eConnectionState_IbssDisconnected) &&
        wextBuf->roamProfile.SSIDs.SSIDList->SSID.length > 0))
   {
       dwrq->length = pHddStaCtx->conn_info.SSID.SSID.length;
       memcpy(extra, pHddStaCtx->conn_info.SSID.SSID.ssId, dwrq->length);
       dwrq->flags = 1;
   } else {
       memset(extra, 0, dwrq->length);
       dwrq->length = 0;
       dwrq->flags = 0;
   }
   EXIT();
   return 0;
}

int iw_get_essid(struct net_device *dev,
                 struct iw_request_info *info,
                 struct iw_point *dwrq, char *extra)
{
   int ret;

   vos_ssr_protect(__func__);
   ret = __iw_get_essid(dev, info, dwrq, extra);
   vos_ssr_unprotect(__func__);

   return ret;
}
/**---------------------------------------------------------------------------

  \brief __iw_set_auth() -
   This function sets the auth type received from the wpa_supplicant.

  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
int __iw_set_auth(struct net_device *dev,struct iw_request_info *info,
                  union iwreq_data *wrqu,char *extra)
{
   hdd_adapter_t *pAdapter;
   hdd_context_t *pHddCtx;
   hdd_wext_state_t *pWextState;
   hdd_station_ctx_t *pHddStaCtx;
   tCsrRoamProfile *pRoamProfile;
   eCsrEncryptionType mcEncryptionType;
   eCsrEncryptionType ucEncryptionType;
   int ret = 0;

   ENTER();

   pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
   if (NULL == pAdapter)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: Adapter is NULL",__func__);
       return -EINVAL;
   }

   pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
   ret = wlan_hdd_validate_context(pHddCtx);
   if (0 != ret)
   {
       return ret;
   }

   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   if (NULL == pHddStaCtx)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: STA Context is NULL",__func__);
       return -EINVAL;
   }

   pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
   if (NULL == pWextState)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                 "%s: pWextState is NULL",__func__);
       return -EINVAL;
   }

   pRoamProfile = &pWextState->roamProfile;

   switch(wrqu->param.flags & IW_AUTH_INDEX)
   {
      case IW_AUTH_WPA_VERSION:

         pWextState->wpaVersion = wrqu->param.value;

         break;

   case IW_AUTH_CIPHER_PAIRWISE:
   {
      if(wrqu->param.value & IW_AUTH_CIPHER_NONE) {
         ucEncryptionType = eCSR_ENCRYPT_TYPE_NONE;
      }
      else if(wrqu->param.value & IW_AUTH_CIPHER_TKIP) {
         ucEncryptionType = eCSR_ENCRYPT_TYPE_TKIP;
      }
      else if(wrqu->param.value & IW_AUTH_CIPHER_CCMP) {
         ucEncryptionType = eCSR_ENCRYPT_TYPE_AES;
      }

     else if(wrqu->param.value & IW_AUTH_CIPHER_WEP40) {

         if( (IW_AUTH_KEY_MGMT_802_1X
                     == (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X)  )
                 && (eCSR_AUTH_TYPE_OPEN_SYSTEM == pHddStaCtx->conn_info.authType) )
                /*Dynamic WEP key*/
             ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP40;
         else
                /*Static WEP key*/
             ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;
      }
      else if(wrqu->param.value & IW_AUTH_CIPHER_WEP104) {

         if( ( IW_AUTH_KEY_MGMT_802_1X
                     == (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X) )
                 && (eCSR_AUTH_TYPE_OPEN_SYSTEM == pHddStaCtx->conn_info.authType))
                  /*Dynamic WEP key*/
            ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP104;
         else
                /*Static WEP key*/
            ucEncryptionType = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;

         }
         else {

               hddLog(LOGW, "%s value %d UNKNOWN IW_AUTH_CIPHER",
                      __func__, wrqu->param.value);
               return -EINVAL;
         }

         pRoamProfile->EncryptionType.numEntries = 1;
         pRoamProfile->EncryptionType.encryptionType[0] = ucEncryptionType;
      }
      break;
      case IW_AUTH_CIPHER_GROUP:
      {
          if(wrqu->param.value & IW_AUTH_CIPHER_NONE) {
            mcEncryptionType = eCSR_ENCRYPT_TYPE_NONE;
      }

      else if(wrqu->param.value & IW_AUTH_CIPHER_TKIP) {
             mcEncryptionType = eCSR_ENCRYPT_TYPE_TKIP;
      }

      else if(wrqu->param.value & IW_AUTH_CIPHER_CCMP) {
              mcEncryptionType = eCSR_ENCRYPT_TYPE_AES;
      }

      else if(wrqu->param.value & IW_AUTH_CIPHER_WEP40) {

         if( ( IW_AUTH_KEY_MGMT_802_1X
                     == (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X ))
                 && (eCSR_AUTH_TYPE_OPEN_SYSTEM == pHddStaCtx->conn_info.authType))

            mcEncryptionType = eCSR_ENCRYPT_TYPE_WEP40;

         else
               mcEncryptionType = eCSR_ENCRYPT_TYPE_WEP40_STATICKEY;
      }

      else if(wrqu->param.value & IW_AUTH_CIPHER_WEP104)
      {
             /*Dynamic WEP keys won't work with shared keys*/
         if( ( IW_AUTH_KEY_MGMT_802_1X
                     == (pWextState->authKeyMgmt & IW_AUTH_KEY_MGMT_802_1X))
                 && (eCSR_AUTH_TYPE_OPEN_SYSTEM == pHddStaCtx->conn_info.authType))
         {
            mcEncryptionType = eCSR_ENCRYPT_TYPE_WEP104;
         }
         else
         {
            mcEncryptionType = eCSR_ENCRYPT_TYPE_WEP104_STATICKEY;
         }
      }
      else {

          hddLog(LOGW, "%s value %d UNKNOWN IW_AUTH_CIPHER",
                 __func__, wrqu->param.value);
          return -EINVAL;
       }

         pRoamProfile->mcEncryptionType.numEntries = 1;
         pRoamProfile->mcEncryptionType.encryptionType[0] = mcEncryptionType;
      }
      break;

      case IW_AUTH_80211_AUTH_ALG:
      {
           /*Save the auth algo here and set auth type to SME Roam profile
                in the iw_set_ap_address*/
          if( wrqu->param.value & IW_AUTH_ALG_OPEN_SYSTEM)
             pHddStaCtx->conn_info.authType = eCSR_AUTH_TYPE_OPEN_SYSTEM;

          else if(wrqu->param.value & IW_AUTH_ALG_SHARED_KEY)
             pHddStaCtx->conn_info.authType = eCSR_AUTH_TYPE_SHARED_KEY;

          else if(wrqu->param.value & IW_AUTH_ALG_LEAP)
            /*Not supported*/
             pHddStaCtx->conn_info.authType = eCSR_AUTH_TYPE_OPEN_SYSTEM;
          pWextState->roamProfile.AuthType.authType[0] = pHddStaCtx->conn_info.authType;
      }
      break;

      case IW_AUTH_KEY_MGMT:
      {
#ifdef FEATURE_WLAN_ESE
#define IW_AUTH_KEY_MGMT_CCKM       8  /* Should be in linux/wireless.h */
         /*Check for CCKM AKM type */
         if ( wrqu->param.value & IW_AUTH_KEY_MGMT_CCKM) {
            hddLog(VOS_TRACE_LEVEL_INFO,"%s: CCKM AKM Set %d",
                   __func__, wrqu->param.value);
            /* Set the CCKM bit in authKeyMgmt */
            /* Right now, this breaks all ref to authKeyMgmt because our
             * code doesn't realize it is a "bitfield"
             */
            pWextState->authKeyMgmt |= IW_AUTH_KEY_MGMT_CCKM;
            /*Set the key management to 802.1X*/
            //pWextState->authKeyMgmt = IW_AUTH_KEY_MGMT_802_1X;
            pWextState->isESEConnection = eANI_BOOLEAN_TRUE;
            //This is test code. I need to actually KNOW whether this is an RSN Assoc or WPA.
            pWextState->collectedAuthType = eCSR_AUTH_TYPE_CCKM_RSN;
         } else if ( wrqu->param.value & IW_AUTH_KEY_MGMT_PSK) {
            /*Save the key management*/
            pWextState->authKeyMgmt |= IW_AUTH_KEY_MGMT_PSK;
            //pWextState->authKeyMgmt = wrqu->param.value;
            //This is test code. I need to actually KNOW whether this is an RSN Assoc or WPA.
            pWextState->collectedAuthType = eCSR_AUTH_TYPE_RSN;
         } else if (!( wrqu->param.value & IW_AUTH_KEY_MGMT_802_1X)) {
            pWextState->collectedAuthType = eCSR_AUTH_TYPE_NONE; //eCSR_AUTH_TYPE_WPA_NONE
            /*Save the key management anyway*/
            pWextState->authKeyMgmt = wrqu->param.value;
         } else { // It must be IW_AUTH_KEY_MGMT_802_1X
            /*Save the key management*/
            pWextState->authKeyMgmt |= IW_AUTH_KEY_MGMT_802_1X;
            //pWextState->authKeyMgmt = wrqu->param.value;
            //This is test code. I need to actually KNOW whether this is an RSN Assoc or WPA.
            pWextState->collectedAuthType = eCSR_AUTH_TYPE_RSN;
         }
#else
         /*Save the key management*/
         pWextState->authKeyMgmt = wrqu->param.value;
#endif /* FEATURE_WLAN_ESE */
      }
      break;

      case IW_AUTH_TKIP_COUNTERMEASURES:
      {
         if(wrqu->param.value) {
            hddLog(VOS_TRACE_LEVEL_INFO_HIGH,
                   "Counter Measure started %d", wrqu->param.value);
            pWextState->mTKIPCounterMeasures = TKIP_COUNTER_MEASURE_STARTED;
         }
         else {
            hddLog(VOS_TRACE_LEVEL_INFO_HIGH,
                   "Counter Measure stopped=%d", wrqu->param.value);
            pWextState->mTKIPCounterMeasures = TKIP_COUNTER_MEASURE_STOPED;
         }
      }
      break;
      case IW_AUTH_DROP_UNENCRYPTED:
      case IW_AUTH_WPA_ENABLED:
      case IW_AUTH_RX_UNENCRYPTED_EAPOL:
      case IW_AUTH_ROAMING_CONTROL:
      case IW_AUTH_PRIVACY_INVOKED:

      default:

         hddLog(LOGW, "%s called with unsupported auth type %d", __func__,
               wrqu->param.flags & IW_AUTH_INDEX);
      break;
   }

   EXIT();
   return 0;
}

int iw_set_auth(struct net_device *dev, struct iw_request_info *info,
                union iwreq_data *wrqu, char *extra)
{
   int ret;

   vos_ssr_protect(__func__);
   ret = __iw_set_auth(dev, info, wrqu, extra);
   vos_ssr_unprotect(__func__);

   return ret;
}

/**---------------------------------------------------------------------------

  \brief __iw_get_auth() -
   This function returns the auth type to the wpa_supplicant.

  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
int __iw_get_auth(struct net_device *dev,struct iw_request_info *info,
                  union iwreq_data *wrqu,char *extra)
{
    hdd_adapter_t* pAdapter;
    hdd_wext_state_t *pWextState;
    tCsrRoamProfile *pRoamProfile;
    hdd_context_t *pHddCtx;
    int ret = 0;

    ENTER();

    pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
    if (NULL == pAdapter)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Adapter is NULL",__func__);
        return -EINVAL;
    }

    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
    {
        return ret;
    }

    pWextState = WLAN_HDD_GET_WEXT_STATE_PTR(pAdapter);
    if (NULL == pWextState)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: pWextState is NULL",__func__);
        return -EINVAL;
    }
    pRoamProfile = &pWextState->roamProfile;

    switch(pRoamProfile->negotiatedAuthType)
    {
        case eCSR_AUTH_TYPE_WPA_NONE:
            wrqu->param.flags = IW_AUTH_WPA_VERSION;
            wrqu->param.value =  IW_AUTH_WPA_VERSION_DISABLED;
            break;
        case eCSR_AUTH_TYPE_WPA:
            wrqu->param.flags = IW_AUTH_WPA_VERSION;
            wrqu->param.value = IW_AUTH_WPA_VERSION_WPA;
            break;
#ifdef WLAN_FEATURE_VOWIFI_11R
        case eCSR_AUTH_TYPE_FT_RSN:
#endif
        case eCSR_AUTH_TYPE_RSN:
            wrqu->param.flags = IW_AUTH_WPA_VERSION;
            wrqu->param.value =  IW_AUTH_WPA_VERSION_WPA2;
            break;
         case eCSR_AUTH_TYPE_OPEN_SYSTEM:
             wrqu->param.value = IW_AUTH_ALG_OPEN_SYSTEM;
             break;
         case eCSR_AUTH_TYPE_SHARED_KEY:
             wrqu->param.value =  IW_AUTH_ALG_SHARED_KEY;
             break;
         case eCSR_AUTH_TYPE_UNKNOWN:
             hddLog(LOG1,"%s called with unknown auth type", __func__);
             wrqu->param.value =  IW_AUTH_ALG_OPEN_SYSTEM;
             break;
         case eCSR_AUTH_TYPE_AUTOSWITCH:
             wrqu->param.value =  IW_AUTH_ALG_OPEN_SYSTEM;
             break;
         case eCSR_AUTH_TYPE_WPA_PSK:
             hddLog(LOG1,"%s called with WPA PSK auth type", __func__);
             wrqu->param.value = IW_AUTH_ALG_OPEN_SYSTEM;
             return -EIO;
#ifdef WLAN_FEATURE_VOWIFI_11R
         case eCSR_AUTH_TYPE_FT_RSN_PSK:
#endif
         case eCSR_AUTH_TYPE_RSN_PSK:
#ifdef WLAN_FEATURE_11W
         case eCSR_AUTH_TYPE_RSN_PSK_SHA256:
         case eCSR_AUTH_TYPE_RSN_8021X_SHA256:
#endif
             hddLog(LOG1,"%s called with RSN PSK auth type", __func__);
             wrqu->param.value = IW_AUTH_ALG_OPEN_SYSTEM;
             return -EIO;
         default:
             hddLog(LOGE,"%s called with unknown auth type", __func__);
             wrqu->param.value = IW_AUTH_ALG_OPEN_SYSTEM;
             return -EIO;
    }
    if(((wrqu->param.flags & IW_AUTH_INDEX) == IW_AUTH_CIPHER_PAIRWISE))
    {
        switch(pRoamProfile->negotiatedUCEncryptionType)
        {
            case eCSR_ENCRYPT_TYPE_NONE:
                wrqu->param.value = IW_AUTH_CIPHER_NONE;
                break;
            case eCSR_ENCRYPT_TYPE_WEP40:
            case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
                wrqu->param.value = IW_AUTH_CIPHER_WEP40;
                break;
            case eCSR_ENCRYPT_TYPE_TKIP:
                wrqu->param.value = IW_AUTH_CIPHER_TKIP;
                break;
            case eCSR_ENCRYPT_TYPE_WEP104:
            case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
                wrqu->param.value = IW_AUTH_CIPHER_WEP104;
                break;
            case eCSR_ENCRYPT_TYPE_AES:
                wrqu->param.value = IW_AUTH_CIPHER_CCMP;
                break;
            default:
                hddLog(LOG1, "%s called with unknown auth type %d ",
                         __func__, pRoamProfile->negotiatedUCEncryptionType);
                return -EIO;
        }
   }

    if(((wrqu->param.flags & IW_AUTH_INDEX) == IW_AUTH_CIPHER_GROUP))
    {
        switch(pRoamProfile->negotiatedMCEncryptionType)
        {
        case eCSR_ENCRYPT_TYPE_NONE:
            wrqu->param.value = IW_AUTH_CIPHER_NONE;
            break;
        case eCSR_ENCRYPT_TYPE_WEP40:
        case eCSR_ENCRYPT_TYPE_WEP40_STATICKEY:
            wrqu->param.value = IW_AUTH_CIPHER_WEP40;
            break;
        case eCSR_ENCRYPT_TYPE_TKIP:
            wrqu->param.value = IW_AUTH_CIPHER_TKIP;
            break;
         case eCSR_ENCRYPT_TYPE_WEP104:
         case eCSR_ENCRYPT_TYPE_WEP104_STATICKEY:
             wrqu->param.value = IW_AUTH_CIPHER_WEP104;
             break;
         case eCSR_ENCRYPT_TYPE_AES:
             wrqu->param.value = IW_AUTH_CIPHER_CCMP;
             break;
         default:
             hddLog(LOG1, "%s called with unknown auth type %d ",
                         __func__, pRoamProfile->negotiatedMCEncryptionType);
            return -EIO;
       }
   }

    hddLog(LOG1, "%s called with auth type %d",
           __func__, pRoamProfile->AuthType.authType[0]);
    EXIT();
    return 0;
}

int iw_get_auth(struct net_device *dev,struct iw_request_info *info,
                union iwreq_data *wrqu,char *extra)
{
    int ret;

    vos_ssr_protect(__func__);
    ret = __iw_get_auth(dev, info, wrqu, extra);
    vos_ssr_unprotect(__func__);

    return ret;
}

/**---------------------------------------------------------------------------

  \brief __iw_set_ap_address() -
   This function calls the sme_RoamConnect function to associate
   to the AP with the specified BSSID received from the wpa_supplicant.

  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
int __iw_set_ap_address(struct net_device *dev,
                        struct iw_request_info *info,
                        union iwreq_data *wrqu, char *extra)
{
    hdd_station_ctx_t *pHddStaCtx;
    hdd_adapter_t *pAdapter;
    hdd_context_t *pHddCtx;
    v_U8_t  *pMacAddress = NULL;
    int ret = 0;

    ENTER();

    pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
    if (NULL == pAdapter)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: Adapter is NULL", __func__);
        return -EINVAL;
    }
    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
    {
        return ret;
    }
    pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    if (NULL == pHddStaCtx)
    {
        VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                  "%s: pHddStaCtx is NULL", __func__);
        return -EINVAL;
    }

    pMacAddress = (v_U8_t*) wrqu->ap_addr.sa_data;
    VOS_TRACE(VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO, "%s "MAC_ADDRESS_STR,
              __func__, MAC_ADDR_ARRAY(pMacAddress));
    vos_mem_copy( pHddStaCtx->conn_info.bssId, pMacAddress, sizeof( tCsrBssid ));

    EXIT();
    return 0;
}

int iw_set_ap_address(struct net_device *dev,
                        struct iw_request_info *info,
                        union iwreq_data *wrqu, char *extra)
{
    int ret;

    vos_ssr_protect(__func__);
    ret = __iw_set_ap_address(dev, info, wrqu, extra);
    vos_ssr_unprotect(__func__);

    return ret;
}

/**---------------------------------------------------------------------------

  \brief __iw_get_ap_address() -
   This function returns the BSSID to the wpa_supplicant
  \param  - dev - Pointer to the net device.
              - info - Pointer to the iw_request_info.
              - wrqu - Pointer to the iwreq_data.
              - extra - Pointer to the data.
  \return - 0 for success, non zero for failure

  --------------------------------------------------------------------------*/
int __iw_get_ap_address(struct net_device *dev,
                        struct iw_request_info *info,
                        union iwreq_data *wrqu, char *extra)
{
    hdd_station_ctx_t *pHddStaCtx;
    hdd_adapter_t *pAdapter;
    hdd_context_t *pHddCtx;
    int ret = 0;

    ENTER();

    pAdapter = WLAN_HDD_GET_PRIV_PTR(dev);
    if (NULL == pAdapter)
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: Adapter is NULL", __func__);
        return -EINVAL;
    }
    pHddCtx = WLAN_HDD_GET_CTX(pAdapter);
    ret = wlan_hdd_validate_context(pHddCtx);
    if (0 != ret)
    {
        return ret;
    }
    pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
    if (NULL == pHddStaCtx)
    {
        hddLog(VOS_TRACE_LEVEL_ERROR,
               "%s: pHddStaCtx is NULL", __func__);
        return -EINVAL;
    }

    if ((pHddStaCtx->conn_info.connState == eConnectionState_Associated) ||
        (eConnectionState_IbssConnected == pHddStaCtx->conn_info.connState))
    {
        memcpy(wrqu->ap_addr.sa_data,pHddStaCtx->conn_info.bssId,ETH_ALEN);
    }
    else
    {
        memset(wrqu->ap_addr.sa_data,0,sizeof(wrqu->ap_addr.sa_data));
    }
    EXIT();
    return 0;
}

int iw_get_ap_address(struct net_device *dev,
                        struct iw_request_info *info,
                        union iwreq_data *wrqu, char *extra)
{
   int ret;

   vos_ssr_protect(__func__);
   ret = __iw_get_ap_address(dev, info, wrqu, extra);
   vos_ssr_unprotect(__func__);

   return ret;
}

#ifdef WLAN_FEATURE_11W
/**---------------------------------------------------------------------------

  \brief hdd_indicateUnprotMgmtFrame -
  This function forwards the unprotected management frame to the supplicant
  \param  - pAdapter - Pointer to HDD adapter
          - nFrameLength - Length of the unprotected frame being passed
          - pbFrames - Pointer to the frame buffer
          - frameType - 802.11 frame type
  \return - nothing

  --------------------------------------------------------------------------*/
void hdd_indicateUnprotMgmtFrame( hdd_adapter_t *pAdapter,
                            tANI_U32 nFrameLength,
                            tANI_U8* pbFrames,
                            tANI_U8 frameType )
{
    tANI_U8 type = 0;
    tANI_U8 subType = 0;

    hddLog(VOS_TRACE_LEVEL_INFO, "%s: Frame Type = %d Frame Length = %d",
            __func__, frameType, nFrameLength);

    /* Sanity Checks */
    if (NULL == pAdapter)
    {
        hddLog( LOGE, FL("pAdapter is NULL"));
        return;
    }

    if (NULL == pAdapter->dev)
    {
        hddLog( LOGE, FL("pAdapter->dev is NULL"));
        return;
    }

    if (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic)
    {
        hddLog( LOGE, FL("pAdapter has invalid magic"));
        return;
    }

    if( !nFrameLength )
    {
        hddLog( LOGE, FL("Frame Length is Invalid ZERO"));
        return;
    }

    if (NULL == pbFrames) {
        hddLog( LOGE, FL("pbFrames is NULL"));
        return;
    }

    type = WLAN_HDD_GET_TYPE_FRM_FC(pbFrames[0]);
    subType = WLAN_HDD_GET_SUBTYPE_FRM_FC(pbFrames[0]);

    /* Get pAdapter from Destination mac address of the frame */
    if (type == SIR_MAC_MGMT_FRAME && subType == SIR_MAC_MGMT_DISASSOC)
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
        cfg80211_rx_unprot_mlme_mgmt(pAdapter->dev, pbFrames, nFrameLength);
#else
        cfg80211_send_unprot_disassoc(pAdapter->dev, pbFrames, nFrameLength);
#endif
        pAdapter->hdd_stats.hddPmfStats.numUnprotDisassocRx++;
    }
    else if (type == SIR_MAC_MGMT_FRAME && subType == SIR_MAC_MGMT_DEAUTH)
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 11, 0))
        cfg80211_rx_unprot_mlme_mgmt(pAdapter->dev, pbFrames, nFrameLength);
#else
        cfg80211_send_unprot_deauth(pAdapter->dev, pbFrames, nFrameLength);
#endif
        pAdapter->hdd_stats.hddPmfStats.numUnprotDeauthRx++;
    }
    else
    {
        hddLog( LOGE, FL("Frame type %d and subtype %d are not valid"), type, subType);
        return;
    }
}
#endif

#if defined (FEATURE_WLAN_ESE) && defined(FEATURE_WLAN_ESE_UPLOAD)
void hdd_indicateTsmIe(hdd_adapter_t *pAdapter, tANI_U8 tid,
                                  tANI_U8 state,
                                  tANI_U16 measInterval )
{
    union iwreq_data wrqu;
    char buf[IW_CUSTOM_MAX + 1];
    int nBytes = 0;

    if (NULL == pAdapter)
        return;

    // create the event
    memset(&wrqu, '\0', sizeof(wrqu));
    memset(buf, '\0', sizeof(buf));

    hddLog(VOS_TRACE_LEVEL_INFO, "TSM Ind tid(%d) state(%d) MeasInt(%d)",
                        tid, state, measInterval);

    nBytes = snprintf(buf, IW_CUSTOM_MAX, "TSMIE=%d:%d:%d",tid,state,measInterval);

    wrqu.data.pointer = buf;
    wrqu.data.length = nBytes;
    // send the event
    wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buf);
}

void hdd_indicateCckmPreAuth(hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo)
{
    union iwreq_data wrqu;
    char buf[IW_CUSTOM_MAX + 1];
    char *pos = buf;
    int nBytes = 0, freeBytes = IW_CUSTOM_MAX;

    if ((NULL == pAdapter) || (NULL == pRoamInfo))
        return;

    // create the event
    memset(&wrqu, '\0', sizeof(wrqu));
    memset(buf, '\0', sizeof(buf));

    /* Timestamp0 is lower 32 bits and Timestamp1 is upper 32 bits */
    hddLog(VOS_TRACE_LEVEL_INFO, "CCXPREAUTHNOTIFY=%02x:%02x:%02x:%02x:%02x:%02x %u:%u",
        pRoamInfo->bssid[0], pRoamInfo->bssid[1], pRoamInfo->bssid[2],
        pRoamInfo->bssid[3], pRoamInfo->bssid[4], pRoamInfo->bssid[5],
        pRoamInfo->timestamp[0], pRoamInfo->timestamp[1]);

    nBytes = snprintf(pos, freeBytes, "CCXPREAUTHNOTIFY=");
    pos += nBytes;
    freeBytes -= nBytes;

    vos_mem_copy(pos, pRoamInfo->bssid, WNI_CFG_BSSID_LEN);
    pos += WNI_CFG_BSSID_LEN;
    freeBytes -= WNI_CFG_BSSID_LEN;

    nBytes = snprintf(pos, freeBytes, " %u:%u", pRoamInfo->timestamp[0], pRoamInfo->timestamp[1]);
    freeBytes -= nBytes;

    wrqu.data.pointer = buf;
    wrqu.data.length = (IW_CUSTOM_MAX - freeBytes);

    // send the event
    wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buf);
}

void hdd_indicateEseAdjApRepInd(hdd_adapter_t *pAdapter, tCsrRoamInfo *pRoamInfo)
{
    union iwreq_data wrqu;
    char buf[IW_CUSTOM_MAX + 1];
    int nBytes = 0;

    if ((NULL == pAdapter) || (NULL == pRoamInfo))
        return;

    // create the event
    memset(&wrqu, '\0', sizeof(wrqu));
    memset(buf, '\0', sizeof(buf));

    hddLog(VOS_TRACE_LEVEL_INFO, "CCXADJAPREP=%u", pRoamInfo->tsmRoamDelay);

    nBytes = snprintf(buf, IW_CUSTOM_MAX, "CCXADJAPREP=%u", pRoamInfo->tsmRoamDelay);

    wrqu.data.pointer = buf;
    wrqu.data.length = nBytes;

    // send the event
    wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buf);
}

void hdd_indicateEseBcnReportNoResults(const hdd_adapter_t *pAdapter,
                                       const tANI_U16 measurementToken,
                                       const tANI_BOOLEAN flag,
                                       const tANI_U8 numBss)
{
    union iwreq_data wrqu;
    char buf[IW_CUSTOM_MAX];
    char *pos = buf;
    int nBytes = 0, freeBytes = IW_CUSTOM_MAX;

    memset(&wrqu, '\0', sizeof(wrqu));
    memset(buf, '\0', sizeof(buf));

    hddLog(VOS_TRACE_LEVEL_INFO, FL("CCXBCNREP=%d %d %d"), measurementToken, flag,
           numBss);

    nBytes = snprintf(pos, freeBytes, "CCXBCNREP=%d %d %d", measurementToken,
                      flag, numBss);

    wrqu.data.pointer = buf;
    wrqu.data.length = nBytes;
    // send the event
    wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buf);
}


static void hdd_indicateEseBcnReportInd(const hdd_adapter_t *pAdapter,
                                 const tCsrRoamInfo *pRoamInfo)
{
    union iwreq_data wrqu;
    char buf[IW_CUSTOM_MAX + 1];
    char *pos = buf;
    int nBytes = 0, freeBytes = IW_CUSTOM_MAX;
    tANI_U8 i = 0, len = 0;
    tANI_U8 tot_bcn_ieLen = 0;  /* total size of the beacon report data */
    tANI_U8 lastSent = 0, sendBss = 0;
    int bcnRepFieldSize = sizeof(pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[0].bcnReportFields);
    tANI_U8 ieLenByte = 1;
    /* CCXBCNREP=meas_tok<sp>flag<sp>no_of_bss<sp>tot_bcn_ie_len = 18 bytes */
#define ESEBCNREPHEADER_LEN  (18)

    if ((NULL == pAdapter) || (NULL == pRoamInfo))
        return;

    /* Custom event can pass maximum of 256 bytes of data,
       based on the IE len we need to identify how many BSS info can
       be filled in to custom event data */
    /*
       meas_tok<sp>flag<sp>no_of_bss<sp>tot_bcn_ie_len bcn_rep_data
         bcn_rep_data will have bcn_rep_fields,ie_len,ie without any spaces
         CCXBCNREP=meas_tok<sp>flag<sp>no_of_bss<sp>tot_bcn_ie_len = 18 bytes
    */

    if ((pRoamInfo->pEseBcnReportRsp->flag >> 1) && (!pRoamInfo->pEseBcnReportRsp->numBss))
    {
        hddLog(VOS_TRACE_LEVEL_INFO, "Measurement Done but no scan results");
        /* If the measurement is none and no scan results found,
            indicate the supplicant about measurement done */
        hdd_indicateEseBcnReportNoResults(pAdapter,
                                 pRoamInfo->pEseBcnReportRsp->measurementToken,
                                 pRoamInfo->pEseBcnReportRsp->flag,
            pRoamInfo->pEseBcnReportRsp->numBss);
    }
    else
    {
        while (lastSent < pRoamInfo->pEseBcnReportRsp->numBss)
        {
            memset(&wrqu, '\0', sizeof(wrqu));
            memset(buf, '\0', sizeof(buf));
            tot_bcn_ieLen = 0;
            sendBss = 0;
            pos = buf;
            freeBytes = IW_CUSTOM_MAX;

            for (i = lastSent; i < pRoamInfo->pEseBcnReportRsp->numBss; i++)
            {
                len = bcnRepFieldSize + ieLenByte + pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i].ieLen;
                if ((len + tot_bcn_ieLen) > (IW_CUSTOM_MAX - ESEBCNREPHEADER_LEN))
                {
                    break;
                }
                tot_bcn_ieLen += len;
                sendBss++;
                hddLog(VOS_TRACE_LEVEL_INFO, "i(%d) sizeof bcnReportFields(%d)"
                             "IeLength(%d) Length of Ie(%d) totLen(%d)",
                              i, bcnRepFieldSize, 1,
                              pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i].ieLen,
                              tot_bcn_ieLen);
            }

            hddLog(VOS_TRACE_LEVEL_INFO, "Sending %d BSS Info", sendBss);
            hddLog(VOS_TRACE_LEVEL_INFO, "CCXBCNREP=%d %d %d %d",
                pRoamInfo->pEseBcnReportRsp->measurementToken, pRoamInfo->pEseBcnReportRsp->flag,
                sendBss, tot_bcn_ieLen);

            nBytes = snprintf(pos, freeBytes, "CCXBCNREP=%d %d %d ",
                pRoamInfo->pEseBcnReportRsp->measurementToken, pRoamInfo->pEseBcnReportRsp->flag,
                sendBss);
            pos += nBytes;
            freeBytes -= nBytes;

            /* Copy total Beacon report data length */
            vos_mem_copy(pos, (char*)&tot_bcn_ieLen, sizeof(tot_bcn_ieLen));
            pos += sizeof(tot_bcn_ieLen);
            freeBytes -= sizeof(tot_bcn_ieLen);

            for (i = 0; i < sendBss; i++)
            {
                hddLog(VOS_TRACE_LEVEL_INFO, "ChanNum(%d) Spare(%d) MeasDuration(%d)"
                       " PhyType(%d) RecvSigPower(%d) ParentTSF(%u)"
                       " TargetTSF[0](%u) TargetTSF[1](%u) BeaconInterval(%u)"
                       " CapabilityInfo(%d) BSSID(%02X:%02X:%02X:%02X:%02X:%02X)",
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.ChanNum,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Spare,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.MeasDuration,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.PhyType,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.RecvSigPower,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.ParentTsf,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.TargetTsf[0],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.TargetTsf[1],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.BcnInterval,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.CapabilityInfo,
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[0],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[1],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[2],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[3],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[4],
                        pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields.Bssid[5]);

                /* bcn report fields are copied */
                len = sizeof(pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields);
                vos_mem_copy(pos, (char*)&pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].bcnReportFields, len);
                pos += len;
                freeBytes -= len;

                /* Add 1 byte of ie len */
                len = pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].ieLen;
                vos_mem_copy(pos, (char*)&len, sizeof(len));
                pos += sizeof(len);
                freeBytes -= sizeof(len);

                /* copy IE from scan results */
                vos_mem_copy(pos, (char*)pRoamInfo->pEseBcnReportRsp->bcnRepBssInfo[i+lastSent].pBuf, len);
                pos += len;
                freeBytes -= len;
            }

            wrqu.data.pointer = buf;
            wrqu.data.length = strlen(buf);

            // send the event
            wireless_send_event(pAdapter->dev, IWEVCUSTOM, &wrqu, buf);
            lastSent += sendBss;
        }
    }
}

#endif /* FEATURE_WLAN_ESE && FEATURE_WLAN_ESE_UPLOAD */

hdd_adapter_t *hdd_get_sta_connection_in_progress(hdd_context_t *hdd_ctx)
{
    hdd_adapter_list_node_t *adapter_node = NULL, *next = NULL;
    hdd_adapter_t *adapter = NULL;
    VOS_STATUS status;
    hdd_station_ctx_t *hdd_sta_ctx;

    if (!hdd_ctx) {
        hddLog(LOGE, FL("HDD context is NULL"));
        return NULL;
    }

    status = hdd_get_front_adapter(hdd_ctx, &adapter_node);
    while (NULL != adapter_node && VOS_STATUS_SUCCESS == status) {
        adapter = adapter_node->pAdapter;
        if (!adapter)
            goto end;

        if ((WLAN_HDD_INFRA_STATION == adapter->device_mode) ||
            (WLAN_HDD_P2P_CLIENT == adapter->device_mode) ||
            (WLAN_HDD_P2P_DEVICE == adapter->device_mode)) {
            hdd_sta_ctx = WLAN_HDD_GET_STATION_CTX_PTR(adapter);
            if (eConnectionState_Connecting ==
                hdd_sta_ctx->conn_info.connState) {
                    hddLog(LOG1, FL("session_id %d: Connection is in progress"),
                           adapter->sessionId);
                    return adapter;
            } else if ((eConnectionState_Associated ==
                hdd_sta_ctx->conn_info.connState) &&
                sme_is_sta_key_exchange_in_progress(
                hdd_ctx->hHal, adapter->sessionId)) {
                    hddLog(LOG1, FL("session_id %d: Key exchange is in progress"),
                           adapter->sessionId);
                    return adapter;
            }
        }
end:
        status = hdd_get_next_adapter(hdd_ctx, adapter_node, &next);
        adapter_node = next;
    }
    return NULL;
}

