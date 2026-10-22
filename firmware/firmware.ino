/* homeBert native HomeKit bridge for the Nubert nuXinema preAV. */
#include <Arduino.h>
#include <HomeSpan.h>
#include <NimBLEClient.h>
#include <NimBLEDevice.h>
#include <NimBLERemoteCharacteristic.h>
#include <NimBLERemoteService.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_mac.h>
#include <esp_heap_caps.h>
#include <qrcode.h>
#include <mbedtls/sha256.h>

#include "homebert_build_info.h"
#include "homebert_icon.h"

namespace {
constexpr char kFirmwareVersion[] = HOMEBERT_BUILD_VERSION;
constexpr char kFirmwareGitCommit[] = HOMEBERT_BUILD_GIT_COMMIT;
constexpr uint8_t kButtonPin = 39;
constexpr uint8_t kRgbPin = 27;
constexpr uint32_t kStatusDisplayMs = 5000;
constexpr uint32_t kFactoryResetHoldMs = 10000;
constexpr uint32_t kDoublePressWindowMs = 350;
constexpr uint32_t kButtonDebounceMs = 30;
constexpr uint32_t kStartupLedMs = 500;
constexpr uint32_t kBleLedBlinkHalfPeriodMs = 250;
constexpr uint32_t kWarningLedBlinkHalfPeriodMs = 500;
constexpr uint32_t kLedAlternatePeriodMs = 500;
constexpr uint32_t kInitialBleLedMinMs = 2000;
constexpr uint32_t kConnectedLedConfirmationMs = 5000;
constexpr uint32_t kCommandLedFlashMs = 100;
constexpr uint32_t kWiFiOutageLedDelayMs = 30000;
constexpr uint32_t kInitialWiFiTimeoutMs = 60000;
constexpr uint32_t kSetupConfirmationDisplayMs = 1500;
constexpr uint32_t kReconnectIntervalMs = 5000;
constexpr uint32_t kBleScanMs = 5000;
constexpr uint32_t kDiagnosticScanTimeoutMs = kBleScanMs + 1000;
constexpr uint32_t kBleReconnectMs = 5000;
constexpr uint32_t kNubertRefreshDelayMs = 500;
// A targeted status refresh is requested after a successful volume write.  This
// is a bounded fallback if neither that response nor a notification confirms it.
constexpr uint32_t kVolumeConfirmationTimeoutMs = 2000;
constexpr uint32_t kPowerOnTransitionMs = 10000;
constexpr uint8_t kMaxBleScanResults = 8;
constexpr uint8_t kMaxSetupWiFiNetworks = 32;
constexpr uint32_t kSetupWiFiScanTimeoutMs = 15000;
// An AD structure uses a one-byte length; a complete local name has at most 254 bytes.
constexpr size_t kMaxSetupBleNameLength = 254;
constexpr char kSetupApName[] = "homeBert Setup";
constexpr char kSetupHtmlContentType[] = "text/html; charset=utf-8";
constexpr char kNubertServiceUuid[] = "8e2ceaaa-0e27-11e7-93ae-92361f002671";
constexpr char kNubertCharacteristicUuid[] = "8e2cece4-0e27-11e7-93ae-92361f002671";
constexpr char kNubertAdvertisingServiceUuid[] = "0000a600-0000-1000-8000-00805f9b34fb";
constexpr char kHomeKitCodeDomain[] = "homeBert/setup-code/v1";
constexpr char kHomeKitSetupIdDomain[] = "homeBert/setup-id/v1";
// ESP-IDF NVS keys are limited to 15 characters.
constexpr char kHomeKitDerivationKey[] = "homekit_v";
constexpr uint8_t kHomeKitDerivationVersion = 1;

enum class LedColor : uint8_t { kOff, kWhite, kGreen, kBlue, kRed, kYellow };
enum class NubertPowerState : uint8_t { kUnknown, kOn, kStandby };
enum class NubertCommand : uint8_t {
  kNone,
  kPowerOn,
  kPowerStandby,
  kVolume,
  kInput,
  kSurround,
};
enum NubertQuery : uint8_t { kNoQuery = 0, kMainStatusQuery = 1, kSoundStatusQuery = 2 };
enum NubertStateChange : uint8_t {
  kNoStateChange = 0,
  kPowerChanged = 1,
  kVolumeChanged = 2,
  kInputChanged = 4,
  kSurroundChanged = 8,
};
enum class BleScanPurpose : uint8_t { kNone, kSetup, kDiagnostic };
enum class BleDiagnosticEvent : uint8_t {
  kNone,
  kConnectRequested,
  kConnected,
  kConnectFailed,
  kDisconnected,
};
struct RuntimeConfig {
  bool configured;
  bool wifiVerified;
  char ssid[33];
  char password[65];
  char bleAddress[18];
  uint8_t bleAddressType;
};

struct SetupBleCandidate {
  char name[kMaxSetupBleNameLength + 1];
  char address[18];
  uint8_t addressType;
  int rssi;
};

struct SetupWiFiNetwork {
  char ssid[33];
  int32_t rssi;
};

struct NubertDeviceState {
  NubertPowerState power = NubertPowerState::kUnknown;
  bool volumeValid = false;
  uint8_t volume = 0;
  bool inputValid = false;
  uint8_t input = 0;
  bool surroundValid = false;
  uint8_t surround = 0;
  bool bleConnected = false;
};

struct HomeKitVolumePending {
  bool active = false;
  bool writeSent = false;
  bool lastConfirmedValid = false;
  uint8_t target = 0;
  uint8_t lastConfirmed = 0;
  uint32_t deadline = 0;
};

struct HomeKitPowerPending {
  bool active = false;
  bool writeSent = false;
  bool lastConfirmedValid = false;
  NubertPowerState target = NubertPowerState::kUnknown;
  NubertPowerState lastConfirmed = NubertPowerState::kUnknown;
  uint32_t deadline = 0;
};

RuntimeConfig config{};
WebServer setupServer(80);
NimBLEScan* bleScan = nullptr;
NimBLEClient* bleClient = nullptr;
NimBLERemoteCharacteristic* nubertCharacteristic = nullptr;
bool homeKitStarted = false;
bool nimbleStarted = false;
bool setupPortalRunning = false;
bool bleScanRunning = false;
bool bleScanReady = false;
bool bleScanFailed = false;
SetupBleCandidate setupBleCandidates[kMaxBleScanResults]{};
uint8_t setupBleCandidateCount = 0;
bool setupBleCandidatesTruncated = false;
SetupBleCandidate setupSelectedBleCandidate{};
bool setupBleSelectionValid = false;
SetupWiFiNetwork setupWiFiNetworks[kMaxSetupWiFiNetworks]{};
uint8_t setupWiFiNetworkCount = 0;
bool setupWiFiNetworksTruncated = false;
bool setupWiFiScanStarted = false;
bool setupWiFiScanRunning = false;
bool setupWiFiScanFailed = false;
uint32_t setupWiFiScanStartedAt = 0;
BleScanPurpose bleScanPurpose = BleScanPurpose::kNone;
bool diagnosticScanPending = false;
uint32_t diagnosticScanStartedAt = 0;
bool diagnosticScanTimedOut = false;
bool bleConnecting = false;
bool bleGattLogged = false;
NubertDeviceState nubertState{};
HomeKitVolumePending homeKitVolumePending{};
HomeKitPowerPending homeKitPowerPending{};
volatile uint8_t nubertStateChanges = kNoStateChange;
NubertCommand pendingNubertCommand = NubertCommand::kNone;
uint8_t pendingNubertValue = 0;
uint8_t pendingNubertQueries = kNoQuery;
uint32_t nubertRefreshAt = 0;
bool automaticSurroundPending = false;
uint8_t automaticSurroundInput = 0;
bool homeKitPowerOnPending = false;
bool deferredHomeKitInputValid = false;
uint8_t deferredHomeKitInput = 0;
volatile bool bleConnectedEvent = false;
volatile bool bleConnectFailedEvent = false;
volatile bool bleDisconnectedEvent = false;
volatile int bleConnectionReason = 0;
uint8_t homeBertDiagnosticLevel = 0;
BleDiagnosticEvent lastBleDiagnosticEvent = BleDiagnosticEvent::kNone;
uint32_t lastBleDiagnosticEventAt = 0;
bool lastBleReasonValid = false;
int lastBleReason = 0;
uint32_t bleConnectionAttemptCount = 0;
uint32_t lastBleConnectionAttemptAt = 0;
char lastBleTargetAddress[18] = {};
uint8_t lastBleTargetAddressType = 0;
bool stationConnectionPending = false;
bool stationConnectRequested = false;
bool wasPressed = false;
bool rawButtonPressed = false;
bool factoryResetTriggered = false;
bool shortPressPending = false;
bool initialWiFiValidationFailed = false;
uint32_t pressedAt = 0;
uint32_t lastReleaseAt = 0;
uint32_t lastButtonEdgeAt = 0;
uint32_t statusUntil = 0;
uint32_t startupLedUntil = 0;
uint32_t commandLedUntil = 0;
uint32_t initialBleLedUntil = 0;
uint32_t connectedLedUntil = 0;
uint32_t wifiUnavailableSince = 0;
bool initialBleAttempted = false;
bool connectedLedPending = false;
LedColor displayedLed = LedColor::kOff;
uint32_t stationConnectStartedAt = 0;
uint32_t stationConnectRequestedAt = 0;
uint32_t lastReconnectAt = 0;
uint32_t bleReconnectAt = 0;
char homeKitSetupCode[9]{};
char homeKitSetupId[5]{};
bool homeKitSetupDerived = false;
String* homeKitQrSvgTarget = nullptr;

SpanCharacteristic* hkPower = nullptr;
SpanCharacteristic* hkInput = nullptr;
SpanCharacteristic* hkVolume = nullptr;

bool queueNubertCommand(NubertCommand command, uint8_t value = 0);
bool queueHomeKitInput(uint8_t input);
void requestNubertQueries(uint8_t queries);
void clearNubertState();
void acceptHomeKitVolume(uint8_t value);
void reconcilePendingHomeKitVolume();
void acceptHomeKitPower(NubertPowerState target);
void reconcilePendingHomeKitPower();
const char* powerStateName(NubertPowerState state);

bool isAllowedHomeKitSetupCode(const char* code) {
  if (strlen(code) != 8 || strspn(code, "0123456789") != 8) return false;
  if (strcmp(code, "12345678") == 0 || strcmp(code, "87654321") == 0) return false;
  for (uint8_t digit = 0; digit <= 9; ++digit) {
    bool allSame = true;
    for (uint8_t i = 0; i < 8; ++i) allSame &= code[i] == static_cast<char>('0' + digit);
    if (allSame) return false;
  }
  return true;
}

bool hashFactoryMac(const char* domain, const uint8_t mac[6], uint8_t retry,
                    uint8_t digest[32]) {
  uint8_t input[32]{};
  const size_t domainLength = strlen(domain);
  if (domainLength + 7 > sizeof(input)) return false;
  memcpy(input, domain, domainLength);
  memcpy(input + domainLength, mac, 6);
  input[domainLength + 6] = retry;
  return mbedtls_sha256(input, domainLength + 7, digest, 0) == 0;
}

uint32_t digestValue(const uint8_t digest[32]) {
  return (static_cast<uint32_t>(digest[0]) << 24) |
         (static_cast<uint32_t>(digest[1]) << 16) |
         (static_cast<uint32_t>(digest[2]) << 8) |
         static_cast<uint32_t>(digest[3]);
}

bool deriveHomeKitSetup() {
  if (homeKitSetupDerived) return true;
  uint8_t mac[6]{};
  if (esp_efuse_mac_get_default(mac) != ESP_OK) {
    Serial.println("ERROR: Factory MAC is unavailable; HomeKit setup cannot start.");
    return false;
  }

  for (uint16_t retry = 0; retry <= UINT8_MAX; ++retry) {
    uint8_t digest[32]{};
    if (!hashFactoryMac(kHomeKitCodeDomain, mac, static_cast<uint8_t>(retry), digest)) {
      return false;
    }
    snprintf(homeKitSetupCode, sizeof(homeKitSetupCode), "%08lu",
             static_cast<unsigned long>(digestValue(digest) % 100000000UL));
    if (isAllowedHomeKitSetupCode(homeKitSetupCode)) break;
    if (retry == UINT8_MAX) return false;
  }

  uint8_t digest[32]{};
  if (!hashFactoryMac(kHomeKitSetupIdDomain, mac, 0, digest)) return false;
  static constexpr char kSetupIdAlphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  uint32_t value = digestValue(digest);
  for (int i = 3; i >= 0; --i) {
    homeKitSetupId[i] = kSetupIdAlphabet[value % 36];
    value /= 36;
  }
  homeKitSetupId[4] = '\0';
  homeKitSetupDerived = true;
  return true;
}

void printHomeKitSetupInfo() {
  if (!deriveHomeKitSetup()) return;
  Serial.printf("HomeKit Setup Code: %.3s-%.2s-%.3s\n", homeKitSetupCode,
                homeKitSetupCode + 3, homeKitSetupCode + 5);
  Serial.printf("HomeKit Setup ID: %s\n", homeKitSetupId);
  HapQR qrCode;
  Serial.printf("HomeKit QR payload: %s\n",
                qrCode.get(static_cast<uint32_t>(strtoul(homeKitSetupCode, nullptr, 10)),
                           homeKitSetupId, static_cast<uint8_t>(Category::Television)));
}

bool homeKitQrPayload(char payload[21]) {
  if (!deriveHomeKitSetup()) return false;
  HapQR qrCode;
  strlcpy(payload,
          qrCode.get(static_cast<uint32_t>(strtoul(homeKitSetupCode, nullptr, 10)),
                     homeKitSetupId, static_cast<uint8_t>(Category::Television)),
          21);
  return true;
}

void renderHomeKitQrSvg(esp_qrcode_handle_t qrCode) {
  if (homeKitQrSvgTarget == nullptr) return;
  const int modules = esp_qrcode_get_size(qrCode);
  if (modules <= 0 || modules > 25) return;

  String& svg = *homeKitQrSvgTarget;
  const int quietZone = 4;
  const int span = modules + 2 * quietZone;
  svg.reserve(4096);
  svg += F("<svg class=homekit-qr viewBox=\"");
  svg += -quietZone;
  svg += ' ';
  svg += -quietZone;
  svg += ' ';
  svg += span;
  svg += ' ';
  svg += span;
  svg += F("\" width=198 height=198 role=img aria-label=\"HomeKit setup QR code\" shape-rendering=\"crispEdges\"><rect x=");
  svg += -quietZone;
  svg += F(" y=");
  svg += -quietZone;
  svg += F(" width=");
  svg += span;
  svg += F(" height=");
  svg += span;
  svg += F(" fill=\"#fff\"/><path fill=\"#000\" d=\"");
  for (int y = 0; y < modules; ++y) {
    for (int x = 0; x < modules;) {
      if (!esp_qrcode_get_module(qrCode, x, y)) {
        ++x;
        continue;
      }
      const int start = x;
      while (x < modules && esp_qrcode_get_module(qrCode, x, y)) ++x;
      svg += F("M");
      svg += start;
      svg += ',';
      svg += y;
      svg += F("h");
      svg += x - start;
      svg += F("v1h-");
      svg += x - start;
      svg += F("z");
    }
  }
  svg += F("\"/></svg>");
}

bool appendHomeKitQrSvg(String& page, const char* payload) {
  String svg;
  homeKitQrSvgTarget = &svg;
  esp_qrcode_config_t config = ESP_QRCODE_CONFIG_DEFAULT();
  config.display_func = renderHomeKitQrSvg;
  config.max_qrcode_version = 2;
  config.qrcode_ecc_level = ESP_QRCODE_ECC_MED;
  const esp_err_t result = esp_qrcode_generate(&config, payload);
  homeKitQrSvgTarget = nullptr;
  if (result != ESP_OK || svg.isEmpty()) return false;
  page += svg;
  return true;
}

bool homeKitDerivationCurrent() {
  Preferences prefs;
  if (!prefs.begin("homebert", true)) return false;
  const bool current = prefs.getUChar(kHomeKitDerivationKey, 0) == kHomeKitDerivationVersion;
  prefs.end();
  return current;
}

bool recordHomeKitDerivation() {
  Preferences prefs;
  if (!prefs.begin("homebert", false)) return false;
  const bool recorded = prefs.putUChar(kHomeKitDerivationKey, kHomeKitDerivationVersion) == 1;
  if (recorded) prefs.remove("homekit_code");
  prefs.end();
  return recorded;
}

bool configureHomeKitSetup() {
  if (!deriveHomeKitSetup()) return false;
  homeSpan.setQRID(homeKitSetupId);
  return true;
}

bool migrateHomeKitSetupIfNeeded() {
  if (homeKitDerivationCurrent()) return true;
  // HomeSpan stores only an SRP verifier.  Its controller records and accessory
  // identity are separate, so this migration leaves existing pairings intact.
  homeSpan.setPairingCode(homeKitSetupCode, false);
  if (!recordHomeKitDerivation()) {
    Serial.println("ERROR: HomeKit setup migration marker was not saved.");
    return false;
  }
  Serial.println("HomeKit setup credentials migrated to deterministic v1.");
  return true;
}

const char* bleAddressTypeName(uint8_t type) {
  switch (type) {
    case BLE_ADDR_PUBLIC: return "public";
    case BLE_ADDR_RANDOM: return "random";
    case BLE_ADDR_PUBLIC_ID: return "public-id";
    case BLE_ADDR_RANDOM_ID: return "random-id";
    default: return "unknown";
  }
}

const char* bleDiagnosticEventName(BleDiagnosticEvent event) {
  switch (event) {
    case BleDiagnosticEvent::kConnectRequested: return "connection requested";
    case BleDiagnosticEvent::kConnected: return "connected";
    case BleDiagnosticEvent::kConnectFailed: return "connection failed";
    case BleDiagnosticEvent::kDisconnected: return "disconnected";
    case BleDiagnosticEvent::kNone: return "none";
  }
  return "none";
}

void recordBleDiagnosticEvent(BleDiagnosticEvent event, bool hasReason = false,
                              int reason = 0) {
  lastBleDiagnosticEvent = event;
  lastBleDiagnosticEventAt = millis();
  lastBleReasonValid = hasReason;
  if (hasReason) lastBleReason = reason;

  if (homeBertDiagnosticLevel == 0) return;
  Serial.printf("BLE diagnostic [%lu ms]: %s", static_cast<unsigned long>(millis()),
                bleDiagnosticEventName(event));
  if (hasReason) Serial.printf("; reason=%d (0x%X)", reason, static_cast<unsigned>(reason));
  if (homeBertDiagnosticLevel >= 2 && event != BleDiagnosticEvent::kConnectRequested &&
      lastBleConnectionAttemptAt != 0) {
    Serial.printf("; last-attempt-age=%lu ms",
                  static_cast<unsigned long>(millis() - lastBleConnectionAttemptAt));
  }
  Serial.println(".");
}

void printBleIdentity(const char* label, const char* address, uint8_t type) {
  Serial.printf("%s: %s, type=%s (%u)\n", label,
                address[0] == '\0' ? "not configured" : address,
                bleAddressTypeName(type), static_cast<unsigned>(type));
}

bool diagnosticScanIsActive() {
  return diagnosticScanPending || bleScanPurpose == BleScanPurpose::kDiagnostic;
}

void startDiagnosticBleScan();
void restoreNormalBleScanConfiguration();

void clearSetupBleCandidates() {
  setupBleCandidateCount = 0;
  setupBleCandidatesTruncated = false;
}

void clearSetupBleSelection() {
  setupBleSelectionValid = false;
  setupSelectedBleCandidate.address[0] = '\0';
  setupSelectedBleCandidate.name[0] = '\0';
}

bool setupBleSelectionMatches(const SetupBleCandidate& candidate) {
  return setupBleSelectionValid &&
         setupSelectedBleCandidate.addressType == candidate.addressType &&
         strcmp(setupSelectedBleCandidate.address, candidate.address) == 0;
}

void validateSetupBleSelection() {
  if (!setupBleSelectionValid) return;
  for (uint8_t i = 0; i < setupBleCandidateCount; ++i) {
    if (setupBleSelectionMatches(setupBleCandidates[i])) {
      setupSelectedBleCandidate = setupBleCandidates[i];
      return;
    }
  }
  clearSetupBleSelection();
}

void selectSetupBleCandidate(const SetupBleCandidate& candidate) {
  setupSelectedBleCandidate = candidate;
  setupBleSelectionValid = true;
}

void clearSetupWiFiNetworks() {
  setupWiFiNetworkCount = 0;
  setupWiFiNetworksTruncated = false;
}

int findSetupWiFiNetwork(const String& ssid) {
  for (uint8_t i = 0; i < setupWiFiNetworkCount; ++i) {
    if (ssid == setupWiFiNetworks[i].ssid) return i;
  }
  return -1;
}

void sortSetupWiFiNetworks() {
  for (uint8_t i = 0; i < setupWiFiNetworkCount; ++i) {
    for (uint8_t j = i + 1; j < setupWiFiNetworkCount; ++j) {
      if (setupWiFiNetworks[j].rssi > setupWiFiNetworks[i].rssi) {
        const SetupWiFiNetwork temporary = setupWiFiNetworks[i];
        setupWiFiNetworks[i] = setupWiFiNetworks[j];
        setupWiFiNetworks[j] = temporary;
      }
    }
  }
}

void finishSetupWiFiScan(int16_t rawCount) {
  clearSetupWiFiNetworks();
  for (int16_t i = 0; i < rawCount; ++i) {
    const String ssid = WiFi.SSID(i);
    if (ssid.isEmpty() || ssid.length() >= sizeof(setupWiFiNetworks[0].ssid)) continue;
    const int existing = findSetupWiFiNetwork(ssid);
    const int32_t rssi = WiFi.RSSI(i);
    if (existing >= 0) {
      if (rssi > setupWiFiNetworks[existing].rssi) setupWiFiNetworks[existing].rssi = rssi;
      continue;
    }
    if (setupWiFiNetworkCount >= kMaxSetupWiFiNetworks) {
      setupWiFiNetworksTruncated = true;
      uint8_t weakest = 0;
      for (uint8_t j = 1; j < setupWiFiNetworkCount; ++j) {
        if (setupWiFiNetworks[j].rssi < setupWiFiNetworks[weakest].rssi) weakest = j;
      }
      if (rssi > setupWiFiNetworks[weakest].rssi) {
        ssid.toCharArray(setupWiFiNetworks[weakest].ssid,
                         sizeof(setupWiFiNetworks[weakest].ssid));
        setupWiFiNetworks[weakest].rssi = rssi;
      }
      continue;
    }
    SetupWiFiNetwork& network = setupWiFiNetworks[setupWiFiNetworkCount++];
    ssid.toCharArray(network.ssid, sizeof(network.ssid));
    network.rssi = rssi;
  }
  sortSetupWiFiNetworks();
  WiFi.scanDelete();
  setupWiFiScanRunning = false;
  setupWiFiScanFailed = false;
}

void startSetupWiFiScan() {
  if (!setupPortalRunning || setupWiFiScanRunning) return;
  clearSetupWiFiNetworks();
  setupWiFiScanStarted = true;
  setupWiFiScanFailed = false;
  const int16_t result = WiFi.scanNetworks(true, false, false, 120);
  if (result == WIFI_SCAN_RUNNING) {
    setupWiFiScanRunning = true;
    setupWiFiScanStartedAt = millis();
    return;
  }
  WiFi.scanDelete();
  setupWiFiScanRunning = false;
  setupWiFiScanFailed = true;
  Serial.printf("Wi-Fi setup scan could not start; status=%d.\n", result);
}

void serviceSetupWiFiScan() {
  if (!setupWiFiScanRunning) return;
  const int16_t result = WiFi.scanComplete();
  if (result >= 0) {
    finishSetupWiFiScan(result);
    Serial.printf("Wi-Fi setup scan finished: %u network(s).\n",
                  static_cast<unsigned>(setupWiFiNetworkCount));
  } else if (result == WIFI_SCAN_FAILED ||
             static_cast<uint32_t>(millis() - setupWiFiScanStartedAt) >=
                 kSetupWiFiScanTimeoutMs) {
    WiFi.scanDelete();
    setupWiFiScanRunning = false;
    setupWiFiScanFailed = true;
    Serial.println("Wi-Fi setup scan failed or timed out.");
  }
}

int findSetupBleCandidate(const std::string& address, uint8_t addressType) {
  for (uint8_t i = 0; i < setupBleCandidateCount; ++i) {
    if (setupBleCandidates[i].addressType == addressType &&
        strcmp(setupBleCandidates[i].address, address.c_str()) == 0) return i;
  }
  return -1;
}

class SetupScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* device) override {
    // NimBLE invokes onResult after an active-scan response, or at scan end when
    // no response arrived.  The advertised service and name may arrive in that
    // response, so do not filter early in onDiscovered.
    static const NimBLEUUID nubertAdvertisingService(kNubertAdvertisingServiceUuid);
    if (device == nullptr || !device->isConnectable() ||
        !device->isAdvertisingService(nubertAdvertisingService)) return;
    const std::string address = device->getAddress().toString();
    const int existing = findSetupBleCandidate(address, device->getAddressType());
    if (existing >= 0) {
      SetupBleCandidate& candidate = setupBleCandidates[existing];
      if (device->haveName()) {
        const std::string name = device->getName();
        if (name.size() <= kMaxSetupBleNameLength) {
          name.copy(candidate.name, name.size());
          candidate.name[name.size()] = '\0';
        }
      }
      candidate.rssi = device->getRSSI();
      return;
    }
    // Preserve the first eight matching identities in discovery order.
    if (setupBleCandidateCount >= kMaxBleScanResults) {
      setupBleCandidatesTruncated = true;
      return;
    }
    SetupBleCandidate& candidate = setupBleCandidates[setupBleCandidateCount++];
    candidate.name[0] = '\0';
    if (device->haveName()) {
      const std::string name = device->getName();
      if (name.size() <= kMaxSetupBleNameLength) {
        name.copy(candidate.name, name.size());
        candidate.name[name.size()] = '\0';
      }
    }
    strncpy(candidate.address, address.c_str(), sizeof(candidate.address) - 1);
    candidate.address[sizeof(candidate.address) - 1] = '\0';
    candidate.addressType = device->getAddressType();
    candidate.rssi = device->getRSSI();
  }
} setupScanCallbacks;

