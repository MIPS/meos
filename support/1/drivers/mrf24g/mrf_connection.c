/***(C)2017***************************************************************
*
* Copyright (C) 2017 MIPS Tech, LLC
* 
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
* this list of conditions and the following disclaimer in the documentation
* and/or other materials provided with the distribution.
* 3. Neither the name of the copyright holder nor the names of its
* contributors may be used to endorse or promote products derived from this
* software without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
****(C)2017**************************************************************/

/*************************************************************************
*
*   Description:	MRF24G driver
*
*************************************************************************/

#include "meos/mrf24g/mrf_connection.h"

#include "mrf_raw.h"
#include "mrf_mgmt.h"
#include "mrf_const.h"

typedef enum {
	MRF_CP_ELEMENT_SSID = 1,
	MRF_CP_ELEMENT_BSSID = 2,
	MRF_CP_ELEMENT_SECURITY = 3,
	MRF_CP_ELEMENT_NETWORK_TYPE = 4,
	MRF_CP_ELEMENT_ADHOC_BEHAVIOR = 5,
	MRF_CP_ELEMENT_WEP_KEY_INDEX = 6,
	MRF_CP_ELEMENT_SSID_TYPE = 7,
	MRF_CP_ELEMENT_WEPKEY_TYPE = 8,
	MRF_CP_ELEMENT_UPDATE_PMK = 9,
	MRF_CP_ELEMENT_READ_WPS_CRED = 10
} CP_element_id_t;

typedef enum {
	MRF_CA_ELEMENT_SCANTYPE = 1,
	MRF_CA_ELEMENT_RSSI = 2,
	MRF_CA_ELEMENT_CP_LIST = 3,
	MRF_CA_ELEMENT_LIST_RETRY_COUNT = 4,
	MRF_CA_ELEMENT_EVENT_NOTIFICATION_ACTION = 5,
	MRF_CA_ELEMENT_BEACON_TIMEOUT_ACTION = 6,
	MRF_CA_ELEMENT_DEAUTH_ACTION = 7,
	MRF_CA_ELEMENT_CHANNEL_LIST = 8,
	MRF_CA_ELEMENT_LISTEN_INTERVAL = 9,
	MRF_CA_ELEMENT_BEACON_TIMEOUT = 10,
	MRF_CA_ELEMENT_SCAN_COUNT = 11,
	MRF_CA_ELEMENT_MIN_CHANNEL_TIME = 12,
	MRF_CA_ELEMENT_MAX_CHANNEL_TIME = 13,
	MRF_CA_ELEMENT_PROBE_DELAY = 14,
	MRF_CA_ELEMENT_DTIM_INTERVAL = 15,
	MRF_CA_ELEMENT_BEACON_PERIOD = 16
} CA_element_id_t;

typedef enum {
	/* Misc subtypes */
	MRF_SCAN_SUBTYPE = 1,
	MRF_JOIN_SUBTYPE = 2,
	MRF_AUTH_SUBTYPE = 3,
	MRF_ASSOC_SUBTYPE = 4,
	MRF_DISCONNECT_SUBTYPE = 5,
	MRF_DISASOCC_SUBTYPE = 6,
	MRF_SET_POWER_MODE_SUBTYPE = 7,
	MRF_SET_PM_KEY_SUBTYPE = 8,
	MRF_SET_WEP_M_SUBTYPE = 9,
	MRF_SET_WEP_KEY_SUBTYPE = 10,
	MRF_SET_TEMP_KEY_SUBTYPE = 11,
	MRF_CALC_PSK_KEY_SUBTYPE = 12,
	MRF_SET_WEP_KEY_ID_SUBTYPE = 13,
	MRF_CONFIG_KEY_SPACE_SUBTYPE = 14,
	MRF_SET_PARAM_SUBTYPE = 15,
	MRF_GET_PARAM_SUBTYPE = 16,
	MRF_ADHOC_CONNECT_SUBTYPE = 17,
	MRF_ADHOC_START_SUBTYPE = 18,

	/* Connection Profile Message Subtypes */
	MRF_CP_CREATE_PROFILE_SUBTYPE = 21,
	MRF_CP_DELETE_PROFILE_SUBTYPE = 22,
	MRF_CP_GET_ID_LIST_SUBTYPE = 23,
	MRF_CP_SET_ELEMENT_SUBTYPE = 24,
	MRF_CP_GET_ELEMENT_SUBTYPE = 25,

	/* Connection Algorithm Message Subtypes */
	MRF_CA_SET_ELEMENT_SUBTYPE = 26,
	MRF_CA_GET_ELEMENT_SUBTYPE = 27,

	/* Connnection Manager Message Subtypes */
	MRF_CM_CONNECT_SUBYTPE = 28,
	MRF_CM_DISCONNECT_SUBYTPE = 29,
	MRF_CM_GET_CONNECTION_STATUS_SUBYTPE = 30,

	MRF_SCAN_START_SUBTYPE = 31,
	MRF_SCAN_GET_RESULTS_SUBTYPE = 32,

	MRF_CM_INFO_SUBTYPE = 33,

	MRF_SCAN_FOR_IE_SUBTYPE = 34,	/* not yet supported */
	MRF_SCAN_IE_GET_RESULTS_SUBTYPE = 35,	/* not yet supported */

	MRF_CM_GET_CONNECTION_STATISTICS_SUBYTPE = 36,	/* not yet supported so moved here for now */
	MRF_NUM_REQUEST_SUBTYPES
} mgmt_msg_subtypes_t;

