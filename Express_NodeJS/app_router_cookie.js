var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static');
var cookieParser = require('cookie-parser');

var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));
app.use(cookieParser());

var router = express.Router();

router.route('/process/showCookie').get(function(req,res){
  console.log('/process/showCookie 호출됨');
  res.send(req.cookie);
});

router.route('/process/setUserCookie').get(function(req,res){
  console.log('/process/setUserCookie 호출됨');

  res.cookie('user', {
    id: 'ZYP',
    name: '조용필',
    authorized: true
  });

  res.redirect('/process/showCookie');
});
app.use('/',router);

http.createServer(app).listen(app.get('port'), function(){
  console.log('익스프레스 서버 시작 : ' + app.get('port'));
});