void showHomeBertDiagnostics(const char* command) {
  if (command == nullptr || command[0] != 'L') return;

  if (command[1] == '\0') {
    Serial.println("\n*** homeBert diagnostics ***");
    Serial.printf("Level: %u\n", static_cast<unsigned>(homeBertDiagnosticLevel));
    Serial.printf("Uptime: %lu ms\n", static_cast<unsigned long>(millis()));
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Wi-Fi: connected, IP=%s, RSSI=%d dBm\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
      Serial.println("Wi-Fi: disconnected");
    }
    Serial.printf("HomeKit: %s\n", homeKitStarted ? "started" : "not started");
    Serial.printf("BLE: %s\n", nubertState.bleConnected ? "connected" :
                  (bleConnecting ? "connecting" : "disconnected"));
    printBleIdentity("Configured peer", config.bleAddress, config.bleAddressType);
    printBleIdentity("Last connection target", lastBleTargetAddress,
                     lastBleTargetAddressType);
    Serial.printf("Connection attempts: %lu", static_cast<unsigned long>(bleConnectionAttemptCount));
    if (lastBleConnectionAttemptAt != 0) {
      Serial.printf("; last attempt %lu ms ago",
                    static_cast<unsigned long>(millis() - lastBleConnectionAttemptAt));
    }
    Serial.println();
    Serial.printf("Last BLE event: %s", bleDiagnosticEventName(lastBleDiagnosticEvent));
    if (lastBleDiagnosticEventAt != 0) {
      Serial.printf(" at %lu ms (%lu ms ago)",
                    static_cast<unsigned long>(lastBleDiagnosticEventAt),
                    static_cast<unsigned long>(millis() - lastBleDiagnosticEventAt));
    }
    Serial.println();
    if (lastBleReasonValid) {
      Serial.printf("Last BLE reason: %d (0x%X)\n", lastBleReason,
                    static_cast<unsigned>(lastBleReason));
    } else {
      Serial.println("Last BLE reason: none");
    }
    Serial.printf("Diagnostic scan: %s\n",
                  diagnosticScanPending ? "queued" :
                  (bleScanPurpose == BleScanPurpose::kDiagnostic ? "running" : "idle"));
    Serial.println("*** End homeBert diagnostics ***\n");
    return;
  }

  if (command[2] != '\0' || command[1] < '0' || command[1] > '2') {
    Serial.println("ERROR: Usage: @L, @L0, @L1 or @L2");
    return;
  }
  homeBertDiagnosticLevel = static_cast<uint8_t>(command[1] - '0');
  Serial.printf("homeBert diagnostic level set to %u (RAM only).\n",
                static_cast<unsigned>(homeBertDiagnosticLevel));
}

