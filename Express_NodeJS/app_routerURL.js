var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static');
var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));

var router = express.Router();

// Router 이용
router.route('/process/login/:name').post(function(req,res){
  console.log('/process/login/:name 처리 완료');

  var paramName = req.params.name;
  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
  res.write('<h1>Express 서버에서 응답 결과</h1>');
  res.write('<div><p>Parameter name : ' +paramName + '</p></div>');
  res.write('<div><p>Parameter id : ' + paramId + '</p></div>');
  res.write('<div><p>Parameter passwd : ' + paramPassword + '</p></div>');
  res.write("<br><br><a href='/login3.html'>로그인페이지로 돌아가기</a>");
res.end();
});

app.all('*',function(req,res){
  res.status(404).send('<h1>Error 페이지를 찾을 수 없습니다</h1>');
});

app.use('/',router);

http.createServer(app).listen(app.get('port'), function(){
  console.log('익스프레스 서버 시작 : ' + app.get('port'));
});
