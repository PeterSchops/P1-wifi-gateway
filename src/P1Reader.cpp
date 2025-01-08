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

#include "P1Reader.h"

P1Reader::P1Reader(settings &currentConf) : conf(currentConf)
{
  Serial.setRxBufferSize(MAXLINELENGTH-2);
  //Serial.begin(SERIALSPEED);
  Serial.begin(SERIALSPEED, SERIAL_8N1, SERIAL_FULL, 1, true);
  datagram.reserve(1500);
}

void P1Reader::RTS_on() // switch on Data Request
{
  MainSendDebug("[P1] Data requested");
  Serial.flush(); //flush output buffer
  while(Serial.available() > 0 ) Serial.read(); //flush input buffer
  
  state = State::WAITING; // signal that we are waiting for a valid start char (aka /)
  digitalWrite(OE, LOW); // enable buffer
  digitalWrite(DR, HIGH); // turn on Data Request
  TimeOutRead = millis() + P1TIMEOUTREAD; //max read time
}

void P1Reader::RTS_off() // switch off Data Request
{
  state = State::DISABLED;
  nextUpdateTime = millis() + conf.interval * 1000;
  digitalWrite(DR, LOW); // turn off Data Request
  digitalWrite(OE, HIGH); // put buffer in Tristate mode
}

void P1Reader::ResetnextUpdateTime()
{
  RTS_off();
  nextUpdateTime = 0;
}

int P1Reader::FindCharInArray(const char array[], char c, int len)
{
  for (int i = 0; i < len; i++) {
    if (array[i] == c) {
      return i;
    }
  }
  return -1;
}

String P1Reader::identifyMeter(String Name)
{
  if (Name.indexOf("FLU5\\") != -1)            { return "Siconia"; } //Belgium
  if (Name.indexOf("ISK5\\2M550E-1011") != -1) { return "ISKRA AM550e-1011"; }
  if (Name.indexOf("KFM5KAIFA-METER") != -1)   { return "Kaifa  (MA105 of MA304)"; }
  if (Name.indexOf("XMX5LGBBFG10") != -1)      { return "Landis + Gyr E350"; }
  if (Name.indexOf("XMX5LG") != -1)            { return "Landis + Gyr"; }
  if (Name.indexOf("Ene5\\T210-D") != -1)      { return "Sagemcom T210-D"; }
  return "UNKNOW";
}

void P1Reader::decodeTelegram(int len)
{
  int startChar = FindCharInArray(telegram, '/', len);
  int endChar   = FindCharInArray(telegram, '!', len);

  if (state == State::WAITING) { // we're waiting for a valid start sequence, if this line is not it, just return
    if (startChar >= 0) {
      // start found. Reset CRC calculation
      MainSendDebug("[P1] Start of datagram found");
      
      digitalWrite(DR, LOW);  // turn off Data Request
      digitalWrite(OE, HIGH); // put buffer in Tristate mode
      
      // reset datagram
      datagram = "";
      dataEnd = false;
      state = State::READING;

      for (int cnt = startChar; cnt < (len - startChar); cnt++) {
        datagram += telegram[cnt];
      }

      if (meterName == "") {
        meterName = identifyMeter(telegram);
      }

      return;
    }
    else {
      return; // We're waiting for a valid start char, if we're in the middle of a datagram, just return.
    }
  }

  if (state == State::READING) {
    if (endChar >= 0) {
      // we have found the endchar !
      MainSendDebug("[P1] End found");
      dataEnd = true; // we're at the end of the data stream, so mark (for raw data output) We don't know if the data is valid, we will test this below.
     
      if (datagram.length() < 2048) {
        for (int cnt = 0; cnt < len; cnt++) {
          datagram += telegram[cnt];
        }
        datagram += "\r\n";
      }
      else {
        MainSendDebug("[P1] Buffer overflow ?");
        state = State::FAULT;
        return;
      }

      state = State::DONE;
      LastSample = millis();
      return;
    }
    else { 
      // no endchar, so normal line, process
      for (int cnt = 0; cnt < len; cnt++) {
        datagram += telegram[cnt];
      }
      OBISparser(len);
    }
    return;
  }
  return;
}

String P1Reader::readFirstParenthesisVal(int start, int end)
{
  String value = "";
  int i = start + 1;
  bool trailingZero = true;
  while ((telegram[i] != ')') && (i < end))
  {
    if (trailingZero && telegram[i] == '0') {
      i++;
    }
    else {
      value += (char)telegram[i];
      trailingZero = false;
      i++;
    }
  }
  return value;
}