void requestBleIdentityScan(const char*) {
  if (diagnosticScanIsActive()) {
    Serial.println("BLE diagnostic scan is already queued or running.");
    return;
  }
  if (nubertState.bleConnected || (bleClient != nullptr && bleClient->isConnected())) {
    Serial.println("BLE diagnostic scan refused: nuXinema is connected.");
    return;
  }
  if (bleConnecting) {
    diagnosticScanPending = true;
    Serial.println("BLE diagnostic scan queued until the current connection attempt completes.");
    return;
  }
  startDiagnosticBleScan();
}

void showHomeBertStatus(const char*) {
  Serial.println("\n*** homeBert / nuXinema status ***");
  Serial.printf("Wi-Fi: %s\n", WiFi.status() == WL_CONNECTED ? "connected" : "disconnected");
  Serial.printf("BLE: %s\n", nubertState.bleConnected ? "connected" : "disconnected");
  Serial.printf("Power: %s\n", powerStateName(nubertState.power));
  if (nubertState.volumeValid) {
    Serial.printf("Volume: raw=%u dB=%d\n", nubertState.volume,
                  static_cast<int>(nubertState.volume) - 100);
  } else {
    Serial.println("Volume: unknown");
  }
  if (nubertState.inputValid) Serial.printf("Input: %u\n", nubertState.input);
  else Serial.println("Input: unknown");
  if (nubertState.surroundValid) Serial.printf("Surround: %u\n", nubertState.surround);
  else Serial.println("Surround: unknown");
  if (nubertState.bleConnected) {
    requestNubertQueries(kMainStatusQuery | kSoundStatusQuery);
    Serial.println("Read-only status refresh requested.");
  }
  Serial.println("*** End homeBert status ***\n");
}

void showHomeKitPairingCode(const char*) {
  printHomeKitSetupInfo();
}

struct HomeBertTelevision : Service::Television {
  HomeBertTelevision() : Service::Television() {
    hkPower = new Characteristic::Active(0);
    hkInput = new Characteristic::ActiveIdentifier(1);
    TLV8 inputOrder;
    // Nubert remote order: HDMI (eARC, 1, 2, 3), Coax, Bluetooth, AUX, Optical, Port.
    static constexpr uint8_t kInputOrder[] = {1, 2, 3, 4, 7, 6, 5, 8, 9};
    for (uint8_t i = 0; i < sizeof(kInputOrder); ++i) {
      inputOrder.add(1, kInputOrder[i]);
      if (i + 1 < sizeof(kInputOrder)) inputOrder.add(0);
    }
    new Characteristic::DisplayOrder(inputOrder);
    new Characteristic::ConfiguredName("nuXinema");
  }

  boolean update() override {
    if (hkPower->updated()) {
      const bool requestedOn = hkPower->getNewVal();
      if (requestedOn) {
        if (!queueNubertCommand(NubertCommand::kPowerOn)) return false;
        acceptHomeKitPower(NubertPowerState::kOn);
        homeKitPowerOnPending = true;
      } else {
        if (!queueNubertCommand(NubertCommand::kPowerStandby)) return false;
        acceptHomeKitPower(NubertPowerState::kStandby);
        deferredHomeKitInputValid = false;
        homeKitPowerOnPending = false;
      }
      return true;
    }
    if (hkInput->updated()) {
      const uint32_t identifier = hkInput->getNewVal();
      if (identifier >= 1 && identifier <= 9) queueHomeKitInput(identifier - 1);
      return false;
    }
    return true;
  }
};

struct HomeBertVolume : Service::Fan {
  SpanCharacteristic* active;

  HomeBertVolume() : Service::Fan() {
    active = new Characteristic::Active(1);
    hkVolume = new Characteristic::RotationSpeed(0);
    new Characteristic::ConfiguredName("Volume");
  }

  boolean update() override {
    // Fan Active is intentionally unrelated to receiver power and volume writes.
    if (active->updated()) return false;
    if (hkVolume->updated()) {
      const uint8_t requested = hkVolume->getNewVal();
      if (!queueNubertCommand(NubertCommand::kVolume, requested)) return false;
      acceptHomeKitVolume(requested);
      return true;
    }
    return true;
  }
};

struct HomeBertVolumeButtons : Service::TelevisionSpeaker {
  SpanCharacteristic* selector;

  HomeBertVolumeButtons() : Service::TelevisionSpeaker() {
    new Characteristic::VolumeControlType(3);
    selector = new Characteristic::VolumeSelector();
  }

  boolean update() override {
    if (!selector->updated() || !nubertState.volumeValid) return false;
    const int delta = selector->getNewVal() == 0 ? 1 : -1;
    const int requested = static_cast<int>(nubertState.volume) + delta;
    if (requested < 0 || requested > 100 ||
        !queueNubertCommand(NubertCommand::kVolume, requested)) return false;
    return false;
  }
};

struct HomeBertInput : Service::InputSource {
  HomeBertInput(uint8_t id, const char* name) : Service::InputSource() {
    new Characteristic::Identifier(id);
    new Characteristic::ConfiguredName(name);
    new Characteristic::IsConfigured(1);
    new Characteristic::CurrentVisibilityState(0);
  }
};

class BleClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient*) override { bleConnectedEvent = true; }

  void onConnectFail(NimBLEClient*, int reason) override {
    bleConnectionReason = reason;
    bleConnectFailedEvent = true;
  }

  void onDisconnect(NimBLEClient*, int reason) override {
    bleConnectionReason = reason;
    bleDisconnectedEvent = true;
  }
} bleClientCallbacks;

void setLed(LedColor color) {
  if (color == displayedLed) return;
  displayedLed = color;
  uint8_t red = 0, green = 0, blue = 0;
  switch (color) {
    case LedColor::kWhite: red = green = blue = 24; break;
    case LedColor::kGreen: green = 24; break;
    case LedColor::kBlue: blue = 24; break;
    case LedColor::kRed: red = 24; break;
    case LedColor::kYellow: red = green = 24; break;
    case LedColor::kOff: break;
  }
  rgbLedWrite(kRgbPin, red, green, blue);
}

bool ledPhase(uint32_t periodMs) {
  return (millis() / periodMs) % 2 == 0;
}

void serviceLed() {
  const uint32_t now = millis();
  LedColor color = LedColor::kOff;

  // Setup and warnings always take precedence over activity and normal status.
  if (initialWiFiValidationFailed && setupPortalRunning) {
    color = ledPhase(kLedAlternatePeriodMs) ? LedColor::kYellow : LedColor::kRed;
  } else if (setupPortalRunning) {
    color = ledPhase(kWarningLedBlinkHalfPeriodMs) ? LedColor::kYellow : LedColor::kOff;
  } else if (config.wifiVerified && WiFi.status() != WL_CONNECTED &&
             wifiUnavailableSince != 0 &&
             static_cast<uint32_t>(now - wifiUnavailableSince) >= kWiFiOutageLedDelayMs) {
    color = ledPhase(kWarningLedBlinkHalfPeriodMs) ? LedColor::kRed : LedColor::kOff;
  } else if (startupLedUntil != 0 &&
             static_cast<int32_t>(now - startupLedUntil) < 0) {
    color = LedColor::kWhite;
  } else if (initialBleAttempted && initialBleLedUntil == 0) {
    initialBleLedUntil = now + kInitialBleLedMinMs;
    color = ledPhase(kBleLedBlinkHalfPeriodMs) ? LedColor::kBlue : LedColor::kOff;
  } else if (initialBleLedUntil != 0 &&
             (static_cast<int32_t>(now - initialBleLedUntil) < 0 || bleConnecting)) {
    color = ledPhase(kBleLedBlinkHalfPeriodMs) ? LedColor::kBlue : LedColor::kOff;
  } else if (statusUntil != 0 && static_cast<int32_t>(now - statusUntil) < 0) {
    color = WiFi.status() != WL_CONNECTED ? LedColor::kRed
          : nubertState.bleConnected ? LedColor::kGreen : LedColor::kBlue;
  } else if (commandLedUntil != 0 &&
             static_cast<int32_t>(now - commandLedUntil) < 0) {
    color = LedColor::kBlue;
  } else if (connectedLedPending && WiFi.status() == WL_CONNECTED &&
             nubertState.bleConnected) {
    connectedLedPending = false;
    connectedLedUntil = now + kConnectedLedConfirmationMs;
    color = LedColor::kGreen;
  } else if (connectedLedUntil != 0 &&
             static_cast<int32_t>(now - connectedLedUntil) < 0) {
    color = LedColor::kGreen;
  } else if (WiFi.status() == WL_CONNECTED && bleConnecting) {
    color = ledPhase(kBleLedBlinkHalfPeriodMs) ? LedColor::kBlue : LedColor::kOff;
  }
  setLed(color);
}

