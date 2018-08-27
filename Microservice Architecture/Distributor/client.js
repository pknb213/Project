'use strict';

const net = require('net');

class topClient{
  constructor(host, port, onCreate, onRead, onEnd, onError){
      this.options = {
        host: host,
        port: port
      };
      this.onCreate = onCreate;
      this.onRead = onRead;
      this.onEnd = onEnd;
      this.onError = onError;
  }

  connect(){                                              // 접속 처리 함수
    this.client = net.connect(this.options,() => {
      if(this.onCreate)
      this.onCreate(this.options);                        // 접속 완료 이벤트 콜백
    });
    this.client.on('data', (data) =>{                     // 데이터 수신 처리
      var sz = this.merge? this.merge + data.toString() : data.toString();
      var arr = sz.split('/n');
      for(var n in arr){
        if(sz.charAt(sz.length -1) != 'q' && n ==arr.length - 1){
          this.merge = arr[n];
          break
        }else if(arr[n] == ""){
          break;
        }else{
          this.onRead(this.options, JSON.parse(arr[n]));
        }
      }
    });

    this.client.on('close',() =>{                          // 접속 종료 처리
      if(this.onEnd)
        this.onEnd(this.options);
    });
    this.client.on('error', (err) =>{                      // 에러 발생 처
      if(this.onError)
        this.onError(this.options,err);
    });

    write(packet){
      this.client.write(JSON.stringify(packet) + '/n');
    }
  }
}


module.export = topClient;
