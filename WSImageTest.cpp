// Copyright (c) 2013-2017, Matt Godbolt
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// An extraordinarily simple test which presents a web page with some buttons.
// Clicking on the numbered button increments the number, which is visible to
// other connected clients.  WebSockets are used to do this: by the rather
// suspicious means of sending raw JavaScript commands to be executed on other
// clients.

#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"
#include "seasocks/util/Json.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core.hpp"

#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <string>

using namespace seasocks;
using namespace std;
using namespace cv;

class CameraHandler: public WebSocket::Handler {
public:
    explicit CameraHandler(Server* server) : _server(server), _currentValue(0) {
        setValue(1);
    }

    virtual void onConnect(WebSocket* connection) {
        _connections.insert(connection);
        connection->send(_currentSetValue.c_str());
        cout << "Connected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
        cout << "Credentials: " << *(connection->credentials()) << endl;
    }

    virtual void onData(WebSocket* connection, const char* data) {
        if (0 == strcmp("die", data)) {
            _server->terminate();
            return;
        }
        if (0 == strcmp("close", data)) {
            cout << "Closing.." << endl;
            connection->close();
            cout << "Closed." << endl;
            return;
        }

        /*int value = atoi(data) + 1;
        if (value > _currentValue) {
            setValue(value);
            for (auto c : _connections) {
                c->send(_currentSetValue.c_str());
            }
        }*/
    }
    virtual void onData(WebSocket*, const uint8_t* data, size_t size) {
      vector<char> jpgbytes(data, data+size);
      Mat img = imdecode(jpgbytes, 1);//Mat(480, 640, CV_8UC3, &data).clone();

      if(img.empty())
      {
          cout<<"image not loaded";
      }
      else
      {
          // cv::namedWindow( "test", CV_WINDOW_AUTOSIZE );
          cv::imshow("test",img);
          if( waitKey(1) == 27 ) exit(0);
      }
      // copies from Result.Buffer into img
      //memcpy(img.ptr(),data,480*640);
    }

    virtual void onDisconnect(WebSocket* connection) {
        _connections.erase(connection);
        cout << "Disconnected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
    }

private:
    set<WebSocket*> _connections;
    Server* _server;
    int _currentValue;
    string _currentSetValue;

    void setValue(int value) {
        _currentValue = value;
        _currentSetValue = makeExecString("set", _currentValue);
    }
};

int main(int /*argc*/, const char* /*argv*/[]) {
    auto logger = std::make_shared<PrintfLogger>(Logger::Level::DEBUG);

    Server server(logger);

    auto handler = std::make_shared<CameraHandler>(&server);
    server.addWebSocketHandler("/ws", handler);
    server.serve("/home/slumber/Repos/DeviceTracking/static", 9090);
    return 0;
}