void reportMemory(const char *label) {
  Serial.printf("MEM %s free=%lu min=%lu largest=%lu\n", label,
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getMinFreeHeap()),
                static_cast<unsigned long>(
                    heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)));
}

bool loadConfig() {
  Preferences prefs;
  if (!prefs.begin("homebert", true)) return false;
  config.configured = prefs.getBool("configured", false);
  config.wifiVerified = prefs.getBool("wifi_verified", false);
  prefs.getString("ssid", config.ssid, sizeof(config.ssid));
  prefs.getString("password", config.password, sizeof(config.password));
  prefs.getString("ble_address", config.bleAddress, sizeof(config.bleAddress));
  config.bleAddressType = prefs.getUChar("ble_addr_type", 0);
  prefs.end();
  return config.configured && config.ssid[0] != '\0';
}

bool saveConfig(const String &ssid, const String &password,
                const SetupBleCandidate *device) {
  if (ssid.isEmpty() || ssid.length() >= sizeof(config.ssid) ||
      password.length() >= sizeof(config.password) || device == nullptr) {
    return false;
  }
  Preferences prefs;
  if (!prefs.begin("homebert", false)) return false;
  prefs.putBool("configured", true);
  prefs.putBool("wifi_verified", false);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.putString("ble_address", device->address);
  prefs.putUChar("ble_addr_type", device->addressType);
  prefs.end();
  ssid.toCharArray(config.ssid, sizeof(config.ssid));
  password.toCharArray(config.password, sizeof(config.password));
  strncpy(config.bleAddress, device->address, sizeof(config.bleAddress) - 1);
  config.bleAddress[sizeof(config.bleAddress) - 1] = '\0';
  config.bleAddressType = device->addressType;
  config.configured = true;
  config.wifiVerified = false;
  initialWiFiValidationFailed = false;
  return true;
}

void startHomeKit() {
  if (homeKitStarted) return;
  if (!configureHomeKitSetup()) return;
  homeSpan.setLogLevel(0);
  homeSpan.setSketchVersion(kFirmwareVersion);
  homeSpan.begin(Category::Television, "homeBert");
  // HAP initialization occurs in the first poll(), so this records the
  // deterministic verifier after HomeSpan knows its category but before it can
  // create a default verifier.
  if (!migrateHomeKitSetupIfNeeded()) return;
  SPAN_ACCESSORY("homeBert");
  new Characteristic::FirmwareRevision(kFirmwareVersion);
  SpanService* inputs[] = {
      new HomeBertInput(1, "eARC"), new HomeBertInput(2, "HDMI 1"),
      new HomeBertInput(3, "HDMI 2"), new HomeBertInput(4, "HDMI 3"),
      new HomeBertInput(5, "AUX"), new HomeBertInput(6, "Bluetooth"),
      new HomeBertInput(7, "Coax"), new HomeBertInput(8, "Optical"),
      new HomeBertInput(9, "Port"),
  };
  HomeBertVolumeButtons* volumeButtons = new HomeBertVolumeButtons();
  HomeBertTelevision* television = new HomeBertTelevision();
  for (SpanService* input : inputs) television->addLink(input);
  television->addLink(volumeButtons);
  new HomeBertVolume();
  new SpanUserCommand('n', "show homeBert and nuXinema status", showHomeBertStatus);
  new SpanUserCommand('p', "show HomeKit setup information", showHomeKitPairingCode);
  new SpanUserCommand('L', "show/set homeBert diagnostics (@L[0-2])",
                      showHomeBertDiagnostics);
  new SpanUserCommand('b', "scan BLE identities without changing configuration",
                      requestBleIdentityScan);
  homeKitStarted = true;
  reportMemory("homespan-started");
}

void startNimble() {
  if (nimbleStarted) return;
  NimBLEDevice::init("homeBert");
  bleScan = NimBLEDevice::getScan();
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(50);
  bleScan->setMaxResults(kMaxBleScanResults);
  nimbleStarted = true;
  reportMemory("nimble-started");
}

void startBleScan(BleScanPurpose purpose = BleScanPurpose::kSetup) {
  if (bleScan == nullptr || bleScanRunning) return;
  bleScanReady = false;
  bleScanFailed = false;
  bleScan->clearResults();
  bleScanPurpose = purpose;
  if (purpose == BleScanPurpose::kSetup) {
    clearSetupBleCandidates();
    bleScan->setScanCallbacks(&setupScanCallbacks);
    bleScan->setMaxResults(0);  // Callback-only; candidate limit is applied after filtering.
  }
  if (purpose == BleScanPurpose::kDiagnostic) diagnosticScanTimedOut = false;
  bleScanFailed = !bleScan->start(kBleScanMs, false);
  bleScanRunning = !bleScanFailed;
  if (bleScanRunning) {
    if (purpose == BleScanPurpose::kDiagnostic) diagnosticScanStartedAt = millis();
    reportMemory("ble-scan-started");
    if (purpose == BleScanPurpose::kDiagnostic) {
      printBleIdentity("BLE diagnostic configured peer", config.bleAddress,
                       config.bleAddressType);
      Serial.println("BLE diagnostic scan started (five seconds; no connection will be made).");
    } else {
      Serial.println("BLE scan started.");
    }
  } else {
    bleScanPurpose = BleScanPurpose::kNone;
    if (purpose == BleScanPurpose::kSetup) restoreNormalBleScanConfiguration();
    if (purpose == BleScanPurpose::kDiagnostic) {
      diagnosticScanPending = false;
      Serial.println("BLE diagnostic scan failed to start; automatic reconnect resumed.");
    } else {
      Serial.println("BLE scan failed to start.");
    }
  }
}

void printDiagnosticScanResults() {
  const NimBLEScanResults results = bleScan->getResults();
  bool exactMatch = false;
  uint8_t reported = 0;
  Serial.printf("BLE diagnostic scan finished: %d cached result(s).\n", results.getCount());
  for (int i = 0; i < results.getCount() && reported < kMaxBleScanResults; ++i) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device == nullptr || !device->isConnectable()) continue;
    const std::string address = device->getAddress().toString();
    const bool match = config.bleAddress[0] != '\0' &&
                       strcmp(address.c_str(), config.bleAddress) == 0 &&
                       device->getAddressType() == config.bleAddressType;
    const std::string name = device->haveName() ? device->getName() : "";
    Serial.printf("  %u: name=%.32s address=%s type=%s (%u) RSSI=%d exact-match=%s\n",
                  static_cast<unsigned>(reported + 1),
                  name.empty() ? "(none)" : name.c_str(), address.c_str(),
                  bleAddressTypeName(device->getAddressType()),
                  static_cast<unsigned>(device->getAddressType()), device->getRSSI(),
                  match ? "yes" : "no");
    exactMatch = exactMatch || match;
    ++reported;
  }
  if (reported == 0) Serial.println("  No connectable BLE devices observed.");
  Serial.printf("BLE diagnostic exact configured identity observed: %s.\n",
                exactMatch ? "yes" : "no");
}

void finishDiagnosticBleScan(bool timedOut) {
  bleScanRunning = false;
  diagnosticScanPending = false;
  bleScanPurpose = BleScanPurpose::kNone;
  diagnosticScanStartedAt = 0;
  if (timedOut) Serial.println("BLE diagnostic scan timed out; results collected so far:");
  printDiagnosticScanResults();
  bleScan->clearResults();
  Serial.println("BLE diagnostic scan complete; automatic reconnect resumed.");
}

void startDiagnosticBleScan() {
  if (bleScan == nullptr || bleScanRunning || bleConnecting || nubertState.bleConnected) return;
  diagnosticScanPending = true;
  startBleScan(BleScanPurpose::kDiagnostic);
}

void restoreNormalBleScanConfiguration() {
  bleScan->setScanCallbacks(nullptr);
  bleScan->setMaxResults(kMaxBleScanResults);
  bleScan->setActiveScan(true);
  bleScan->setInterval(100);
  bleScan->setWindow(50);
}

void serviceBleScan() {
  if (bleScanRunning && bleScanPurpose == BleScanPurpose::kDiagnostic &&
      static_cast<uint32_t>(millis() - diagnosticScanStartedAt) >= kDiagnosticScanTimeoutMs) {
    Serial.println("BLE diagnostic scan timeout reached; stopping scan.");
    diagnosticScanTimedOut = true;
    if (!bleScan->stop()) finishDiagnosticBleScan(true);
  }
  if (bleScanRunning && !bleScan->isScanning()) {
    if (bleScanPurpose == BleScanPurpose::kDiagnostic) {
      finishDiagnosticBleScan(diagnosticScanTimedOut);
    } else {
      const bool setupScan = bleScanPurpose == BleScanPurpose::kSetup;
      bleScanRunning = false;
      bleScanPurpose = BleScanPurpose::kNone;
      bleScanReady = true;
      if (setupScan) restoreNormalBleScanConfiguration();
      reportMemory("ble-scan-finished");
      Serial.printf("BLE scan finished: %u selectable result(s).\n",
                    static_cast<unsigned>(setupBleCandidateCount));
    }
  }
}

void logHex(const char *label, const uint8_t *data, size_t length) {
  Serial.print(label);
  for (size_t i = 0; i < length; ++i) Serial.printf("%02X", data[i]);
  Serial.println();
}

