/*
  모듈 require 하기
*/
const http = require('http');
const url = require('url');
const querystring = require('querystring');

const members = require('./members.js');
const goods = require('./goods.js');
const purchases = require('./purchases.js');

/*
  HTTP 서버를 만들고 Request 처리
*/
var server = http.createServer((req,res) => {
  var method = req.method;              // 메시지 얻어 옴
  var uri = url.parse(req.url, true);   // Parsing (get, delete 경우)
  var pathname = uri.pathname;          // URI 얻어 옴
  console.log("hoho ajumma");

  if(method === "POST" || method === "PUT"){
    var body = "";

    req.on('data', function(data){
      body += data;
    });

    req.on('end', function(){
      var params;
      if(req.headers['content-tpye'] == "application/json"){
        params = JSON.parse(body);
      }else{
        params = querysting.parse(body);
      }
      onRequest(res,method,pathname,params);
    });
  }else{
    onRequest(res,method,pathname,uri.query);
  }

}).listen(8000);

/*
  Request에 대해 회원, 상품, 구매 관리 모듈별로 분기
  @param res      response object
  @param method   메서드
  @param pathname URI
  @param params   입력 파라미터
*/
function onRequest(res,method,pathname,params){

  switch(pathname){
    case "/members":
      members.onRequest(res,method,pathname,params,response);
      break;
    case "/goods":
      goods.onRequest(res,method,pathname,params,response);
      break;
    case "/purchases":
      purchases.onRequest(res,method,pathname,params,response);
      break;
    default:
      res.writeHead(404);
      return res.end();
  }
  res.end("response!");
}

/*
  HTTP 헤더에 JSON 형식으로 응답
  @param resave   response object
  @param packet   결과 파라미터
*/
function response(res, packet){
  res.writeHead(200, { 'Content-Type':'application/json'});
  res.end(JSON.stringify(packet));
}

console.log("app.js 가동 중 입니다.");
