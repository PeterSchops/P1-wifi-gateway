/*
 * Copyright (c) 2025 Jean-Pierre Sneyers
 * Source : https://github.com/narfight/P1-wifi-gateway
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Additionally, please note that the original source code of this file
 * may contain portions of code derived from (or inspired by)
 * previous works by:
 *
 * Ronald Leenes (https://github.com/romix123/P1-wifi-gateway and http://esp8266thingies.nl)
 * MQTT part based on https://github.com/daniel-jong/esp8266_p1meter/blob/master/esp8266_p1meter/esp8266_p1meter.ino
 */

#ifndef MQTT_H
#define MQTT_H

#define MAXERROR 10
#define RETRYTIME 10000

#include <Arduino.h>
#include "GlobalVar.h"
#include <WiFiClient.h>
#include <AsyncMqttClient.h>
#include "Debug.h"
#include "P1Reader.h"
#include "WifiMgr.h"

class MQTTMgr
{
private:
  unsigned long LastReportinMillis = 0;
  AsyncMqttClient mqtt_client; // * Initiate MQTT client
  settings &conf;
  WifiMgr &WifiClient;
  P1Reader &DataReaderP1;
  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  /// @brief Send a message to a broker topic
  /// @param topic
  /// @param payload
  void send_msg(const char *topic, const char *payload);
  char* uint32ToChar(uint32_t value, char* buffer);
  enum {
    CONNECTING,
    CONNECTED,
    DISCONNECTING,
    DISCONNECTED
  } _state = DISCONNECTED;
  u_int8_t CountError;
public:
  long unsigned nextMQTTreconnectAttempt = millis();

  explicit MQTTMgr(settings &currentConf, WifiMgr &Link, P1Reader &currentP1);
  void stop();
  bool mqtt_connect();
  bool IsConnected();

  void send_float(String name, float metric);
  void send_char(String name, const char *metric);
  void send_uint32_t(String name, uint32_t metric);
  void MQTT_reporter();
  void SendDebug(String payload);
};
#endif