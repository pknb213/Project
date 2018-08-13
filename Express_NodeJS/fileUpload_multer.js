var express = require('express'), http = require('http'), path = require('path');
var bodyParser = require('body-parser'), static = require('serve-static'), errorHandler = require('errorhandler');
var cookieParser = require('cookie-parser'), expressSession = require('express-session');

var expressErrorHandler = require('express-error-handler');
var expressSession = require('express-session');
var multer = require('multer');
var fs = require('fs');
var cors = require('cors');

var app = express();

app.set('port', process.env.PORT || 3000);

app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use(static(path.join(__dirname, 'public')));
app.use('/uploads',static(path.join(__dirname, 'uploads')));
app.use(cookieParser());
app.use(expressSession({
  secret:'my key',
  resave:true,
  saveUninitialized:true
}));
app.use(cors());

var storage = multer.diskStorage({
  destination: function(req,file,callback){
    callback(null,'./Express_NodeJS/uploads');
  },
  filename:function(req,file,callback){
    callback(null,file.originalname + Date.now());
  }
});

var upload = multer({
  storage: storage,
  limits: {
    files: 10,
    fileSize: 1024 * 1024 * 1024
  }
});

var router = express.Router();

router.route('/process/product').get(function(req,res){
  console.log('/process/product 호출됨');

  if(req.session.user){
    res.redirect('/product.html');
  }else{
    res.redirect('/login2.html');
  }
});

router.route('/process/login').post(function(req,res){
  console.log('/process/login 호출됨');

  var paramId = req.body.id || req.query.id;
  var paramPassword = req.body.password || req.query.password;

  if(req.session.user){
    console.log('이미 로그인되어 상품페이지로 이동합니다');

    res.redirect('/product.html');
  }else{
    req.session.user = {
      id: paramId,
      name: '소녀시대',
      authorized:true
    };

    res.writeHead('200', {'Content-Type': 'text/html;charset=utf8'});
    res.write('<h1>로그인 성공</h1>');
    res.write('<div><p>Param Id : ' + paramId + '</p></div>');
    res.write('<div><p>Param Password : ' + paramPassword + '</p></div>');
    res.write("<br><br><a href='/process/product'>상품 페이지로 이동하기</a>");
    res.end();
  }
});

router.route('/process/logout').get(function(req,res){
  console.log('/process/logout 호출됨');

  if(req.session.user){
    console.log('로그아웃 합니다');

    req.session.destroy(function(err){
      if(err){throw err;}
      console.log('세션을 삭제하고 로그아웃 되었습니다');
      res.redirect('/login2.html');
    });
  }else{
    console.log('아직 로그인되어 있지 않습니다');
    res.redirect('/login2.html');
  }
});

router.route('/process/photo').post(upload.array('photo',1),function(req,res){
  console.log('/process/photo 호출됨');

  try{
    var files = req.files;

    console.dir('#===== 업로드된 첫번째 파일 정보 ====#');
    console.dir(req.files[0]);
    console.dir('#====#');

    var originalname ='',
      filename = '',
      mimetype = '',
      size = 0;

      if(Array.isArray(files)){
        console.log("배열에 들어있는 파일 갯수 : %d", files.length);

        for(var index = 0; index < files.length; index++){
          originalname = files[index].originalname;
          filename = files[index].filename;
          mimetype = files[index].mimetype;
          size = files[index].size;
        }
      }else{
        console.log("파일 갯수 : 1");

        originalname = files[index].originalname;
        filename = files[index].filename;
        mimetype = files[index].mimetype;
        size = files[index].size;
      }

        console.log('현재 파일 정보 :' + originalname + ', ' + filename + ', ' + mimetype + ', ' + size);

        res.writeHead('200', {'Content-Type':'text/html;charset=utf8'});
        res.write('<h3>파일 업로드 성공</h3>');
        res.write('<hr/>');
        res.write('<p>원본 파일 이름 : ' + originalname + '-> 저장 파일명 : ' + filename + '</p>');
        res.write('<p>MIME Type : ' + mimetype + '</p>');
        res.write('<p>파일 크기 : ' + size + '</p>');
        res.end();
  }catch(err){
    console.dir(err.stack);
  }
});

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
