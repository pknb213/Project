'use strict';

const net = require('net');

/*
  tcpClient 클래스
*/
class tcpClient{
  /*
    생성자
  */
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

  /*
    접속 함수
  */
  connect(){                                              // 접속 처리 함수
    this.client = net.connect(this.options,() => {
      if(this.onCreate)
      this.onCreate(this.options);                        // 접속 완료 이벤트 콜백
    });
    // 데이터 수신 처리
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
    // 접속 종료 처리
    this.client.on('close',() =>{                          // 접속 종료 처리
      if(this.onEnd)
        this.onEnd(this.options);
    });
    // 에러 처리
    this.client.on('error', (err) =>{                      // 에러 발생 처
      if(this.onError)
        this.onError(this.options,err);
    });
  }

  /*
    데이터 발신
  */
  write(packet){
    this.client.write(JSON.stringify(packet) + '/n');
  }
}


module.export = tcpClient;
