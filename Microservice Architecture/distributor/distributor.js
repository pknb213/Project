'use strict'

/*
  distributor
*/
// 노드 접속 관리 오브젝트
var map = {};

// Server 클래스 상속
class distributor extends require('./server.js'){
  constructor(){
    // Server 클래스 생성자 호출
    super("distributor", 9000, ["POST/distributes", "GET/distributes"]);
  }

  onCreate(socket){   // 노드 접속 이벤트 처리
    console.log("onCreate", socket.remoteAddress, socket.remoteport);
    this.sendInfo(socket);
  }

  onClose(socket){    // 접속 해제 이벤트 처리
    var key = socket.remoteAddress + ":" + socket.remotePort;
    console.log("onClose", socket.remoteAddress, socket.remotePort);
    delete map[key];
    this.sendInfo();
  }

  onRead(socket, json){   // 노드 등록 처리, 데이터 수신
    var key = socket.remoteAddress + ":" + socket.remotePort; // 키 생성
    console.log("onRead", socket.remoteAddress, socket.remotePort, json);

    if(json.uri == "/distributes" && json.method == "POST"){ // 노드 정보 등록
      map[key] = {
        socket: socket
      };
      map[key].info = json.params;
      map[key].info.host = socket.remoteAddress;
      this.sendInfo();                                       //접속한 노드에 전파
    }
  }

  // 패킷 전송
  write(socket, packet){
    socket.write(JSON.stringify(packet) + "\n");
  }

  // 노드 접속 또는 특정 소켓에 노드 접속 정보 전파
  sendInfo(socket){
    var packet = {
      uri: "/distributes",
      method: "GET",
      key: 0,
      params: []
    };

    for(var n in map){
      packet.params.push(map[n].info);
    }

    if(socket){
      this.write(socket, packet)
    }else{
      for(var n in map){
        this.write(map[n].socket, packet);
      }
    }
  }
}

new distributor();