const char* powerStateName(NubertPowerState state) {
  switch (state) {
    case NubertPowerState::kOn: return "ON";
    case NubertPowerState::kStandby: return "STANDBY";
    case NubertPowerState::kUnknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

void acceptHomeKitPower(NubertPowerState target) {
  homeKitPowerPending.active = true;
  homeKitPowerPending.writeSent = false;
  homeKitPowerPending.target = target;
  homeKitPowerPending.deadline = 0;
  homeKitPowerPending.lastConfirmedValid = nubertState.power != NubertPowerState::kUnknown;
  homeKitPowerPending.lastConfirmed = nubertState.power;
}

void reconcilePendingHomeKitPower() {
  if (!homeKitPowerPending.active) return;
  const bool confirmed = homeKitPowerPending.lastConfirmedValid;
  const NubertPowerState state = homeKitPowerPending.lastConfirmed;
  const NubertPowerState target = homeKitPowerPending.target;
  homeKitPowerPending = {};
  if (target == NubertPowerState::kOn) {
    homeKitPowerOnPending = false;
    deferredHomeKitInputValid = false;
  }
  // Do not substitute a cached state after a disconnect. A subsequent GATT
  // synchronization will publish the receiver's current state.
  if (confirmed && nubertState.power != NubertPowerState::kUnknown) {
    nubertState.power = state;
    nubertStateChanges |= kPowerChanged;
  }
}

void setNubertPowerState(NubertPowerState state) {
  const bool changed = nubertState.power != state;
  nubertState.power = state;
  bool publish = changed;
  if (homeKitPowerPending.active) {
    homeKitPowerPending.lastConfirmed = state;
    homeKitPowerPending.lastConfirmedValid = state != NubertPowerState::kUnknown;
    const bool queuedPowerOnAlreadyConfirmed =
        state == NubertPowerState::kOn &&
        homeKitPowerPending.target == NubertPowerState::kOn &&
        pendingNubertCommand == NubertCommand::kPowerOn;
    if (state == homeKitPowerPending.target &&
        (homeKitPowerPending.writeSent || queuedPowerOnAlreadyConfirmed)) {
      if (queuedPowerOnAlreadyConfirmed) pendingNubertCommand = NubertCommand::kNone;
      const NubertPowerState target = homeKitPowerPending.target;
      homeKitPowerPending = {};
      if (target == NubertPowerState::kOn) {
        homeKitPowerOnPending = false;
        if (deferredHomeKitInputValid) {
          const uint8_t input = deferredHomeKitInput;
          deferredHomeKitInputValid = false;
          queueHomeKitInput(input);
        }
      }
      publish = true;
    } else {
      // Keep HomeKit's accepted request visible until it is confirmed or the
      // bounded reconciliation path restores the last receiver state.
      publish = false;
    }
  }
  if (publish) nubertStateChanges |= kPowerChanged;
  if (state != NubertPowerState::kOn) {
    automaticSurroundPending = false;
    if (state == NubertPowerState::kUnknown) {
      deferredHomeKitInputValid = false;
      homeKitPowerOnPending = false;
      homeKitPowerPending = {};
    }
    if (pendingNubertCommand != NubertCommand::kPowerOn ||
        state == NubertPowerState::kUnknown) {
      pendingNubertCommand = NubertCommand::kNone;
    }
  }
  if (changed) Serial.printf("Nubert power state: %s.\n", powerStateName(state));
}

uint8_t surroundForInput(uint8_t input) { return input <= 3 ? 5 : 0; }

void acceptHomeKitVolume(uint8_t value) {
  homeKitVolumePending.active = true;
  homeKitVolumePending.writeSent = false;
  homeKitVolumePending.target = value;
  homeKitVolumePending.deadline = 0;
  homeKitVolumePending.lastConfirmedValid = nubertState.volumeValid;
  homeKitVolumePending.lastConfirmed = nubertState.volume;
}

void reconcilePendingHomeKitVolume() {
  if (!homeKitVolumePending.active) return;
  const bool confirmed = homeKitVolumePending.lastConfirmedValid;
  const uint8_t value = homeKitVolumePending.lastConfirmed;
  homeKitVolumePending = {};
  // Do not substitute a cached value while disconnected.  A later GATT
  // synchronization will publish the receiver's actual value.
  if (confirmed && nubertState.volumeValid) {
    nubertState.volume = value;
    nubertStateChanges |= kVolumeChanged;
  }
}

void setNubertVolume(uint8_t value) {
  const bool changed = !nubertState.volumeValid || nubertState.volume != value;
  nubertState.volume = value;
  nubertState.volumeValid = true;

  if (homeKitVolumePending.active) {
    homeKitVolumePending.lastConfirmed = value;
    homeKitVolumePending.lastConfirmedValid = true;
    // Before the queued write has actually left the ESP32, even a matching
    // notification cannot confirm this request.  Once sent, only the newest
    // requested target completes the transaction.  Other feedback is retained
    // as the authoritative fallback, but not published until confirmation or
    // the bounded reconciliation deadline.
    if (homeKitVolumePending.writeSent && value == homeKitVolumePending.target) {
      homeKitVolumePending = {};
      nubertStateChanges |= kVolumeChanged;
    }
  } else if (changed) {
    nubertStateChanges |= kVolumeChanged;
  }

  if (changed) {
    Serial.printf("Nubert volume raw=%u dB=%d.\n", value,
                  static_cast<int>(value) - 100);
  }
}

void setNubertValue(uint8_t opcode, uint8_t value) {
  switch (opcode) {
    case 0x1E:
      if (value == 0x00) setNubertPowerState(NubertPowerState::kOn);
      if (value == 0x01) setNubertPowerState(NubertPowerState::kStandby);
      break;
    case 0x0A:
      setNubertVolume(value);
      break;
    case 0x0E:
      if (value <= 8 && (!nubertState.inputValid || nubertState.input != value)) {
        nubertState.input = value;
        nubertState.inputValid = true;
        nubertStateChanges |= kInputChanged;
        Serial.printf("Nubert input=%u.\n", value);
        if (automaticSurroundPending && value == automaticSurroundInput) {
          automaticSurroundPending = false;
          const uint8_t surround = surroundForInput(value);
          if (!nubertState.surroundValid || nubertState.surround != surround) {
            queueNubertCommand(NubertCommand::kSurround, surround);
          }
        }
      }
      break;
    case 0x93:
      if (!nubertState.surroundValid || nubertState.surround != value) {
        nubertState.surround = value;
        nubertState.surroundValid = true;
        nubertStateChanges |= kSurroundChanged;
        Serial.printf("Nubert surround=%u.\n", value);
      }
      break;
  }
}

void parseNubertRecords(const uint8_t* data, size_t begin, size_t end) {
  for (size_t i = begin; i + 2 < end;) {
    const uint8_t valueLength = data[i + 1];
    if (i + 2 + valueLength > end) break;
    if (valueLength == 1) setNubertValue(data[i], data[i + 2]);
    i += 2 + valueLength;
  }
}

void parseNubertMessage(const uint8_t* data, size_t length) {
  if (length >= 2 && data[0] == 0x86) {
    const size_t end = min(length, static_cast<size_t>(data[1]) + 2);
    parseNubertRecords(data, 2, end);
  } else {
    parseNubertRecords(data, 0, length);
  }
}

bool allowNubertDeviceCommand(NubertCommand command) {
  const bool transportReady = nubertState.bleConnected && bleClient != nullptr &&
                              bleClient->isConnected() && nubertCharacteristic != nullptr;
  const bool allowed = transportReady && (command == NubertCommand::kPowerOn
                           ? nubertState.power == NubertPowerState::kStandby
                           : command == NubertCommand::kPowerStandby
                                 ? nubertState.power == NubertPowerState::kOn
                                 : nubertState.power == NubertPowerState::kOn);
  if (!allowed) {
    Serial.printf("Nubert device command blocked: power state=%s.\n",
                  powerStateName(nubertState.power));
  }
  return allowed;
}

void onNubertNotification(NimBLERemoteCharacteristic*, uint8_t *data,
                          size_t length, bool) {
  logHex("Nubert notification: ", data, length);
  parseNubertMessage(data, length);
}

void requestNubertQueries(uint8_t queries) { pendingNubertQueries |= queries; }

void publishNubertState() {
  const uint8_t changes = nubertStateChanges;
  if (changes == kNoStateChange || !homeKitStarted) return;
  nubertStateChanges = kNoStateChange;
  if ((changes & kPowerChanged) && nubertState.power != NubertPowerState::kUnknown) {
    hkPower->setVal(nubertState.power == NubertPowerState::kOn);
  }
  if ((changes & kVolumeChanged) && nubertState.volumeValid) hkVolume->setVal(nubertState.volume);
  if ((changes & kInputChanged) && nubertState.inputValid) hkInput->setVal(nubertState.input + 1);
}

void clearNubertState() {
  setNubertPowerState(NubertPowerState::kUnknown);
  nubertState.volumeValid = false;
  nubertState.inputValid = false;
  nubertState.surroundValid = false;
  nubertState.bleConnected = false;
  pendingNubertCommand = NubertCommand::kNone;
  pendingNubertQueries = kNoQuery;
  nubertRefreshAt = 0;
  automaticSurroundPending = false;
  deferredHomeKitInputValid = false;
  homeKitPowerOnPending = false;
  homeKitPowerPending = {};
  homeKitVolumePending = {};
}

bool logGattDatabase() {
  if (bleClient == nullptr || !bleClient->isConnected()) return false;
  const NimBLEUUID nubertService(kNubertServiceUuid);
  const NimBLEUUID nubertCharacteristicUuid(kNubertCharacteristicUuid);
  nubertCharacteristic = nullptr;
  const auto& services = bleClient->getServices(true);
  Serial.printf("BLE GATT services: %u.\n",
                static_cast<unsigned>(services.size()));
  for (NimBLERemoteService* service : services) {
    Serial.printf("BLE GATT service: %s.\n",
                  service->getUUID().toString().c_str());
    const auto& characteristics = service->getCharacteristics(true);
    for (NimBLERemoteCharacteristic* characteristic : characteristics) {
      Serial.printf("BLE GATT characteristic: %s read=%u write=%u write_nr=%u"
                    " notify=%u indicate=%u.\n",
                    characteristic->getUUID().toString().c_str(),
                    static_cast<unsigned>(characteristic->canRead()),
                    static_cast<unsigned>(characteristic->canWrite()),
                    static_cast<unsigned>(characteristic->canWriteNoResponse()),
                    static_cast<unsigned>(characteristic->canNotify()),
                    static_cast<unsigned>(characteristic->canIndicate()));
      if (service->getUUID().equals(nubertService) &&
          characteristic->getUUID().equals(nubertCharacteristicUuid)) {
        nubertCharacteristic = characteristic;
      }
    }
  }
  bool verified = false;
  if (nubertCharacteristic != nullptr) {
    const bool expectedProperties = nubertCharacteristic->canRead() &&
                                    nubertCharacteristic->canWrite() &&
                                    nubertCharacteristic->canNotify();
    if (!expectedProperties) {
      Serial.println("Nubert control characteristic has unexpected properties.");
    }
    const NimBLEAttValue value = nubertCharacteristic->readValue();
    logHex("Nubert initial value: ", value.data(), value.size());
    parseNubertMessage(value.data(), value.size());
    const bool subscribed = nubertCharacteristic->subscribe(
        true, onNubertNotification, true);
    Serial.printf("Nubert notification subscribe=%u.\n",
                  static_cast<unsigned>(subscribed));
    requestNubertQueries(kMainStatusQuery | kSoundStatusQuery);
    verified = expectedProperties && subscribed;
  } else {
    Serial.println("Nubert control characteristic not found.");
  }
  bleGattLogged = true;
  reportMemory("ble-gatt-discovered");
  return verified;
}

bool writeNubert(const uint8_t* command, size_t length, const char* label) {
  if (bleClient == nullptr || !bleClient->isConnected() ||
      nubertCharacteristic == nullptr) {
    Serial.printf("%s unavailable: Nubert characteristic is disconnected.\n", label);
    return false;
  }
  const bool written = nubertCharacteristic->writeValue(command, length, true);
  Serial.printf("%s write-with-response result=%u.\n", label,
                static_cast<unsigned>(written));
  return written;
}

bool sendStatusQuery() {
  static constexpr uint8_t kStatusCommand[] = {
      0x86, 0x0D, 0x1E, 0x0A, 0x20, 0x22, 0x34, 0x0E,
      0x44, 0x60, 0x4A, 0x62, 0x58, 0xC6, 0xE8,
  };
  return writeNubert(kStatusCommand, sizeof(kStatusCommand), "Status query");
}

bool sendSoundStatusQuery() {
  static constexpr uint8_t kSoundStatusCommand[] = {
      0x86, 0x07, 0x22, 0x20, 0x24, 0x2A, 0x74, 0x93, 0x95,
  };
  return writeNubert(kSoundStatusCommand, sizeof(kSoundStatusCommand),
                     "Sound status query");
}

bool queueNubertCommand(NubertCommand command, uint8_t value) {
  if (command == NubertCommand::kNone || !allowNubertDeviceCommand(command)) return false;
  if (pendingNubertCommand != NubertCommand::kNone) {
    if (command == NubertCommand::kVolume &&
        pendingNubertCommand == NubertCommand::kVolume) {
      pendingNubertValue = value;
      return true;
    }
    Serial.println("Nubert device command blocked: another command is pending.");
    return false;
  }
  pendingNubertCommand = command;
  pendingNubertValue = value;
  return true;
}

bool queueHomeKitInput(uint8_t input) {
  if (nubertState.inputValid && nubertState.input == input) return false;
  if (nubertState.power == NubertPowerState::kStandby && homeKitPowerOnPending) {
    deferredHomeKitInput = input;
    deferredHomeKitInputValid = true;
    Serial.printf("HomeKit input deferred until verified ON: %u.\n", input);
    return true;
  }
  if (!queueNubertCommand(NubertCommand::kInput, input)) return false;
  automaticSurroundInput = input;
  automaticSurroundPending = true;
  return true;
}

bool sendPendingNubertCommand() {
  uint8_t command[3]{};
  const char* label = "Nubert command";
  switch (pendingNubertCommand) {
    case NubertCommand::kPowerOn:
      command[0] = 0x1F;
      command[1] = 0x01;
      command[2] = 0x00;
      label = "Power ON";
      break;
    case NubertCommand::kPowerStandby:
      command[0] = 0x1F;
      command[1] = 0x01;
      command[2] = 0x01;
      label = "Power STANDBY";
      break;
    case NubertCommand::kVolume:
      command[0] = 0x0B;
      command[1] = 0x01;
      command[2] = pendingNubertValue;
      label = "Volume";
      break;
    case NubertCommand::kInput:
      command[0] = 0x0F;
      command[1] = 0x01;
      command[2] = pendingNubertValue;
      label = "Input";
      break;
    case NubertCommand::kSurround:
      command[0] = 0x94;
      command[1] = 0x01;
      command[2] = pendingNubertValue;
      label = "Surround";
      break;
    case NubertCommand::kNone:
      return false;
  }
  return writeNubert(command, sizeof(command), label);
}

void serviceNubertCommands() {
  if (homeKitPowerPending.active && homeKitPowerPending.writeSent &&
      static_cast<int32_t>(millis() - homeKitPowerPending.deadline) >= 0) {
    Serial.println("HomeKit power confirmation timed out; restoring confirmed receiver state.");
    reconcilePendingHomeKitPower();
  }
  if (pendingNubertCommand != NubertCommand::kNone) {
    const NubertCommand command = pendingNubertCommand;
    const uint8_t value = pendingNubertValue;
    const bool written = sendPendingNubertCommand();
    pendingNubertCommand = NubertCommand::kNone;
    if (command == NubertCommand::kVolume && homeKitVolumePending.active &&
        homeKitVolumePending.target == value) {
      if (written) {
        // A GATT write response only confirms transport delivery.  Keep the
        // HomeKit request pending until the receiver reports this value.
        homeKitVolumePending.writeSent = true;
        homeKitVolumePending.deadline = millis() + kVolumeConfirmationTimeoutMs;
      } else {
        reconcilePendingHomeKitVolume();
      }
    }
    if ((command == NubertCommand::kPowerOn || command == NubertCommand::kPowerStandby) &&
        homeKitPowerPending.active &&
        homeKitPowerPending.target ==
            (command == NubertCommand::kPowerOn ? NubertPowerState::kOn
                                                  : NubertPowerState::kStandby)) {
      if (written) {
        homeKitPowerPending.writeSent = true;
        homeKitPowerPending.deadline = millis() + kPowerOnTransitionMs;
      } else {
        reconcilePendingHomeKitPower();
      }
    }
    if (written) {
      nubertRefreshAt = millis() + kNubertRefreshDelayMs;
      commandLedUntil = millis() + kCommandLedFlashMs;
    }
  }
  if (nubertRefreshAt != 0 && static_cast<int32_t>(millis() - nubertRefreshAt) >= 0) {
    nubertRefreshAt = 0;
    requestNubertQueries(kMainStatusQuery | kSoundStatusQuery);
  }
  if (pendingNubertQueries & kMainStatusQuery) {
    pendingNubertQueries &= ~kMainStatusQuery;
    sendStatusQuery();
  } else if (pendingNubertQueries & kSoundStatusQuery) {
    pendingNubertQueries &= ~kSoundStatusQuery;
    sendSoundStatusQuery();
  }
  if (homeKitVolumePending.active && homeKitVolumePending.writeSent &&
      static_cast<int32_t>(millis() - homeKitVolumePending.deadline) >= 0) {
    Serial.println("HomeKit volume confirmation timed out; restoring confirmed device value.");
    reconcilePendingHomeKitVolume();
  }
}

void startBleConnection() {
  if (config.bleAddress[0] == '\0' || bleConnecting ||
      bleScanRunning || diagnosticScanPending ||
      (bleClient != nullptr && bleClient->isConnected())) {
    return;
  }
  const NimBLEAddress address(std::string(config.bleAddress), config.bleAddressType);
  if (!initialBleAttempted) {
    initialBleAttempted = true;
  }
  strncpy(lastBleTargetAddress, config.bleAddress, sizeof(lastBleTargetAddress) - 1);
  lastBleTargetAddress[sizeof(lastBleTargetAddress) - 1] = '\0';
  lastBleTargetAddressType = config.bleAddressType;
  ++bleConnectionAttemptCount;
  lastBleConnectionAttemptAt = millis();
  recordBleDiagnosticEvent(BleDiagnosticEvent::kConnectRequested);
  if (homeBertDiagnosticLevel >= 2) {
    Serial.printf("BLE diagnostic: attempt=%lu target=%s type=%s (%u).\n",
                  static_cast<unsigned long>(bleConnectionAttemptCount),
                  lastBleTargetAddress, bleAddressTypeName(lastBleTargetAddressType),
                  static_cast<unsigned>(lastBleTargetAddressType));
  }
  if (bleClient == nullptr) {
    bleClient = NimBLEDevice::createClient(address);
    if (bleClient == nullptr) {
      Serial.println("BLE client creation failed.");
      recordBleDiagnosticEvent(BleDiagnosticEvent::kConnectFailed);
      bleReconnectAt = millis() + kBleReconnectMs;
      return;
    }
    bleClient->setClientCallbacks(&bleClientCallbacks, false);
  }
  bleConnecting = bleClient->connect(address, true, true, false);
  if (bleConnecting) {
    Serial.println("BLE connection requested.");
  } else {
    Serial.println("BLE connection request failed.");
    recordBleDiagnosticEvent(BleDiagnosticEvent::kConnectFailed);
    bleReconnectAt = millis() + kBleReconnectMs;
  }
}

void serviceBleConnection() {
  if (bleConnectedEvent) {
    bleConnectedEvent = false;
    bleConnecting = false;
    nubertState.bleConnected = true;
    connectedLedPending = true;
    connectedLedUntil = 0;
    Serial.println("BLE connected.");
    recordBleDiagnosticEvent(BleDiagnosticEvent::kConnected);
    reportMemory("ble-connected");
    if (!bleGattLogged) {
      logGattDatabase();
    }
  }
  if (bleConnectFailedEvent || bleDisconnectedEvent) {
    const bool failed = bleConnectFailedEvent;
    bleConnectFailedEvent = false;
    bleDisconnectedEvent = false;
    bleConnecting = false;
    bleGattLogged = false;
    nubertCharacteristic = nullptr;
    connectedLedPending = false;
    connectedLedUntil = 0;
    clearNubertState();
    Serial.printf("BLE %s; reason=%d.\n", failed ? "connection failed" : "disconnected",
                  bleConnectionReason);
    recordBleDiagnosticEvent(failed ? BleDiagnosticEvent::kConnectFailed :
                              BleDiagnosticEvent::kDisconnected, true, bleConnectionReason);
    bleReconnectAt = millis() + kBleReconnectMs;
  }
  if (diagnosticScanPending && !bleConnecting &&
      !nubertState.bleConnected) {
    startDiagnosticBleScan();
  }
  if (homeKitStarted && !setupPortalRunning &&
      (bleClient == nullptr || !bleClient->isConnected()) && !bleConnecting &&
      !diagnosticScanIsActive() && !bleScanRunning &&
      static_cast<int32_t>(millis() - bleReconnectAt) >= 0) {
    startBleConnection();
  }
}

void stopSetupPortal() {
  if (!setupPortalRunning) return;
  if (setupWiFiScanStarted) WiFi.scanDelete();
  setupWiFiScanRunning = false;
  setupServer.stop();
  setupPortalRunning = false;
}

void beginStationConnection() {
  if (bleScanRunning) bleScan->stop();
  stopSetupPortal();
  const bool modeSet = WiFi.mode(WIFI_STA);
  const wl_status_t beginStatus = WiFi.begin(config.ssid, config.password);
  stationConnectionPending = !config.wifiVerified;
  stationConnectStartedAt = millis();
  Serial.printf("Wi-Fi state: STA mode=%u mode_set=%u begin=%d ssid_len=%u"
                " password_len=%u.\n",
                static_cast<unsigned>(WiFi.getMode()),
                static_cast<unsigned>(modeSet), static_cast<int>(beginStatus),
                static_cast<unsigned>(strlen(config.ssid)),
                static_cast<unsigned>(strlen(config.password)));
}

void appendHtmlEscaped(String &page, const std::string &value) {
  for (size_t i = 0; i < value.size(); ++i) {
    switch (value[i]) {
      case '&': page += F("&amp;"); break;
      case '<': page += F("&lt;"); break;
      case '>': page += F("&gt;"); break;
      case '"': page += F("&quot;"); break;
      default: page += value[i]; break;
    }
  }
}

bool parseBleIndex(const String &value, uint8_t &index) {
  if (value.isEmpty()) return false;
  uint16_t parsed = 0;
  for (size_t i = 0; i < value.length(); ++i) {
    if (value[i] < '0' || value[i] > '9') return false;
    parsed = parsed * 10 + static_cast<uint8_t>(value[i] - '0');
    if (parsed >= setupBleCandidateCount) return false;
  }
  index = static_cast<uint8_t>(parsed);
  return true;
}

const SetupBleCandidate *selectedBleDevice(const String &value) {
  uint8_t index = 0;
  if (!bleScanReady || !parseBleIndex(value, index)) return nullptr;
  return &setupBleCandidates[index];
}

bool parseWiFiIndex(const String &value, uint8_t &index) {
  if (value.isEmpty()) return false;
  uint16_t parsed = 0;
  for (size_t i = 0; i < value.length(); ++i) {
    if (value[i] < '0' || value[i] > '9') return false;
    parsed = parsed * 10 + static_cast<uint8_t>(value[i] - '0');
    if (parsed >= setupWiFiNetworkCount) return false;
  }
  index = static_cast<uint8_t>(parsed);
  return true;
}

bool selectedSetupWiFiSsid(String &ssid) {
  ssid = setupServer.arg("ssid");
  ssid.trim();
  if (!ssid.isEmpty()) return ssid.length() < sizeof(config.ssid);
  uint8_t index = 0;
  if (!setupServer.hasArg("network") ||
      !parseWiFiIndex(setupServer.arg("network"), index)) return false;
  ssid = setupWiFiNetworks[index].ssid;
  return true;
}

void appendSetupCandidateName(String &page, const SetupBleCandidate &device) {
  if (device.name[0] == '\0') {
    page += F("Nubert device (name unavailable)");
  } else {
    appendHtmlEscaped(page, device.name);
  }
}

void appendSetupPageStart(String &page, uint8_t stage) {
  page += F("<!doctype html><html><head><meta name=viewport content=\"width=device-width,initial-scale=1\"><title>homeBert setup</title><style>"
            "*{box-sizing:border-box}body{margin:0;background:#f4f6f8;color:#17212b;font:16px Arial,sans-serif;line-height:1.45}.wrap{max-width:560px;margin:auto;padding:20px 14px}.card{background:#fff;border:1px solid #dfe5ea;border-radius:16px;padding:22px;box-shadow:0 3px 12px #1020300d}.brand{display:flex;align-items:center;gap:11px;margin-bottom:22px}.logo{width:38px;height:38px;flex:none}.brand b{display:block;font-size:21px;letter-spacing:-.4px}.brand small{color:#607080}h1{font-size:23px;line-height:1.2;margin:0 0 12px}h2{font-size:18px;margin:0 0 12px}.intro{margin:0 0 18px;color:#41515d}.tasks{margin:18px 0;padding:0;list-style:none}.tasks li{margin:9px 0;display:flex;align-items:center;gap:9px}.task-icon{display:grid;place-items:center;width:21px;height:21px;border:1px solid #cfd8df;border-radius:50%;color:#607080;font-size:12px;font-weight:bold}.notice{margin:0 0 16px;padding:10px 12px;border-left:3px solid #7fb4b8;border-radius:8px;background:#f4fbfb;color:#34525a;font-size:14px}.device{display:block;margin:8px 0;padding:13px;border:1px solid #dce3e8;border-radius:11px;cursor:pointer}.device:has(input:checked){border-color:#147d86;background:#f2fbfb;box-shadow:0 0 0 1px #147d86}.device input{margin:0 9px 0 1px;accent-color:#147d86}.device-name{font-weight:bold}.meta{display:block;margin:4px 0 0 25px;color:#687786;font-size:13px;overflow-wrap:anywhere}.networks{max-height:188px;overflow:auto;padding-right:3px;-webkit-overflow-scrolling:touch}.networks .device{margin:4px 0;padding:8px 10px}.scroll-hint{margin:5px 0;color:#607080;font-size:12px}.field{display:block;margin:14px 0}.field span{display:block;margin-bottom:5px;font-weight:bold}.field input{width:100%;padding:12px;border:1px solid #cfd8df;border-radius:9px;font:inherit}.actions{display:flex;gap:10px;align-items:center;margin-top:18px;flex-wrap:wrap}.nav{display:flex;justify-content:space-between;align-items:center;gap:10px;margin-top:22px}.nav-right{display:flex;justify-content:flex-end;gap:10px;align-items:center;flex-wrap:wrap}.form-nav{margin-top:24px;padding-top:18px;border-top:1px solid #dfe5ea}button.btn,a.btn{-webkit-appearance:none;appearance:none;display:inline-flex;align-items:center;justify-content:center;border:1px solid #d5dde3;border-radius:9px;padding:12px 16px;background:#fff;color:#27414b;font:inherit;font-weight:bold;line-height:1.2;cursor:pointer;min-height:44px;text-decoration:none}button.btn.primary,a.btn.primary{border-color:#178491;background:#178491;color:#fff;box-shadow:0 1px 2px #0d596033}button.btn.secondary,a.btn.secondary{background:#eef2f5;color:#27414b}button.btn.tertiary,a.btn.tertiary{background:transparent;color:#52636e;border-color:transparent;text-decoration:underline;padding-left:4px;padding-right:4px}.password-tools{display:flex;justify-content:flex-end;margin-top:-8px}.password-tools .btn{min-height:44px}.form-options{margin-top:22px}.form-options .btn{padding-left:4px}.btn[disabled]{background:#eef2f5;color:#82909a;border-color:#d5dde3;box-shadow:none;cursor:default}.hint,.empty{color:#607080;font-size:14px}.selected{padding:13px;border:1px solid #b8dadd;background:#f2fbfb;border-radius:10px;margin:14px 0}.selected strong{display:block;font-size:17px;color:#173f45}.scan{display:flex;gap:12px;align-items:center;padding:14px;border:1px solid #d8e7e9;border-radius:11px;background:#f7fbfb}.scan-icon{width:30px;height:30px;flex:none;color:#147d86}.bluetooth-icon{width:32px;height:32px;overflow:visible}.pulse{height:4px;margin-top:6px;overflow:hidden;border-radius:4px;background:#dbecef}.pulse:before{content:'';display:block;width:42%;height:100%;border-radius:4px;background:#147d86;animation:scan 1.1s ease-in-out infinite}@keyframes scan{from{transform:translateX(-110%)}to{transform:translateX(260%)}}.homekit-complete,.homekit-pairing{text-align:center}.success-icon{display:grid;place-items:center;width:38px;height:38px;margin:4px auto 14px;border-radius:50%;background:#147d86;color:#fff;font-weight:bold;font-size:22px}.homekit-pairing{margin-top:24px;padding-top:22px;border-top:1px solid #dfe5ea}.homekit-pairing .btn{margin:4px auto 14px}.homekit-qr{display:block;margin:12px auto;background:#fff}.setup-code{display:block;user-select:all;font:700 24px ui-monospace,monospace;font-variant-numeric:tabular-nums;letter-spacing:1px;margin:7px 0}.close-message{margin:24px 0 0;padding-top:18px;border-top:1px solid #dfe5ea;text-align:center;font-size:18px;font-weight:600;color:#173f45}.progress{margin-top:20px;color:#71808a;font-size:11px;text-align:center}.progress-row{display:flex;align-items:flex-start}.progress .milestone{flex:1}.progress .icon{display:grid;place-items:center;width:27px;height:27px;margin:0 auto 5px;border:1px solid #d2dbe1;border-radius:50%;background:#fff;color:#71808a}.progress .done .icon,.progress .active .icon{border-color:#147d86;background:#147d86;color:#fff}.progress .active{color:#136f77;font-weight:bold}.progress-track{display:flex;gap:2px;margin:7px 12.5% 0}.progress-track i{display:block;flex:1;height:2px;background:#dbe2e8}.progress-track i.done{background:#147d86}@media(max-width:380px){.card{padding:17px}.wrap{padding:12px}.progress{font-size:10px}.nav{align-items:flex-start}.nav-right{justify-content:flex-end}}</style></head><body><main class=wrap><section class=card><header class=brand><svg class=logo viewBox=\"0 0 40 40\" aria-hidden=true><rect x=\"3\" y=\"3\" width=\"34\" height=\"34\" rx=\"9\" fill=\"#e9f4f5\" stroke=\"#147d86\"/><rect x=\"8\" y=\"10\" width=\"24\" height=\"17\" rx=\"4\" fill=\"#17212b\"/><path d=\"M13 20q2-5 4 0m6 0q2-5 4 0M16 24q4 3 8 0\" fill=\"none\" stroke=\"#ffd35a\" stroke-width=\"1.8\" stroke-linecap=\"round\"/></svg><div><b>homeBert</b><small>Nubert &middot; HomeKit Bridge</small></div></header>");
}

void appendSetupPageEnd(String &page, uint8_t stage) {
  page += F("<footer class=progress aria-label=\"Setup progress\"><div class=progress-row><span class=\"milestone");
  if (stage > 0) page += F(" done");
  else page += F(" active");
  page += F("\"");
  if (stage == 0) page += F(" aria-current=step");
  page += F("><span class=icon>S</span>Start</span><span class=\"milestone");
  if (stage > 1) page += F(" done");
  else if (stage == 1) page += F(" active");
  page += F("\"");
  if (stage == 1) page += F(" aria-current=step");
  page += F("><span class=icon>B</span>Bluetooth</span><span class=\"milestone");
  if (stage > 2) page += F(" done");
  else if (stage == 2) page += F(" active");
  page += F("\"");
  if (stage == 2) page += F(" aria-current=step");
  page += F("><span class=icon>W</span>Wi-Fi</span><span class=\"milestone");
  if (stage == 3) page += F(" done active");
  page += F("\"");
  if (stage == 3) page += F(" aria-current=step");
  page += F("><span class=icon>✓</span>Finish</span></div><div class=progress-track aria-hidden=true><i");
  if (stage >= 1) page += F(" class=done");
  page += F("></i><i");
  if (stage >= 2) page += F(" class=done");
  page += F("></i><i");
  if (stage >= 3) page += F(" class=done");
  page += F("></i></div></footer></section></main></body></html>");
}

void appendSetupBleNotice(String &page) {
  page += F("<aside class=notice><strong>Important:</strong> Close the Nubert app completely before scanning. If the app still holds the Bluetooth connection, your device may not appear.</aside>");
}

void handleSetupRoot() {
  String page;
  page.reserve(3072);
  appendSetupPageStart(page, 0);
  page += F("<h1>Welcome to homeBert</h1><p class=intro>Connect your Nubert device and Wi-Fi to finish setup.</p><ul class=tasks><li><span class=task-icon>1</span>Find your Nubert device.</li><li><span class=task-icon>2</span>Connect homeBert to Wi-Fi.</li></ul><nav class=nav aria-label=\"Wizard navigation\"><span></span><a class=\"btn primary\" href=/ble>Start setup</a></nav>");
  appendSetupPageEnd(page, 0);
  setupServer.send(200, kSetupHtmlContentType, page);
}

void handleSetupBle() {
  if (!bleScanReady && !bleScanRunning && !bleScanFailed) startBleScan();
  if (bleScanRunning) {
    String page;
    page.reserve(3072);
    appendSetupPageStart(page, 1);
    page += F("<h1>Searching for Nubert devices</h1><p class=intro>Please wait while homeBert scans for nearby Nubert devices.</p><div class=scan><svg class=\"scan-icon bluetooth-icon\" viewBox=\"-1 -1 26 26\" aria-hidden=true><path d=\"M12 2v20l6-6-6-6 6-6-6-6zm0 10L6 6m0 12 6-6\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/></svg><div><strong>Scanning for Nubert devices...</strong><div class=pulse></div><small>This may take a few seconds.</small></div></div>");
    appendSetupBleNotice(page);
    page += F("<nav class=nav aria-label=\"Wizard navigation\"><a class=btn tertiary href=/>Back</a><span></span></nav><meta http-equiv=refresh content=2>");
    appendSetupPageEnd(page, 1);
    setupServer.send(200, kSetupHtmlContentType, page);
    return;
  }
  if (bleScanFailed) {
    String page;
    page.reserve(3072);
    appendSetupPageStart(page, 1);
    page += F("<h1>Select your Nubert device</h1><p class=empty>BLE scan could not start. Please try again.</p><form action=/rescan method=get><nav class=nav aria-label=\"Wizard navigation\"><a class=btn tertiary href=/>Back</a><button class=\"btn primary\" type=submit>Scan again</button></nav></form>");
    appendSetupPageEnd(page, 1);
    setupServer.send(503, kSetupHtmlContentType, page);
    return;
  }

  validateSetupBleSelection();
  String page;
  page.reserve(4096);
  appendSetupPageStart(page, 1);
  page += F("<h1>Select your Nubert device</h1>");
  appendSetupBleNotice(page);
  page += F("<form method=post action=/select>");
  for (uint8_t i = 0; i < setupBleCandidateCount; ++i) {
    const SetupBleCandidate &device = setupBleCandidates[i];
    page += F("<label class=device><input type=radio name=ble value=");
    page += i;
    if (setupBleSelectionMatches(device) || (!setupBleSelectionValid && i == 0)) {
      page += F(" checked");
    }
    page += F("><span class=device-name>");
    appendSetupCandidateName(page, device);
    page += F("</span><span class=meta>RSSI ");
    page += device.rssi;
    page += F(" &middot; ");
    page += device.address;
    page += F("</span></label>");
  }
  if (setupBleCandidatesTruncated) {
    page += F("<p>More than eight matching Nubert devices were found; showing the first eight discovered.</p>");
  }
  if (setupBleCandidateCount == 0) {
    page += F("<p class=empty>No Nubert devices found. Make sure your device is powered on and the Nubert app is completely closed, then scan again.</p><nav class=nav aria-label=\"Wizard navigation\"><a class=btn tertiary href=/>Back</a><span class=nav-right><a class=btn secondary href=/rescan>Scan again</a><button class=\"btn primary\" type=submit disabled>Continue to Wi-Fi</button></span></nav>");
  } else {
    page += F("<nav class=nav aria-label=\"Wizard navigation\"><a class=btn tertiary href=/>Back</a><span class=nav-right><a class=btn secondary href=/rescan>Scan again</a><button class=\"btn primary\" type=submit>Continue to Wi-Fi</button></span></nav>");
  }
  if (setupBleCandidateCount == 0) {
    page += F("</form>");
  } else {
    page += F("</form>");
  }
  appendSetupPageEnd(page, 1);
  setupServer.send(200, kSetupHtmlContentType, page);
}

void handleSetupSelection() {
  const SetupBleCandidate *device =
      setupServer.hasArg("ble") ? selectedBleDevice(setupServer.arg("ble")) : nullptr;
  if (device == nullptr) {
    setupServer.sendHeader("Location", "/ble");
    setupServer.send(303, "text/plain", "");
    return;
  }
  selectSetupBleCandidate(*device);
  setupServer.sendHeader("Location", "/wifi");
  setupServer.send(303, "text/plain", "");
}

void handleSetupWiFi() {
  validateSetupBleSelection();
  if (!setupBleSelectionValid) {
    setupServer.sendHeader("Location", "/ble");
    setupServer.send(303, "text/plain", "");
    return;
  }
  if (!setupWiFiScanStarted && !setupWiFiScanRunning) startSetupWiFiScan();
  String page;
  page.reserve(4096);
  appendSetupPageStart(page, 2);
  page += F("<h1>Connect to Wi-Fi</h1><p class=selected>Selected Nubert device<strong>");
  appendSetupCandidateName(page, setupSelectedBleCandidate);
  page += F("</strong><br><span class=hint>");
  page += setupSelectedBleCandidate.address;
  page += F("</span></p>");
  if (setupWiFiScanRunning) {
    page += F("<h1>Searching for Wi-Fi networks</h1><p class=intro>Looking for nearby 2.4 GHz networks.</p><div class=scan><svg class=scan-icon viewBox=\"0 0 24 24\" aria-hidden=true><path d=\"M3 9a13 13 0 0 1 18 0M6 12a9 9 0 0 1 12 0m-9 3a5 5 0 0 1 6 0m-3 4h.01\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\" stroke-linecap=\"round\"/></svg><div><strong>Scanning for Wi-Fi networks...</strong><div class=pulse></div></div></div><nav class=nav aria-label=\"Wizard navigation\"><a class=btn tertiary href=/ble>Back</a><span></span></nav><meta http-equiv=refresh content=2>");
    appendSetupPageEnd(page, 2);
    setupServer.send(200, kSetupHtmlContentType, page);
    return;
  } else {
    if (setupWiFiScanFailed) {
      page += F("<p class=empty>Wi-Fi scan could not complete. You can still enter a network manually.</p>");
    } else if (setupWiFiNetworkCount == 0) {
      page += F("<p class=empty>No Wi-Fi networks found. You can still enter a network manually.</p>");
    }
    page += F("<form action=/wifi-rescan method=get class=actions><button class=btn secondary type=submit>Rescan networks</button></form><h2>Available networks</h2>");
  }
  page += F("<form method=post action=/save>");
  if (setupWiFiNetworkCount > 0) {
    page += F("<div id=network-list class=networks>");
    for (uint8_t i = 0; i < setupWiFiNetworkCount; ++i) {
      const SetupWiFiNetwork &network = setupWiFiNetworks[i];
      page += F("<label class=device><input type=radio name=network value=");
      page += i;
      page += F(" onchange=\"pick(this)\"><span class=device-name>");
      appendHtmlEscaped(page, network.ssid);
      page += F("</span><span class=meta>RSSI ");
      page += network.rssi;
      page += F(" dBm</span></label>");
    }
    page += F("</div>");
    if (setupWiFiNetworkCount > 4) {
      page += F("<p class=scroll-hint>Scroll for more</p>");
    }
  } else {
    page += F("<div id=network-list></div>");
  }
  page += F("<input id=other type=radio name=network value=other hidden><div id=details hidden aria-live=polite><p id=choice class=selected></p><div id=manual hidden><label class=field><span>SSID</span><input id=ssid name=ssid maxlength=32 autocapitalize=none autocomplete=off disabled></label></div><label class=field><span>Password</span><input id=password name=password type=password maxlength=64 autocomplete=current-password></label><div class=password-tools><button id=toggle class=\"btn tertiary\" type=button onclick=\"var p=d('password');p.type=p.type=='password'?'text':'password';this.textContent=p.type=='password'?'Hide password':'Show password'\">Show password</button></div></div><div class=form-options><button class=\"btn tertiary\" type=button onclick=\"manual()\">Other / Hidden Network</button></div><nav class=\"nav form-nav\" aria-label=\"Wizard navigation\"><a class=btn tertiary href=/ble>Back</a><button id=connect class=\"btn primary\" type=submit disabled>Next</button></nav></form><script>function d(i){return document.getElementById(i)}function select(){d('details').hidden=0;d('connect').disabled=0;var p=d('password');p.value='';p.type='password';d('toggle').textContent='Show password'}function pick(e){select();d('manual').hidden=1;d('ssid').disabled=1;d('ssid').value='';d('choice').innerHTML='Selected network<strong>'+e.parentNode.querySelector('.device-name').textContent+'</strong>'}function manual(){select();d('other').checked=1;d('manual').hidden=0;d('ssid').disabled=0;d('choice').innerHTML='Selected network<strong>Other / Hidden Network</strong>';d('ssid').focus()}</script>");
  appendSetupPageEnd(page, 2);
  setupServer.send(200, kSetupHtmlContentType, page);
}

void handleSetupSave() {
  String ssid;
  if (!setupServer.hasArg("password") || !setupBleSelectionValid ||
      !selectedSetupWiFiSsid(ssid) ||
      !saveConfig(ssid, setupServer.arg("password"),
                  &setupSelectedBleCandidate)) {
    setupServer.send(400, "text/plain",
                     "Select a discovered BLE device and enter valid Wi-Fi credentials.");
    return;
  }
  String page;
  page.reserve(8192);
  appendSetupPageStart(page, 3);
  page += F("<section class=homekit-complete><div class=success-icon aria-hidden=true>✓</div><h1>Setup complete</h1><p>homeBert is ready to add to Apple Home.</p></section><section class=homekit-pairing><p class=hint>HomeKit setup code</p>");
  char qrPayload[21]{};
  if (homeKitQrPayload(qrPayload)) {
    page += F("<span id=homekit-code class=setup-code>");
    page += homeKitSetupCode[0];
    page += homeKitSetupCode[1];
    page += homeKitSetupCode[2];
    page += F("-");
    page += homeKitSetupCode[3];
    page += homeKitSetupCode[4];
    page += F("-");
    page += homeKitSetupCode[5];
    page += homeKitSetupCode[6];
    page += homeKitSetupCode[7];
    page += F("</span><button class=\"btn secondary\" type=button onclick=\"copyCode(this)\">Copy code</button>");
    if (appendHomeKitQrSvg(page, qrPayload)) {
      page += F("<p class=hint>Open Apple Home and enter this code, or scan the QR code from another screen.</p>");
    } else {
      page += F("<p class=empty>HomeKit QR code could not be prepared. Enter the setup code manually in Apple Home.</p>");
    }
    page += F("<script>function copyCode(b){var c=document.getElementById('homekit-code'),r=document.createRange(),s=window.getSelection();r.selectNodeContents(c);s.removeAllRanges();s.addRange(r);if(navigator.clipboard&&window.isSecureContext){navigator.clipboard.writeText(c.textContent).then(function(){b.textContent='Code copied'})}else b.textContent='Code selected'}</script>");
  } else {
    page += F("<p class=empty>HomeKit setup information could not be prepared. Use the serial console to retrieve it.</p>");
  }
  page += F("</section><p class=close-message>This page can now be closed.</p>");
  appendSetupPageEnd(page, 3);
  setupServer.send(200, kSetupHtmlContentType, page);
  stationConnectRequestedAt = millis();
  stationConnectRequested = true;
}

void handleBleRescan() {
  if (!bleScanRunning) startBleScan();
  setupServer.sendHeader("Location", "/ble");
  setupServer.send(303, "text/plain", "");
}

void handleSetupWiFiRescan() {
  if (!setupWiFiScanRunning) startSetupWiFiScan();
  setupServer.sendHeader("Location", "/wifi");
  setupServer.send(303, "text/plain", "");
}

void handleSetupIcon() {
  setupServer.send_P(200, "image/webp", reinterpret_cast<PGM_P>(kHomeBertIconWebp),
                     kHomeBertIconWebpLength);
}

void startSetupPortal() {
  if (setupPortalRunning) return;
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(kSetupApName)) {
    Serial.println("Setup access point failed.");
    return;
  }
  setupServer.on("/", HTTP_GET, handleSetupRoot);
  setupServer.on("/ble", HTTP_GET, handleSetupBle);
  setupServer.on("/select", HTTP_POST, handleSetupSelection);
  setupServer.on("/wifi", HTTP_GET, handleSetupWiFi);
  setupServer.on("/wifi-rescan", HTTP_GET, handleSetupWiFiRescan);
  setupServer.on("/homebert-icon.webp", HTTP_GET, handleSetupIcon);
  setupServer.on("/save", HTTP_POST, handleSetupSave);
  setupServer.on("/rescan", HTTP_GET, handleBleRescan);
  setupServer.onNotFound(handleSetupRoot);
  setupServer.begin();
  setupPortalRunning = true;
  stationConnectionPending = false;
  bleScanReady = false;
  bleScanFailed = false;
  setupWiFiScanStarted = false;
  setupWiFiScanRunning = false;
  setupWiFiScanFailed = false;
  clearSetupWiFiNetworks();
  Serial.println("Wi-Fi state: setup AP running.");
  Serial.printf("Setup AP ready: %s at %s\n", kSetupApName,
                WiFi.softAPIP().toString().c_str());
}

void startWiFiOrSetup() {
  if (loadConfig()) {
    Serial.printf("Wi-Fi config loaded: ssid_len=%u password_len=%u verified=%u.\n",
                  static_cast<unsigned>(strlen(config.ssid)),
                  static_cast<unsigned>(strlen(config.password)),
                  static_cast<unsigned>(config.wifiVerified));
    beginStationConnection();
  } else {
    startSetupPortal();
  }
}

void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("Wi-Fi state: STA started.");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("Wi-Fi state: associated; waiting for IP.");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      if (wifiUnavailableSince == 0) wifiUnavailableSince = millis();
      Serial.printf("Wi-Fi state: STA disconnected; reason=%u.\n",
                    static_cast<unsigned>(info.wifi_sta_disconnected.reason));
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.printf("Wi-Fi state: got IP %s.\n",
                    WiFi.localIP().toString().c_str());
      reportMemory("wifi-connected");
      stationConnectionPending = false;
      wifiUnavailableSince = 0;
      if (!config.wifiVerified) {
        Preferences prefs;
        if (prefs.begin("homebert", false)) {
          prefs.putBool("wifi_verified", true);
          prefs.end();
          config.wifiVerified = true;
        }
      }
      startHomeKit();
      bleReconnectAt = millis();
      break;
    default:
      break;
  }
}

