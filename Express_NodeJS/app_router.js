var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static');
var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));

var router = express.Router();

// Router 이용
router.route('/process/login').post(function(req,res){
  console.log('/process/login 처리 완료');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
  res.write('<h1>Express 서버에서 응답 결과</h1>');
  res.write('<div><p>Parameter id : ' + paramId + '</p></div>');
  res.write('<div><p>Parameter passwd : ' + paramPassword + '</p></div>');
  res.write("<br><br><a href='/login2.html'>로그인페이지로 돌아가기</a>");
res.end();
});

app.use('/',router);

/* Use 함수이용
app.use(function(req,res,next){
  console.log('1st 미들웨어에서 요청 처리');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  res.writeHead('200', {'Content-Type': 'text/html;charset=utf8'});
  res.write('<h1>Express 서버 응답 결과</h1>');
  res.write('<div><p>Parameter id : ' + paramId + '</p></div>');
  res.write('<div><p>Parameter passwd : ' + paramPassword + '</p></div>');
  res.end();
});
*/
http.createServer(app).listen(app.get('port'), function(){
  console.log('익스프레스 서버 시작 : ' + app.get('port'));
});
