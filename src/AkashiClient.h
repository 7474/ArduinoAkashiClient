#ifndef _AKASHICLIENT_H_
#define _AKASHICLIENT_H_

// https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure
#include <WiFiClientSecure.h>
// https://arduinojson.org/
#include <ArduinoJson.h>
// https://github.com/janelia-arduino/Vector
#include <Vector.h>

#define AKASHICLIENT_VERSION "0.0.1"

enum AkashiStampType {
    AkashiStampTypeShukkin = 11,
    AkashiStampTypeTaikin = 12,
    AkashiStampTypeChokko = 21,
    AkashiStampTypeChokki = 22,
    AkashiStampTypeKyukeiIri = 31,
    AkashiStampTypeKyukeiModori = 32,
};

// Return value rule:
// - Pointer:
//     - NULL: Error
//     - other: Success
// - int:
//     - 0: Success
//     - other:
class AkashiClient
{
public:
    AkashiClient();
    ~AkashiClient();

    void setCompanyCode(const char* companyCode);
    void setToken(const char* token);

    // https://akashi.zendesk.com/hc/ja/articles/115000475854-AKASHI-%E5%85%AC%E9%96%8BAPI-%E4%BB%95%E6%A7%98#stamp
    int stamp(const AkashiStampType type);
    int updateToken(char* updatedToken, time_t& expiredAt);

private:
    char* apiHost;
    char token[128];
    WiFiClientSecure client;

    char companyCode[32];

    int get(const char* path, JsonDocument &response);
    int post(const char* path, JsonDocument &body, JsonDocument &response);
    int openRequest(const char* method, const char* path);
    int receiveResponse(JsonDocument &response);
};

#endif