void serviceWiFi() {
  if (setupPortalRunning) {
    setupServer.handleClient();
    serviceSetupWiFiScan();
  }
  if (stationConnectRequested &&
      static_cast<uint32_t>(millis() - stationConnectRequestedAt) >=
          kSetupConfirmationDisplayMs) {
    stationConnectRequested = false;
    beginStationConnection();
  }
  const uint32_t now = millis();
  if (config.wifiVerified && !setupPortalRunning && WiFi.status() != WL_CONNECTED &&
      wifiUnavailableSince == 0) {
    wifiUnavailableSince = now;
  }
  if (stationConnectionPending &&
      now - stationConnectStartedAt >= kInitialWiFiTimeoutMs) {
    Serial.printf("Initial Wi-Fi validation failed; status=%d; returning to setup.\n",
                  static_cast<int>(WiFi.status()));
    initialWiFiValidationFailed = true;
    WiFi.mode(WIFI_OFF);
    startSetupPortal();
  }
  if (config.wifiVerified && !setupPortalRunning &&
      WiFi.status() != WL_CONNECTED &&
      now - lastReconnectAt >= kReconnectIntervalMs) {
    if (wifiUnavailableSince == 0) wifiUnavailableSince = now;
    lastReconnectAt = now;
    WiFi.reconnect();
  }
}