typedef enum {
	PARAM_MAC_ADDRESS = 1,	/* the device MAC address (6 bytes)                            */
	PARAM_REGIONAL_DOMAIN = 2,	/* the device Regional Domain (1 byte)                         */
	PARAM_RTS_THRESHOLD = 3,	/* the RTS byte threshold 256 - 2347 (2 bytes)                 */
	PARAM_LONG_FRAME_RETRY_LIMIT = 4,	/* the long Frame Retry limit  (1 byte)                        */
	PARAM_SHORT_FRAME_RETRY_LIMIT = 5,	/* the short Frame Retry limit (1 byte)                        */
	PARAM_TX_LIFETIME_TU = 6,	/* the Tx Request lifetime in TU's 0 - 4194303 (4 bytes)       */
	PARAM_RX_LIFETIME_TU = 7,	/* the Rx Frame lifetime in TU's 0 - 4194303 (4 bytes)         */
	PARAM_SUPPLICANT_ON_OFF = 8,	/* boolean 1 = on 0 = off (1 byte)                             */
	PARAM_CONFIRM_DATA_TX_REQ = 9,	/* boolean 1 = on 0 = off (1 byte)                             */
	PARAM_MASTER_STATE = 10,	/* master state of the MAC using enumerated values (1 byte)    */
	PARAM_HOST_ALERT_BITS = 11,	/* a bit field which enables/disables various asynchronous     */
	/*   indications from the MAC to the host (2 bytes)            */
	PARAM_NUM_MISSED_BEACONS = 12,	/* number of consecutive beacons MAC can miss before it        */
	/*   considers the network lost (1 byte)                       */
	PARAM_DIFS_AND_EIFS = 13,	/* delay intervals in usec DIFS and EIFS ( 2 * 2 bytes)        */
	PARAM_TX_POWER = 14,	/* max and min boundaries for Tx power (2 * 2 bytes)           */
	PARAM_DEFAULT_DEST_MAC_ADDR = 15,	/* stores a persistant destination MAC address for small       */
	/*   Tx Requests (6 bytes)                                     */
	PARAM_WPA_INFO_ELEMENT = 16,	/* stores a WPA info element (IE) in 802.11 IE format.  Used   */
	/*   in Assoc Request and Supplicant exchange (3 - 258 bytes)  */
	PARAM_RSN_INFO_ELEMENT = 17,	/* stores a RSN info element (IE) in 802.11 IE format.  Used   */
	/*   in Assoc Request and Supplicant exchange (3 - 258 bytes)  */
	PARAM_ON_OFF_RADIO = 18,	/* bool to force a radio state change 1 = on 0 = off (1 byte)  */
	PARAM_COMPARE_ADDRESS = 19,	/* a MAC address used to filter received frames                */
	/*   (sizeof(tAddressFilterInput) = 8 bytes)                   */
	PARAM_SUBTYPE_FILTER = 20,	/* bitfield used to filter received frames based on type and   */
	/* sub-type (sizeof(tAddressFilterInput) = 4 bytes)            */
	PARAM_ACK_CONTROL = 21,	/* bitfield used to control the type of frames that cause ACK  */
	/*   responses (sizeof(tAckControlInput) = 4 bytes)            */
	PARAM_STAT_COUNTERS = 22,	/* complete set of statistics counters that are maintained by  */
	/*   the MAC                                                   */
	PARAM_TX_THROTTLE_TABLE = 23,	/* custom Tx Rate throttle table to be used to control tx Rate */
	PARAM_TX_THROTTLE_TABLE_ON_OFF = 24,	/* a boolean to enable/disable use of the throttle Table and a */
	/*   tx rate to use if the throttle table is disabled          */
	PARAM_TX_CONTENTION_ARRAY = 25,	/* custom Retry contention ladder used for backoff calculation */
	/*   prior to a Tx attempt                                     */
	PARAM_SYSTEM_VERSION = 26,	/* 2 bytes representation of a version number for the ROM and  */
	/*  Patch                                                      */
	PARAM_STATUE_INFO = 27,	/* MAC State information                                       */
	PARAM_SECURITY_CONTROL = 28,	/* 2 bytes data structure to enable/disable encryption         */
	PARAM_FACTORY_SET_TX_MAX_POWER = 29,	/* gets the factory-set tx max power level                     */
	PARAM_CONNECT_CONTEXT = 31,	/* gets current connection status                              */
	PARAM_TX_MODE = 34,	/* choose tx mode                                              */
	PARAM_LINK_DOWN_THRESHOLD = 37,	/* sets link down threshold                                    */
	PARAM_SET_PSK = 39,	/* set psk                                                     */
	PARAM_SET_HOST_DERIVE_KEY = 40,	/* has host derive key from passphrase                         */
	PARAM_SET_MULTICAST_FILTER = 41	/* set multicast filter                                        */
} mrf_param_t;

