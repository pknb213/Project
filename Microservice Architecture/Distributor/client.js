'use strict';

const net = require('net');

class topClient{
  topClient(){
    console.log("생성자입니다");
  }
}

var c1 = new topClient;
c1.topClient();
