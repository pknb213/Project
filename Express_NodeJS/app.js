var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static');
var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));

// Middleware Test
app.use(function(req,res,next){
  console.log('1st Middleware 요청 처리');
  req.user = 'YoungJo';

 // JSON 데이터 보내기
 //res.send({name:'영조', age:27});

 // Redirect()
 //res.redirect('http://google.co.kr');

 var userAgent = req.header('User-Agent');
 var paramName = req.query.name;

 res.writeHead('200',{'Content-Type':'text/html; charset=utf8'});
 res.write('<h1>Express 서버에서 응답 결과</h1>');
 res.write('<div><p>User-Agent : ' + userAgent + '</p></div>');
 res.write('<div><p>Parameter Name : ' + paramName + '</p></div>');
 res.end();
// next();
});

/* 다중 Multiware
app.use('/',function(req,res,next){
  console.log('2st Middleware 요청 처리');
  res.writeHead('200',{'Content-Type':'text/html;charset=utf8'});
  res.end('<h1>Express 서버에서 '+ req.user +'님이 응답한 결과입니다</h1>');
});
*/
http.createServer(app).listen(app.get('port'), function(){
  console.log('익스프레스 서버 시작 : ' + app.get('port'));
});