int MRF_pel_set(const uint8_t id, const uint8_t * const data,
		const uint8_t length)
{
	uint8_t hdr[] = { MRF_MGMT_REQUEST_TYPE, MRF_CP_SET_ELEMENT_SUBTYPE, 1,	// cpid
		id, length
	};
	return MRF_mgmt_send(hdr, sizeof(hdr), data, length);
}

int MRF_pel_get_read(const uint8_t id, const uint8_t index,
		     uint8_t * const data, const uint8_t length)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_CP_SET_ELEMENT_SUBTYPE, 1, id };
	return MRF_mgmt_send_read(hdr, sizeof(hdr), 0, 0, index, data, length);
}

int MRF_pel_get(const uint8_t id)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_CP_SET_ELEMENT_SUBTYPE, 1, id };
	return MRF_mgmt_send_nofree(hdr, sizeof(hdr), 0, 0);
}

int MRF_pctxCreate(void)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_CP_CREATE_PROFILE_SUBTYPE };
	return MRF_mgmt_send(hdr, sizeof(hdr), 0, 0);
}

int MRF_ssidSet(const uint8_t * const ssid, const uint8_t length)
{
	return MRF_pel_set(MRF_CP_ELEMENT_SSID, ssid, length);
}

int MRF_ssidGet(uint8_t * const ssid)
{
	int ret = MRF_pel_get(MRF_CP_ELEMENT_SSID);
	if (ret != 0) {
		return ret;
	}
	uint8_t hdr[7];
	ret =
	    MRF_raw_read(MRF_RAW_SCRATCH_ID, MRF_MGMT_RX_BASE, hdr,
			 sizeof(hdr));
	if (ret != 0) {
		return ret;
	}
	int length = hdr[6];
	MRF_raw_rel_read(MRF_RAW_SCRATCH_ID, ssid, length);
	return length;
}

int MRF_bssidSet(const uint8_t * const bssid)
{
	return MRF_pel_set(MRF_CP_ELEMENT_BSSID, bssid, 6);
}

int MRF_networkTypeSet(const uint8_t network_type)
{
	return MRF_pel_set(MRF_CP_ELEMENT_NETWORK_TYPE, &network_type, 1);
}

int MRF_securityOpenSet(void)
{
	uint8_t hdr[] = { MRF_MGMT_REQUEST_TYPE, MRF_CP_SET_ELEMENT_SUBTYPE, 1,
		MRF_CP_ELEMENT_SECURITY
	};
	uint8_t data[] = { 0, 0 };
	return MRF_mgmt_send(hdr, sizeof(hdr), data, sizeof(data));
}

int MRF_ael_set(const uint8_t id, const uint8_t * const data,
		const uint8_t length)
{
	uint8_t hdr[] = { MRF_MGMT_REQUEST_TYPE, MRF_CA_SET_ELEMENT_SUBTYPE, id,
		length
	};
	return MRF_mgmt_send(hdr, sizeof(hdr), data, length);
}

int MRF_ael_get_read(const uint8_t id, const uint8_t index,
		     uint8_t * const data, const uint8_t length)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_CA_GET_ELEMENT_SUBTYPE, id, 0 };
	return MRF_mgmt_send_read(hdr, sizeof(hdr), 0, 0, index, data, length);
}

int MRF_ael_get(const uint8_t id)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_CA_GET_ELEMENT_SUBTYPE, id, 0 };
	return MRF_mgmt_send_nofree(hdr, sizeof(hdr), 0, 0);
}

int MRF_scanTypeSet(const uint8_t type)
{
	return MRF_ael_set(MRF_CA_ELEMENT_SCANTYPE, &type, sizeof(type));
}

int MRF_scanCountSet(const uint8_t count)
{
	return MRF_ael_set(MRF_CA_ELEMENT_SCAN_COUNT, &count, sizeof(count));
}

int MRF_scanMinChannelTimeSet(const uint16_t tim)
{
	uint8_t tnbo[] = { tim >> 8, tim & 0xff };	// big end in
	return MRF_ael_set(MRF_CA_ELEMENT_MIN_CHANNEL_TIME, tnbo, sizeof(tnbo));
}