String P1Reader::readUntilStar(int start, int end)
{
  String value = "";
  int i = start + 1;
  bool trailingZero = true;
  while ((telegram[i] != '*') && (i < end)) {
    if (trailingZero && telegram[i] != '0') {
      trailingZero = false;
    }
    if ((telegram[i] == '0') && (telegram[i + 1] == '.')) {
      // value += (char)telegram[i];
      trailingZero = false;
    }
    if (!trailingZero) {
      value += (char)telegram[i];
    }
    i++;
  }
  return value;
}

String P1Reader::readBetweenDoubleParenthesis(int start, int end)
{
  String value = "";
  int i = start + 1;
  while ((telegram[i] != ')') && (telegram[i + 1] != '(')) {
    i++; // we have found the intersection of the command and data
         // 0-1:24.2.1(231029141500W)(05446.465*m3)
  }
  i++;
  value = readUntilStar(i, end);
  return value;
}

void P1Reader::OBISparser(int len)
{
  int i;
  String value;
  String inString = "";
  int idx;

  for (i = 0; i < len; i++) {
    if (telegram[i] == '(') {
      break;
    }
    if (isDigit(telegram[i])) {
      // convert the incoming byte to a char and add it to the string:
      inString += (char)telegram[i];
    }
  }

  idx = inString.toInt();
  
  switch (idx)
  {
  case 0:
    break;
  case 9614:    // 0-0:96.1.4(50221)                                Version information
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1version, sizeof(DataReaded.P1version));
    break;
  case 1094321: // 1-0:94.32.1(400)                                 (230: 3x230 grid, 400: 3N400V grid)
    break;
  case 9611:    // 0-0:96.1.1(3153414733313031303231363035)         Equipment identifier electricity
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId, sizeof(DataReaded.equipmentId));
    break;
  case 9612:    // 0-0:96.1.2(353431343430303132333435363738393030) EAN code
    break;
  case 100:     // 0-0:1.0.0(200512135409S) DateTime YYMMDDhhmm
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.P1timestamp, sizeof(DataReaded.P1timestamp));
    break;
	
  case 10181: // 1-0:1.8.1(000992.992*kWh)                          Meter reading electricity consumption (Tariff 1, night) in 0.001kWh
    if (!conf.InverseHigh_1_2_Tarif) {
      DataReaded.electricityUsedTariff1 = FixedValue(readUntilStar(i, len));
    } else {
      DataReaded.electricityUsedTariff2 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10182: // 1-0:1.8.2(000560.157*kWh)                          Meter reading electricity consumption (Tariff 2, day) in 0.001kWh
    if (!conf.InverseHigh_1_2_Tarif){
      DataReaded.electricityUsedTariff2 = FixedValue(readUntilStar(i, len));
    } else {
      DataReaded.electricityUsedTariff1 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10281: // 1-0:2.8.1(000348.890*kWh)                          Meter reading electricity injection (Tariff 1, night) in 0.001kWh
    if (!conf.InverseHigh_1_2_Tarif) {
      DataReaded.electricityReturnedTariff1 = FixedValue(readUntilStar(i, len));
    } else {
      DataReaded.electricityReturnedTariff2 = FixedValue(readUntilStar(i, len));
    }
    break;
  case 10282: // 1-0:2.8.2(000859.885*kWh)                          Meter reading electricity injection (Tariff 2, day) in 0.001kWh
    if (!conf.InverseHigh_1_2_Tarif) {
      DataReaded.electricityReturnedTariff2 = FixedValue(readUntilStar(i, len));
    } else {
      DataReaded.electricityReturnedTariff1 = FixedValue(readUntilStar(i, len));
    }
    break;
		
  case 96140: // 0-0:96.0(0001)                                     Tariff indicator (electricity (1: High/normal, 2: low))
    DataReaded.tariffIndicatorElectricity = readFirstParenthesisVal(i, len).toInt();
    if (conf.InverseHigh_1_2_Tarif) {
      if (DataReaded.tariffIndicatorElectricity == 1) {
        DataReaded.tariffIndicatorElectricity = 2;
      } else {
        DataReaded.tariffIndicatorElectricity = 1;
      }
    }
    break;
  case 10140: // 1-0:1.4.0(02.351*kW)                               Current average demand active energy import in kW
    DataReaded.activeEnergyActual = FixedValue(readUntilStar(i, len));
    break;
  case 10160: // 1-0:1.6.0(200509134558S)(02.589*kW)                Maximum demand Active energy import of the current month in kW
    DataReaded.activeEnergyMaximumOfThisMonth = FixedValue(readUntilStar(i, len));
	  break;	
  case 9810:  // 0-0:98.1.0(3)(1-0:1.6.0)(1-0:1.6.0)(200501000000S)(200423192538S)(03.695*kW)(200401000000S)(200305122139S)(05.980*kW)(200301000000S)(200210035421W)(04.318*kW) Maximum demand history (last 13 months) in kW
    //TODO
    break;
  case 10170: // 1-0:1.7.0(00.000*kW)                               Actual electricity power consumption (+P) in 0.001kWh
    DataReaded.actualElectricityPowerDeli = FixedValue(readUntilStar(i, len));
    break;
  case 10270: // 1-0:2.7.0(00.000*kW)                               Actual electricity power injection (-P) in 0.001kWh
    DataReaded.actualElectricityPowerRet = FixedValue(readUntilStar(i, len));
    break;
  case 102170:  // 1-0:21.7.0(00.000*kW)                            Instantaneous active power L1(+P) in 0.001kWh
    DataReaded.activePowerL1P = FixedValue(readUntilStar(i, len));
    break;
  case 104170:  // 1-0:41.7.0(00.000*kW)                            Instantaneous active power L2(+P) in 0.001kWh
    DataReaded.activePowerL2P = FixedValue(readUntilStar(i, len));
    break;
  case 106170:  // 1-0:61.7.0(00.000*kW)                            Instantaneous active power L3(+P) in 0.001kWh
    DataReaded.activePowerL3P = FixedValue(readUntilStar(i, len));
    break;
  case 102270:  // 1-0:22.7.0(00.000*kW)                            Instantaneous active power L1(-P) in 0.001kWh
    DataReaded.activePowerL1NP = FixedValue(readUntilStar(i, len));
    break;
  case 104270:  // 1-0:42.7.0(00.000*kW)                            Instantaneous active power L2(-P) in 0.001kWh
    DataReaded.activePowerL2NP = FixedValue(readUntilStar(i, len));
    break;
  case 106270:  // 1-0:62.7.0(00.000*kW)                            Instantaneous active power L3(-P) in 0.001kWh
    DataReaded.activePowerL3NP = FixedValue(readUntilStar(i, len));
    break;
  case 103270:  // 1-0:32.7.0(232.0*V)                              Instantaneous voltage L1 in V
    DataReaded.instantaneousVoltageL1 = FixedValue(readUntilStar(i, len));
    break;
  case 105270:  // 1-0:52.7.0(232.0*V)                              Instantaneous voltage L2 in V
    DataReaded.instantaneousVoltageL2 = FixedValue(readUntilStar(i, len));
    break;
  case 107270:  // 1-0:72.7.0(232.0*V)                              Instantaneous voltage L3 in V
    DataReaded.instantaneousVoltageL3 = FixedValue(readUntilStar(i, len));
    break;
  case 103170:  // 1-0:31.7.0(000.00*A)                             Instantaneous current L1 in A
    DataReaded.instantaneousCurrentL1 = FixedValue(readUntilStar(i, len));
    break;
  case 105170:  // 1-0:51.7.0(000.00*A)                             Instantaneous current L2 in A
    DataReaded.instantaneousCurrentL2 = FixedValue(readUntilStar(i, len));
    break;
  case 107170:  // 1-0:71.7.0(000.00*A)                             Instantaneous current L3 in A
    DataReaded.instantaneousCurrentL3 = FixedValue(readUntilStar(i, len));
    break;
  case 96310:   // 0-0:96.3.10(1)                                   Breaker state (0: Disconnected, 1: Connected, 2: Ready_for_reconnection)	
	  break;
  case 1700:    // 0-0:17.0.0(99.999*kW)                            Limiter threshold in kW (0-99.998: threshold, 99.999: deactivated)
    break;	
  case 103140:  // 1-0:31.4.0(999.99*A)                             Fuse supervision threshold (L1) in A (0-99.998: threshold, 99.999: deactivated)
	  break;
  case 196310:  // 0-1:96.3.10(0)                                   Breaker state (0: Disconnected, 1: Connected, 2: Ready_for_reconnection)?	
	  break;
  case 296310:  // 0-2:96.3.10(0)                                   Breaker state (0: Disconnected, 1: Connected, 2: Ready_for_reconnection)?	
	  break;
  case 396310:  // 0-3:96.3.10(0)                                   Breaker state (0: Disconnected, 1: Connected, 2: Ready_for_reconnection)?	
	  break;
  case 496310:  // 0-4:96.3.10(0)                                   Breaker state (0: Disconnected, 1: Connected, 2: Ready_for_reconnection)?	
	  break;
  case 96130:   // 0-0:96.13.0()                                    Text message (max 1024 characters)	
	  break;
	
  case 1032320: // 1-0:32.32.0                                      Aantal korte spanningsdalingen Elektriciteit in fase 1
    DataReaded.numberVoltageSagsL1 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1052320: // 1-0:52.32.0                                      Aantal korte spanningsdalingen Elektriciteit in fase 2
   DataReaded.numberVoltageSagsL2 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1072320: // 1-0:72.32.0                                      Aantal korte spanningsdalingen Elektriciteit in fase 3
    DataReaded.numberVoltageSagsL3 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1032360: // 1-0:32.36.0(00000)                               Aantal korte spanningsstijgingen Elektriciteit in fase 1
    DataReaded.numberVoltageSwellsL1 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1052360: // 1-0:52.36.0(00000)                               Aantal korte spanningsstijgingen Elektriciteit in fase 2
    DataReaded.numberVoltageSwellsL2 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1072360: // 1-0:72.36.0(00000)                               Aantal korte spanningsstijgingen Elektriciteit in fase 3
    DataReaded.numberVoltageSwellsL3 = readFirstParenthesisVal(i, len).toInt();
    break;
  case 96721:   // 0-0:96.7.21(00051)  Number of power failures in any phase
    DataReaded.numberPowerFailuresAny = readFirstParenthesisVal(i, len).toInt();
    break;
  case 9679:    // 0-0:96.7.9(00007) Number of long power failures in any phase
    DataReaded.numberLongPowerFailuresAny = readFirstParenthesisVal(i, len).toInt();
    break;
  case 1099970: // 1-0:99.97.0(6) Power Failure Event Log (long power failures)
    DataReaded.longPowerFailuresLog = "";
    while (i < len)
    {
      DataReaded.longPowerFailuresLog += (char)telegram[i];
      i++;
    }
    break;


  case 12410:   // 0-1:24.1.0(003)                                  Device type, gas
    break;
  case 12420:   // 0-n:24.2.0                                       M-Bus device type, gas
    break;
  case 19610:   // 0-n:96.1.0                                       Equipment identifier gas
  case 19611:   // 0-n:96.1.0(37464C4F32313139303333373333)         
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId2, sizeof(DataReaded.equipmentId2));
    break;
  case 19612:   // 0-n:96.1.0(353431343430303132333435363738393030)         
    break;	
  case 12421:   // 0-n:24.2.1                                       Last 5-minute value (temperature converted) in m3, gas
  case 12423:   // 0-n:24.2.3(200512134558S)(00112.384*m3)          Last 5-minute value (not temperature converted) in m3, gas
    DataReaded.gasReceived5min = FixedValue(readBetweenDoubleParenthesis(i, len));
    break;
  case 12424: // 0-n:24.2.4 Breaker state, gas
	  break;

  case 22410:   // 0-2:24.1.0(007)                                  Device type, water
    break;
  case 22420:   // 0-n:24.2.0                                       M-Bus device type, water
    break;
  case 29610:   // 0-n:96.1.0                                       Equipment identifier water
  case 29611:   // 0-n:96.1.1(3853414731323334353637383930)         
    readFirstParenthesisVal(i, len).toCharArray(DataReaded.equipmentId3, sizeof(DataReaded.equipmentId3));
    break;
  case 29612:   // 0-n:96.1.0(353431343430303132333435363738393033)         
    break;	
  case 22421: // 0-n:24.2.1                                         Last 5-minute value in 0,001m3, water
  case 22423: // 0-n:24.2.3(200512134558S)(00872.234*m3)            Last 5-minute value (not temperature converted) in m3, water
    DataReaded.waterReceived5min = FixedValue(readBetweenDoubleParenthesis(i, len));
    break;
	
	
    
    
  case 96131:  //0-0:96.13.1                                        Consumer message code    
  case 13028:  //1-3:0.2.8                                          version information
  case 2410:   //0-n:24.1.0                                         Equipment identifier  
    //ignore line :-)
    break;
  default:
    MainSendDebugPrintf("[P1] Unrecognized line : %s", inString);
    break;
  }

  // clear the string for new input:
  inString = "";
}

