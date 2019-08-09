// *The MIT License (MIT)
// Copyright (c) 2018 by Mickey Poppitz
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// See more at https://github.com/mickeypop/Weather_Station_Remote

// TODOs
//    DONE, ISR, LedChangeState(), need to add on/off var status routines to control led and flash rate
//    DONE, WEB return and settings,
            // http://<remote IP>  Returns info
                  // <Remote ID><Temp Units><Temp><TempIndex><Humidity>
                  // ie;  Office,F,64.6,62.4,35.0
            // http://<remote IP>/set WEB Page, set defaults 
//    DONE, SPIFFS, all functions done


// built on ESP8266 D1 Mini  though most others should work
// once flashed and booted, go to http://<remote IP>/set to set remote name to show on main station and F or C units
// the IP, mDNS and ID name are cycled on the OLED every 10 seconds

// LED is Heartbeat flashes every LED_TICKER_TIME currently 10 sec  and .1 if WiFi is DOWN
