var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static');
var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));

var router = express.Router();

router.route('/process/user/:id').get(function(req,res){
  console.log('/process/user/:id 처리 완료');

  var paramId = req.params.id;

  console.log('/process/users와 토큰 %s를 이용해 처리', paramId);

  res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
  res.write('<h1>Express 서버에서 응답 결과</h1>');
  res.write('<div><p>Parameter id : ' + paramId + '</p></div>');
  res.write("<br><br><a href='/login3.html'>로그인페이지로 돌아가기</a>");
res.end();
});

app.use('/',router);

http.createServer(app).listen(app.get('port'), function(){
  console.log('익스프레스 서버 시작 : ' + app.get('port'));
});