int MRF_scanMaxChannelTimeSet(const uint16_t tim)
{
	uint8_t tnbo[] = { tim >> 8, tim & 0xff };	// big end in
	return MRF_ael_set(MRF_CA_ELEMENT_PROBE_DELAY, tnbo, sizeof(tnbo));
}

int MRF_scanProbeDelaySet(const uint16_t tim)
{
	uint8_t tnbo[] = { tim >> 8, tim & 0xff };	// big end in
	return MRF_ael_set(MRF_CA_ELEMENT_MIN_CHANNEL_TIME, tnbo, sizeof(tnbo));
}

int MRF_scan(void)
{
	mrf_scan_results_count = 0;
	uint8_t hdr[] = { MRF_MGMT_REQUEST_TYPE, MRF_SCAN_START_SUBTYPE, 0xff,
		0
	};
	/* is data and length 0 here? */
	return MRF_mgmt_send(hdr, sizeof(hdr), 0, 0);
}

uint16_t MRF_scanGetResultsCount(void)
{
	return mrf_scan_results_count;
}

int MRF_scanGetResult(MRF_SCAN_RESULT_T * const result, const uint8_t index)
{
	uint8_t hdr[] = { MRF_MGMT_REQUEST_TYPE, MRF_SCAN_GET_RESULTS_SUBTYPE,
		index, 1
	};

	return MRF_mgmt_send_read(hdr, sizeof(hdr), 0, 0, 5,
				  (uint8_t *) result, sizeof(*result));
}

int MRF_rssiSet(const uint8_t rssi)
{
	return MRF_ael_set(MRF_CA_ELEMENT_RSSI, &rssi, sizeof(rssi));
}

int MRF_retryCountSet(const uint8_t count)
{
	return MRF_ael_set(MRF_CA_ELEMENT_LIST_RETRY_COUNT, &count,
			   sizeof(count));
}

int MRF_deauthActionSet(const uint8_t action)
{
	return MRF_ael_set(MRF_CA_ELEMENT_DEAUTH_ACTION, &action,
			   sizeof(action));
}

int MRF_beaconTimeoutSet(const uint8_t timeout)
{
	return MRF_ael_set(MRF_CA_ELEMENT_BEACON_TIMEOUT, &timeout,
			   sizeof(timeout));
}

int MRF_beaconTimeoutActionSet(const uint8_t action)
{
	return MRF_ael_set(MRF_CA_ELEMENT_BEACON_TIMEOUT_ACTION, &action,
			   sizeof(action));
}

int MRF_channelListSet(const uint8_t * const channels, const uint8_t length)
{
	return MRF_ael_set(MRF_CA_ELEMENT_CHANNEL_LIST, channels, length);
}

int MRF_listenIntervalSet(const uint16_t interval)
{
	uint8_t inbo[] = { interval >> 8, interval & 0xff };	// big end in
	return MRF_ael_set(MRF_CA_ELEMENT_LISTEN_INTERVAL, inbo, sizeof(inbo));
}

int MRF_dtimIntervalSet(const uint16_t interval)
{
	uint8_t inbo[] = {
		interval >> 8, interval & 0xff
	};			// big end in
	return MRF_ael_set(MRF_CA_ELEMENT_DTIM_INTERVAL, inbo, sizeof(inbo));
}

int MRF_param_set(uint8_t type, uint8_t * const data, uint8_t length)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_SET_PARAM_SUBTYPE, 0, type };
	return MRF_mgmt_send(hdr, sizeof(hdr), data, length);
}

int MRF_param_get(uint8_t type, uint8_t * const data, uint8_t length)
{
	uint8_t hdr[] =
	    { MRF_MGMT_REQUEST_TYPE, MRF_GET_PARAM_SUBTYPE, 0, type };
	return MRF_mgmt_send_read(hdr, sizeof(hdr), 0, 0,
				  MSG_PARAM_START_DATA_INDEX, data, length);
}

int MRF_versionGet(MRF_VINFO_T * const info)
{
	uint8_t buf[2];
	int ret = MRF_param_get(PARAM_SYSTEM_VERSION, buf, sizeof(buf));
	info->version = buf[0];
	info->patch = buf[1];
	return ret;
}

int MRF_domainGet(uint8_t * const domain)
{
	return MRF_param_get(PARAM_REGIONAL_DOMAIN, domain, 1);
}

int MRF_macGet(uint8_t * const mac)
{
	return MRF_param_get(PARAM_MAC_ADDRESS, mac, 6);
}

int MRF_connect(void)
{
	uint8_t hdr[] = {
		MRF_MGMT_REQUEST_TYPE, MRF_CM_CONNECT_SUBYTPE, 1,
		0
	};
	return MRF_mgmt_send(hdr, sizeof(hdr), 0, 0);
}
