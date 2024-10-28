/*
 * Copyright (c) 2023 Jean-Pierre sneyers
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
 */

#ifndef WEBSERVERMGR_H
#define WEBSERVERMGR_H
#define WWW_PORT_HTTP 80
#include <Arduino.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "GlobalVar.h"
#include "Language.h"
#include "TelnetMgr.h"
#include "MQTT.h"
#include "P1Reader.h"
#include "LogP1Mgr.h"

class HTTPMgr
{
public:
  explicit HTTPMgr(settings &currentConf, TelnetMgr &currentTelnet, MQTTMgr &currentMQTT, P1Reader &currentP1, LogP1Mgr &currentLogP1);
  void DoMe();
  void start_webservices();

private:
  settings &conf;
  TelnetMgr &TelnetSrv;
  MQTTMgr &MQTT;
  P1Reader &P1Captor;
  LogP1Mgr &LogP1;
  ESP8266WebServer server;
  char HTMLBufferContent[4000];
  Language Trad;
  bool ChekifAsAdmin();
  void TradAndSend(const char *content_type, char *content, const char *header, bool refresh);
  char* nettoyerInputText(const char* inputText, size_t maxLen);
  const char* GetAnimWait();
  void handleRoot();
  void handlePassword();
  void handleSetup();
  void handleRAW();
  void handleFactoryReset();
  void handleSetupSave();
  void handleUploadForm();
  void handleUploadFlash();
  void handleFavicon();
  void handleStyleCSS();
  void handleJSON();
  void handleJSONStatus();
  void handleP1Js();
  void handleMainJS();
  void handleReboot();
  void handleFile();

  void handleGraph24();
  void handleGraph24JS();

  void RebootPage(const char *Message);

  bool ActifCache(bool);
  
  void ReplyOTA(bool success, const char* error, u_int ref);

  bool UpdateResultFailed = false; // true = erreur d'update
  String UpdateMsg; // Message d'erreur de la mise à jour
  uint UpdateErrorCode = 0; //code d'erreur

  void handleP1();
  void handleHelp();
};
#endif