void showStatus() {
  statusUntil = millis() + kStatusDisplayMs;
}

void factoryReset() {
  displayedLed = LedColor::kOff;
  setLed(LedColor::kRed);
  delay(1000);  // Required factory-reset indication.
  Preferences prefs;
  if (prefs.begin("homebert", false)) { prefs.clear(); prefs.end(); }
  if (homeKitStarted) {
    homeSpan.processSerialCommand("H");  // Erases HomeSpan pairing data and reboots.
    return;
  }
  ESP.restart();
}

void requestBleReconnect() {
  Serial.println("BLE reconnect requested by button.");
  if (bleClient != nullptr && bleClient->isConnected()) {
    if (!bleClient->disconnect()) {
      Serial.println("BLE reconnect request could not disconnect the active client.");
    }
  } else if (!bleConnecting) {
    bleReconnectAt = millis();
  }
}

void serviceButton() {
  const bool pressed = digitalRead(kButtonPin) == LOW;
  const uint32_t now = millis();
  if (pressed != rawButtonPressed) {
    rawButtonPressed = pressed;
    lastButtonEdgeAt = now;
  }
  if (static_cast<uint32_t>(now - lastButtonEdgeAt) < kButtonDebounceMs) return;

  if (pressed && !wasPressed) {
    wasPressed = true;
    pressedAt = now;
    factoryResetTriggered = false;
  }
  if (wasPressed && !factoryResetTriggered &&
      static_cast<uint32_t>(now - pressedAt) >= kFactoryResetHoldMs) {
    factoryResetTriggered = true;
    factoryReset();
    return;
  }
  if (!pressed && wasPressed) {
    const uint32_t duration = now - pressedAt;
    wasPressed = false;
    if (factoryResetTriggered || duration >= kFactoryResetHoldMs) return;
    if (shortPressPending && now - lastReleaseAt <= kDoublePressWindowMs) {
      shortPressPending = false;
      lastReleaseAt = 0;
      requestBleReconnect();
      return;
    }
    lastReleaseAt = now;
    shortPressPending = true;
  }
  if (shortPressPending && now - lastReleaseAt > kDoublePressWindowMs) {
    shortPressPending = false;
    showStatus();
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.printf("homeBert %s commit %s.\n", kFirmwareVersion,
                kFirmwareGitCommit);
  printHomeKitSetupInfo();
  pinMode(kButtonPin, INPUT);
  rawButtonPressed = digitalRead(kButtonPin) == LOW;
  lastButtonEdgeAt = millis();
  startupLedUntil = millis() + kStartupLedMs;
  reportMemory("boot");
  WiFi.onEvent(onWiFiEvent);
  startNimble();
  startWiFiOrSetup();
  serviceLed();
}

void loop() {
  serviceButton();
  serviceWiFi();
  serviceBleScan();
  serviceBleConnection();
  serviceNubertCommands();
  publishNubertState();
  if (homeKitStarted) homeSpan.poll();
  serviceLed();
}
