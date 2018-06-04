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

#ifndef MRF_CONST_H
#define MRF_CONST_H

/* API */
static const uint8_t MRF_NETWORK_TYPE_INFRASTRUCTURE = 1;
static const uint8_t MRF_NETWORK_TYPE_ADHOC = 2;
static const uint8_t MRF_NETWORK_TYPE_SOFT_AP = 4;

static const int MRF_ACTIVE_SCAN = 1;
static const int MRF_PASSIVE_SCAN = 2;

static const int MRF_DO_NOT_ATTEMPT_TO_RECONNECT = 0;
static const int MRF_ATTEMPT_TO_RECONNECT = 1;

static const int MRF_DISABLED = 0;
static const int MRF_ENABLED = 1;

static const uint8_t MRF_HOST_INT_MASK_INT2 = 0x01;
static const uint8_t MRF_HOST_INT_MASK_RAW_0_INT_0 = 0x02; /* Data Rx */
static const uint8_t MRF_HOST_INT_MASK_RAW_1_INT_0 = 0x04; /* Data Tx */
static const uint8_t MRF_HOST_INT_MASK_FIFO_0_THRESHOLD = 0x40;
static const uint8_t MRF_HOST_INT_MASK_FIFO_1_THRESHOLD = 0x80;
static const uint8_t MRF_HOST_INT_MASK_ALL_INT = 0xff;

/* Bit mask for all interrupts in the level 2 16-bit interrupt register */
static const uint16_t MRF_HOST_2_INT_MASK_RAW_2_INT_0 = 0x0010; /* Mgmt Rx  */
static const uint16_t MRF_HOST_2_INT_MASK_RAW_3_INT_0 = 0x0020; /* Mgmt Tx  */
static const uint16_t MRF_HOST_2_INT_MASK_RAW_4_INT_0 = 0x0004; /* Scratch  */
static const uint16_t MRF_HOST_2_INT_MASK_RAW_5_INT_0 = 0x0008; /* Not used */
static const uint16_t MRF_HOST_2_INT_MASK_MAIL_BOX_0_WRT = 0x0001;
static const uint16_t MRF_HOST_2_INT_MASK_MAIL_BOX_1_WRT = 0x0002;
static const uint16_t MRF_HOST_2_INT_MASK_ALL_INT = 0xffff;

/* HOST_RESET_REG masks */
static const uint16_t MRF_HR_CPU_RST_N_MASK = 0x01 << 15;
static const uint16_t MRF_HR_DISABLE_DOWNLOAD_SCRAMBLER_MASK = 0x01 << 14;
static const uint16_t MRF_HR_FORCE_CPU_CLK_FREEZE_MASK = 0x01 << 13;
static const uint16_t MRF_HR_HOST_ANA_SPI_EN_MASK = 0x01 << 12;
static const uint16_t MRF_HR_HOST_ANA_SPI_DIN_MASK = 0x01 << 11;
static const uint16_t MRF_HR_HOST_ANA_SPI_DOUT_MASK = 0x01 << 10;
static const uint16_t MRF_HR_HOST_ANA_SPI_CLK_MASK = 0x01 << 9;
static const uint16_t MRF_HR_HOST_ANA_SPI_CSN_MASK = 0x07 << 6;	// 8:6
static const uint16_t MRF_HR_RESERVED_2_MASK = 0x01 << 5;
static const uint16_t MRF_HR_HOST_SPI_DISABLE_MASK = 0x01 << 4;
static const uint16_t MRF_HR_HOST_ENABLE_NEW_PROG_MASK = 0x01 << 3;
static const uint16_t MRF_HR_HOST_ENABLE_DOWNLOAD_MASK = 0x01 << 2;
static const uint16_t MRF_HR_HOST_FAST_RESET_MASK = 0x01 << 1;
static const uint16_t MRF_HR_HOST_RESET_MASK = 0x01 << 0;

/* This block of defines needed to restart PLL */
#define MRF_ANALOG_PORT_3_REG_TYPE ((uint32_t)(0x09))	// 16-bit analog register in SPI Port 3
#define MRF_ANALOG_PORT_2_REG_TYPE ((uint32_t)(0x08))	// 16-bit analog register in SPI Port 2
#define MRF_ANALOG_PORT_1_REG_TYPE ((uint32_t)(0x0a))	// 16-bit analog register in SPI Port 1
#define MRF_ANALOG_PORT_0_REG_TYPE ((uint32_t)(0x0b))	// 16-bit analog register in SPI Port 0

static const uint8_t MRF_SPI_WRITE_MASK = 0x00;	// bit 0 = 0
static const uint8_t MRF_SPI_READ_MASK = 0x01;    // bit 0 = 1
static const uint8_t MRF_SPI_AUTO_INCREMENT_ENABLED_MASK = 0x00;	// bit 1 = 0
static const uint8_t MRF_SPI_AUTO_INCREMENT_DISABLED_MASK = 0x02;	// bit 1 = 1

static const uint32_t MRF_PLL0_REG = 0 * 2;
static const uint32_t MRF_OSC0_REG = 0 * 2;
static const uint32_t MRF_BIAS_REG = 4 * 2;

static const uint16_t MRF_HW_STATUS_NOT_IN_RESET_MASK = 0x1000;

