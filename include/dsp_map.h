#pragma once

#include <Arduino.h>

#include "../dsp_export/Smart1466Ver5_IC_1_PARAM.h"
#include "../dsp_export/Smart1466Ver5_IC_1_REG.h"

namespace DspMap {

static constexpr uint16_t kChecksumPm = MOD_FRAMEWORK_PMCHECKSUM_ADDR;
static constexpr uint16_t kChecksumDm0 = MOD_FRAMEWORK_DM0CHECKSUM_ADDR;
static constexpr uint16_t kChecksumDm1 = MOD_FRAMEWORK_DM1CHECKSUM_ADDR;
static constexpr uint16_t kChecksumDone = MOD_FRAMEWORK_CHECKSUMCALCULATIONDONE_ADDR;

static constexpr uint16_t kRtaLeft = MOD_1XRTA1_ALG0_SINGLEBANDLEVELREADS3002LEVEL_ADDR;
static constexpr uint16_t kRtaRight = MOD_1XRTA2_ALG0_SINGLEBANDLEVELREADS3001LEVEL_ADDR;

static constexpr uint16_t kBtGain = MOD_BTGAIN_ALG0_TARGET_ADDR;
static constexpr uint16_t kOpticalGain = MOD_OPTICALGAIN_ALG0_TARGET_ADDR;
static constexpr uint16_t kXlrGain = MOD_XLRGAIN_ALG0_TARGET_ADDR;
static constexpr uint16_t kLpGain = MOD_LPGAIN_ALG0_TARGET_ADDR;
static constexpr uint16_t kMasterVolume = MOD_MASTER_ALG0_TARGET_ADDR;
static constexpr uint16_t kBassGainDb = MOD_MAINTONE_ALG0_HOLTERSALGSSTEREOSPTIS3001GAININ00_ADDR;
static constexpr uint16_t kTrebleGainDb = MOD_MAINTONE_ALG0_HOLTERSALGSSTEREOSPTIS3001GAININ01_ADDR;

static constexpr uint16_t kInputMux = MOD_NX2_3_STEREOMUXSIGMA300NS3INDEX_ADDR;
static constexpr uint16_t kMuteMain = MOD_MUTE1_MUTENOSLEWADAU145XALG1MUTE_ADDR;
static constexpr uint16_t kMuteSub = MOD_MUTE2_MUTENOSLEWADAU145XALG2MUTE_ADDR;
static constexpr uint16_t kOutputMuxA = MOD_MAINOUT_STEREOMUXSIGMA300NS1INDEX_ADDR;
static constexpr uint16_t kOutputMuxB = MOD_AUXOUT_STEREOMUXSIGMA300NS2INDEX_ADDR;
static constexpr uint16_t kMuteCenter = MOD_MUTE3_MUTENOSLEWADAU145XALG3MUTE_ADDR;
static constexpr uint16_t kAdiSurroundDil = MOD_ADISURROUND1_ALG0_ADISURROUNDALGS3001DIL_ADDR;
static constexpr uint16_t kAdiSurroundGain1 = MOD_ADISURROUND1_ALG0_ADISURROUNDALGS3001GAIN1_ADDR;
static constexpr uint16_t kAdiSurroundGain2 = MOD_ADISURROUND1_ALG0_ADISURROUNDALGS3001GAIN2_ADDR;
static constexpr uint16_t kAdiSurroundGain3 = MOD_ADISURROUND1_ALG0_ADISURROUNDALGS3001GAIN3_ADDR;
static constexpr uint16_t kAdiSurroundGain4 = MOD_ADISURROUND1_ALG0_ADISURROUNDALGS3001GAIN4_ADDR;

static constexpr uint16_t kSafeloadData = MOD_SAFELOADMODULE_DATA_SAFELOAD0_ADDR;
static constexpr uint16_t kSafeloadTarget = MOD_SAFELOADMODULE_ADDRESS_SAFELOAD_ADDR;
static constexpr uint16_t kSafeloadNumLower = MOD_SAFELOADMODULE_NUM_SAFELOAD_LOWER_ADDR;
static constexpr uint16_t kSafeloadNumUpper = MOD_SAFELOADMODULE_NUM_SAFELOAD_UPPER_ADDR;

static constexpr uint16_t kRegCoreStatus = REG_CORE_STATUS_IC_1_ADDR;
static constexpr uint16_t kRegPanicFlag = REG_PANIC_FLAG_IC_1_ADDR;
static constexpr uint16_t kRegPanicCode = REG_PANIC_CODE_IC_1_ADDR;
static constexpr uint16_t kRegAsrcInput0 = REG_ASRC_INPUT0_IC_1_ADDR;
static constexpr uint16_t kRegAsrcOutRate0 = REG_ASRC_OUT_RATE0_IC_1_ADDR;
static constexpr uint16_t kRegAsrcLock = REG_ASRC_LOCK_IC_1_ADDR;
static constexpr uint16_t kRegAsrcMute = REG_ASRC_MUTE_IC_1_ADDR;
static constexpr uint16_t kRegAsrc0Ratio = REG_ASRC0_RATIO_IC_1_ADDR;

}  // namespace DspMap