unsigned long P1Reader::GetnextUpdateTime()
{
  return nextUpdateTime;
}

void P1Reader::DoMe()
{
  if ((millis() > nextUpdateTime) && (state == State::DISABLED)) {
    RTS_on();
  }

  if ((state == State::WAITING) || (state == State::READING)) {
    readTelegram();
  }
}

/// @brief Check if timeout for reading P1 data
/// @return true if on timeout
bool P1Reader::CheckTimeout()
{
  if (millis() > TimeOutRead) {
    MainSendDebug("[P1] Timeout");
    RTS_off();
    return true;
  }
  return false;
}

void P1Reader::readTelegram()
{
  if ((state != State::WAITING) && (state != State::READING)) {
    return;
  }

  if (CheckTimeout()) {
    return;
  }

  if (Serial.available()) {
    memset(telegram, 0, sizeof(telegram));
    while (Serial.available()) {
      if (CheckTimeout()) {
        return;
      }

      int len = Serial.readBytesUntil('\n', telegram, sizeof(telegram)-2);
      telegram[len] = '\n';
      telegram[len + 1] = 0;
      
      decodeTelegram(len + 1);

      if (state == State::DONE) {
        blink(1, 400);
        RTS_off();
        TriggerCallbacks();
      }
    }
  }
}
