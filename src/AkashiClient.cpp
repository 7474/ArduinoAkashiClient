#include "AkashiClient.h"

// https://github.com/bblanchon/ArduinoStreamUtils
#include <StreamUtils.h>
#include <time.h>

// Security Communication RootCA2
const char *akashi_root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdzCCAl+gAwIBAgIBADANBgkqhkiG9w0BAQsFADBdMQswCQYDVQQGEwJKUDEl\n"
    "MCMGA1UEChMcU0VDT00gVHJ1c3QgU3lzdGVtcyBDTy4sTFRELjEnMCUGA1UECxMe\n"
    "U2VjdXJpdHkgQ29tbXVuaWNhdGlvbiBSb290Q0EyMB4XDTA5MDUyOTA1MDAzOVoX\n"
    "DTI5MDUyOTA1MDAzOVowXTELMAkGA1UEBhMCSlAxJTAjBgNVBAoTHFNFQ09NIFRy\n"
    "dXN0IFN5c3RlbXMgQ08uLExURC4xJzAlBgNVBAsTHlNlY3VyaXR5IENvbW11bmlj\n"
    "YXRpb24gUm9vdENBMjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBANAV\n"
    "OVKxUrO6xVmCxF1SrjpDZYBLx/KWvNs2l9amZIyoXvDjChz335c9S672XewhtUGr\n"
    "zbl+dp+++T42NKA7wfYxEUV0kz1XgMX5iZnK5atq1LXaQZAQwdbWQonCv/Q4EpVM\n"
    "VAX3NuRFg3sUZdbcDE3R3n4MqzvEFb46VqZab3ZpUql6ucjrappdUtAtCms1FgkQ\n"
    "hNBqyjoGADdH5H5XTz+L62e4iKrFvlNVspHEfbmwhRkGeC7bYRr6hfVKkaHnFtWO\n"
    "ojnflLhwHyg/i/xAXmODPIMqGplrz95Zajv8bxbXH/1KEOtOghY6rCcMU/Gt1SSw\n"
    "awNQwS08Ft1ENCcadfsCAwEAAaNCMEAwHQYDVR0OBBYEFAqFqXdlBZh8QIH4D5cs\n"
    "OPEK7DzPMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8EBTADAQH/MA0GCSqGSIb3\n"
    "DQEBCwUAA4IBAQBMOqNErLlFsceTfsgLCkLfZOoc7llsCLqJX2rKSpWeeo8HxdpF\n"
    "coJxDjrSzG+ntKEju/Ykn8sX/oymzsLS28yN/HH8AynBbF0zX2S2ZTuJbxh2ePXc\n"
    "okgfGT+Ok+vx+hfuzU7jBBJV1uXk3fs+BXziHV7Gp7yXT2g69ekuCkO2r1dcYmh8\n"
    "t/2jioSgrGK+KwmHNPBqAbubKVY8/gA3zyNs8U6qtnRGEmyR7jTV7JqR50S+kDFy\n"
    "1UkC9gLl9B/rfNmWVan/7Ir5mUf/NVoCqgTLiluHcSmRvaS0eg29mvVXIwAHIRc/\n"
    "SjnRBUkLp7Y3gaVdjKozXoEofKd9J+sAro03\n"
    "-----END CERTIFICATE-----\n";

AkashiClient::AkashiClient()
{
        apiHost = "atnd.ak4.jp";
        // https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFiClientSecure/examples/WiFiClientSecure/WiFiClientSecure.ino
        client.setCACert(akashi_root_ca);
}

AkashiClient::~AkashiClient()
{
}

void AkashiClient::setCompanyCode(const char *companyCode)
{
        strcpy(this->companyCode, companyCode);
}

void AkashiClient::setToken(const char *token)
{
        strcpy(this->token, token);
}

// https://akashi.zendesk.com/hc/ja/articles/115000475854-AKASHI-%E5%85%AC%E9%96%8BAPI-%E4%BB%95%E6%A7%98#stamp
int AkashiClient::stamp(const AkashiStampType type)
{
        DynamicJsonDocument doc(128);
        doc["token"] = token;
        if (type != AkashiStampTypeAuto) {
                doc["type"] = type;
        }
        // TODO 他のパラメータ

        char path[128];
        sprintf(path, "/api/cooperation/%s/stamps", companyCode);
        DynamicJsonDocument response(1024 * 5);
        if (post(path, doc, response) != 0)
        {
                return -2;
        }

        return response["success"] ? 0 : -1;
}

// https://akashi.zendesk.com/hc/ja/articles/115000475854-AKASHI-%E5%85%AC%E9%96%8BAPI-%E4%BB%95%E6%A7%98#get_token
int AkashiClient::updateToken(char *updatedToken, time_t& expiredAt)
{
        DynamicJsonDocument doc(128);
        doc["token"] = token;

        char path[128];
        sprintf(path, "/api/cooperation/token/reissue//%s", companyCode);
        DynamicJsonDocument response(1024 * 5);
        if (post(path, doc, response) != 0)
        {
                return -2;
        }

        if (response["success"])
        {
                strcpy(updatedToken, response["response"]["token"]);
                struct tm expireAtTm;
                // (yyyy/mm/dd HH:MM:SS)
                strptime(response["response"]["expired_at"], "%Y/%m/%d %H%M%S", &expireAtTm);
                expiredAt = mktime(&expireAtTm);
                Serial.println(expiredAt);
                return 0;
        }
        else
        {
                return -1;
        }
}

// TODO この辺はJSONのRESTクライアントとして切り出せそう。
int AkashiClient::get(const char *path, JsonDocument &response)
{
        if (openRequest("GET", path) != 0)
        {
                return -1;
        }
        client.println("Connection: close");
        client.println();

        return receiveResponse(response);
}

struct NullWriter
{
        size_t write(uint8_t c)
        {
                return 1;
        }
        size_t write(const uint8_t *s, size_t n)
        {
                return n;
        }
};

int AkashiClient::post(const char *path, JsonDocument &body, JsonDocument &response)
{
        // TODO sizeの取り方。。。
        // NullWriter nullWriter;
        // size_t bodyLength = serializeJson(body, nullWriter);
        size_t bodyLength = serializeJson(body, Serial);

        if (openRequest("POST", path) != 0)
        {
                return -1;
        }
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(bodyLength);
        client.println("Connection: close");
        client.println();
        serializeJson(body, client);

        return receiveResponse(response);
}

int AkashiClient::openRequest(const char *method, const char *path)
{
        Serial.println("\nStarting connection to server...");
        Serial.print("Host: ");
        Serial.println(apiHost);
        if (!client.connect(apiHost, 443))
        {
                Serial.println("Connection failed!");
                return -1;
        }
        Serial.println("Connected to server!");
        client.print(method);
        client.print(" ");
        client.print(path);
        client.println(" HTTP/1.0");
        client.print("Host: ");
        client.println(apiHost);

        return 0;
}

int AkashiClient::receiveResponse(JsonDocument &response)
{
        while (client.connected())
        {
                String line = client.readStringUntil('\n');
                Serial.println(line);
                if (line == "\r")
                {
                        Serial.println("headers received");
                        break;
                }
        }
        ReadLoggingStream loggingStream(client, Serial);
        DeserializationError error = deserializeJson(response, loggingStream);
        client.stop();
        if (error)
        {
                Serial.println(error.c_str());
                return -1;
        }
        Serial.println("\nbody received");

        return 0;
}