/* Mgmt indicate events */
enum MRF_mgmt_indicate_events {
    MRF_EVENT_CONNECTION_ATTEMPT_STATUS_SUBTYPE = 6,
    MRF_EVENT_CONNECTION_LOST_SUBTYPE = 7,
    MRF_EVENT_CONNECTION_REESTABLISHED_SUBTYPE = 8,
    MRF_EVENT_KEY_CALCULATION_REQUEST_SUBTYPE = 9,
    MRF_EVENT_SCAN_RESULTS_READY_SUBTYPE = 11,
    MRF_EVENT_SCAN_IE_RESULTS_READY_SUBTYPE = 12,
    MRF_EVENT_SOFT_AP_EVENT_SUBTYPE = 13,
    MRF_EVENT_DISCONNECT_DONE_SUBTYPE = 14
};


/* event values for index 2 of MRF_CONNECTION_ATTEMPT_STATUS_EVENT_SUBTYPE */
static const uint8_t CONNECTION_ATTEMPT_SUCCESSFUL = 1; /* if not 1 then failed to connect and info field is error code */
static const uint8_t CONNECTION_ATTEMPT_FAILED = 2;

/* event values for index 2 of MRF_EVENT_CONNECTION_LOST_SUBTYPE */
static const uint8_t CONNECTION_TEMPORARILY_LOST = 1;
static const uint8_t CONNECTION_PERMANENTLY_LOST = 2;
static const uint8_t CONNECTION_REESTABLISHED = 3;

static const uint8_t MRF_DATA_REQUEST_TYPE = 1;
static const uint8_t MRF_MGMT_REQUEST_TYPE = 2;

static const uint8_t MRF_DATA_TX_CONFIRM_TYPE = 1;
static const uint8_t MRF_MGMT_CONFIRM_TYPE = 2;
static const uint8_t MRF_DATA_RX_INDICATE_TYPE = 3;
static const uint8_t MRF_MGMT_INDICATE_TYPE = 4;

static const uint8_t MRF_STD_DATA_MSG_SUBTYPE = 1;
static const uint8_t MRF_NULL_DATA_MSG_SUBTYPE = 2;
/* 3 is a reserved value */
static const uint8_t MRF_UNTAMPERED_DATA_MSG_SUBTYPE = 4;

static const uint8_t MRF_TX_DATA_MSG_PREAMBLE_LENGTH = 4;

static const int MSG_PARAM_START_DATA_INDEX = 6;
static const int MULTICAST_ADDRESS = 6;
static const int ADDRESS_FILTER_DEACTIVATE = 0;

enum MRF_interrupt_types
{
    MRF_INT_NONE,
    MRF_INT_RAW0,
    MRF_INT_RAW1,
    MRF_INT_RAW2,
    MRF_INT_RAW3,
    MRF_INT_RAW4,
    MRF_INT_RAW5,
    MRF_INT_FIFO0,
    MRF_INT_FIFO1,
    MRF_INT_MAILBOX0,
    MRF_INT_MAILBOX1
};

typedef enum {
    MRF_EVENT_NONE = 0,
    MRF_EVENT_CONNECTION_SUCCESSFUL = 1,
    MRF_EVENT_CONNECTION_FAILED = 2,
    MRF_EVENT_CONNECTION_TEMPORARILY_LOST = 3,
    MRF_EVENT_CONNECTION_PERMANENTLY_LOST = 4,
    MRF_EVENT_CONNECTION_REESTABLISHED = 5,
    MRF_EVENT_FLASH_UPDATE_SUCCESSFUL = 6,
    MRF_EVENT_FLASH_UPDATE_FAILED = 7,
    MRF_EVENT_KEY_CALCULATION_REQUEST = 8,
    MRF_EVENT_INVALID_WPS_PIN = 9,
    MRF_EVENT_SCAN_RESULTS_READY = 10,
    MRF_EVENT_IE_RESULTS_READY = 11,
    MRF_EVENT_SOFT_AP = 12,
    MRF_EVENT_DISCONNECT_DONE = 13,
    MRF_EVENT_UPDATE = 14,
    MRF_EVENT_ERROR = 15
} MRF_EVENTS;

#define MRF_DISABLED     (0)
#define MRF_ENABLED      (1)

#define MRF_MAX_SSID_LENGTH                (32)
#define MRF_BSSID_LENGTH                   (6)
#define MRF_RETRY_FOREVER                  (255)
#define MRF_RETRY_ADHOC                    (3)

#define MRF_MAX_CHANNEL_LIST_LENGTH        (14)
#define MRF_MAX_SECURITY_KEY_LENGTH        (64)

#define MRF_RTS_THRESHOLD_MAX              (2347) /* maximum RTS threshold size in bytes */
#define MRF_MAX_NUM_RATES                  (8)

/* Key size defines */
#define MRF_MIN_WPA_PASS_PHRASE_LENGTH     (8) // must exclude string terminator
#define MRF_MAX_WPA_PASS_PHRASE_LENGTH     (63) // must exclude string terminator
#define MRF_WPA_KEY_LENGTH                 (32)

// WEP Key Lengths
#define MRF_WEP40_KEY_LENGTH               (20) // 4 keys of 5 bytes each
#define MRF_WEP104_KEY_LENGTH              (52) // 4 keys of 13 bytes each
#define MRF_MAX_WEP_KEY_LENGTH             (MRF_WEP104_KEY_LENGTH)

// WPS PIN Length
#define MRF_WPS_PIN_LENGTH                 8 // 7 digits + checksum byte

// eventInfo define for MRF_ProcessEvent() when no additional info is supplied
#define MRF_NO_ADDITIONAL_INFO             ((uint16_t)0xffff)

#endif
