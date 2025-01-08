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
*/
#ifndef LOGP1MGR_H
#define LOGP1MGR_H

#define FILENAME_LAST24H "/Last24H.json"

#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Debug.h"
#include "GlobalVar.h"
#include "P1Reader.h"

class LogP1Mgr
{
public:

  /// @brief Formatting and Starting LittleFS
  void format()
  {
    LittleFS.format();
    LittleFS.begin();
  }

  explicit LogP1Mgr(settings &currentConf, P1Reader &currentP1) : DataReaderP1(currentP1)
  {
    if (!LittleFS.begin())
    {
      format();
    }
    MainSendDebug("[STRG] Ready");

    //Ecoute de nouveau datagram
    DataReaderP1.OnNewDatagram([this]()
    {
      newDataGram();
    });

    //LittleFS.remove(FILENAME_LAST24H); // for debug
  }

  /// @brief Converting a hex string to uint8_t
  /// @param string Value to convert
  /// @return an unsigned numeric value
  static uint8_t hexStringToUint8(const char* chaine)
  {
    uint8_t resultat = 0;
    int i = 0;

    // The string is traversed until a non-numeric character or the end of the string is encountered.
    while (chaine[i] >= '0' && chaine[i] <= '9') {
      // We extract the number and multiply it by the corresponding power of 10
      resultat = resultat * 10 + (chaine[i] - '0');
      i++;
    }

    return resultat;
  }
private:
  P1Reader &DataReaderP1;
  bool FileInitied = false;
  uint8_t LastHourInLast24H = 0;
  struct DateTime
  {
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
  };


  /// @brief 
  /// @param FileName Allows you to load a JSON file
  /// @param doc The pointer to the element where the data will be loaded
  /// @return True if the file is loaded, otherwise False
  bool loadJSON(const char *FileName, JsonDocument &doc)
  {
    File file = LittleFS.open(FileName, "r");
    if(file) {
      DeserializationError error = deserializeJson(doc, file);
      file.close();
      
      if (!error) {
        return true;
      }
    }

    MainSendDebugPrintf("[STKG] Error on load %s", FileName);
    return false;
  }

  /// @brief Opens the JSON file and loads the time of the last measurement
  void prepareLogLast24H()
  {
    JsonDocument doc;
    char datetime[13];
    
    FileInitied = true;
    if (!loadJSON(FILENAME_LAST24H, doc)) {
      return;
    }

    JsonArray array = doc.as<JsonArray>();
    if (array.isNull()) {
      return;
    }

    JsonObject lastItem = array[array.size()-1];
    strcpy(datetime, lastItem["DateTime"]);
    char charhour[2] = { datetime[6], datetime[7] };
    LastHourInLast24H = hexStringToUint8(charhour);
  }


  /// @brief Processing a new measurement received
  void newDataGram()
  {
    char charhour[2] = { DataReaderP1.DataReaded.P1timestamp[6], DataReaderP1.DataReaded.P1timestamp[7] };
    uint8_t hour = hexStringToUint8(charhour);
    
    if (!FileInitied) {
      prepareLogLast24H();
    }

    if (LastHourInLast24H == hour) {
      return; // we're waiting for the next hour!
    }

    writeNewLineInLast24H();
  }

  /// @brief Processing the new point and checking that it does not have too many points in the JSON file
  void writeNewLineInLast24H()
  {
    MainSendDebug("[STKG] Write log for 24H");
    JsonDocument Points;

    loadJSON(FILENAME_LAST24H, Points);

    if (Points.size() > 24) {
      u_int8_t NbrToRemove = Points.size() - 24;

      JsonDocument newDoc;
      for (size_t i = NbrToRemove; i < Points.size(); i++) {
        newDoc.add(Points[i]);
      }

      addPointAndSave(newDoc);
      return;
    }

    addPointAndSave(Points);
  }

  /// @brief Add and save the measurement file
  /// @param Points Current measurement points
  void addPointAndSave(JsonDocument Points)
  {
    JsonObject point = Points.add<JsonObject>();
    point["DateTime"] = DataReaderP1.DataReaded.P1timestamp;
    point["T1"] = DataReaderP1.DataReaded.electricityUsedTariff1.val();
    point["T2"] = DataReaderP1.DataReaded.electricityUsedTariff2.val();
    point["R1"] = DataReaderP1.DataReaded.electricityReturnedTariff1.val();
    point["R2"] = DataReaderP1.DataReaded.electricityReturnedTariff2.val();

    File file = LittleFS.open(FILENAME_LAST24H, "w");
    serializeJson(Points, file);
    file.close();
    
    //save last hour
    char charhour[2] = { DataReaderP1.DataReaded.P1timestamp[6], DataReaderP1.DataReaded.P1timestamp[7] };
    LastHourInLast24H = hexStringToUint8(charhour);
  }
};
#endif