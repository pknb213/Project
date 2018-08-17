// express 기본 모듈
var express = require("./config/config.js");
var http = require("./config/config.js");
var path = require("./config/config.js");
var bodyParser = require("./config/config.js");
var cookieParser = require("./config/config.js");
var staticFolder = require("./config/config.js");
var errorHandler = require("./config/config.js");
var expressErrorHandler = require("./config/config.js");

// mySQL
var mysql = require("./database/mysql.js");

// express object
var app = express();
// Session store
//var sessionStore = new mysqlSession({});  //###

// Initialization setting
app.set('port', process.env.PORT || 3000);
app.use(bodyParser.urlencoded({extended: false}));
app.use(bodyParser.json());
app.use('/public', staticFolder(path.join(__dirname,'public')));
app.use(cookieParser());
app.use(expressSession({
  secret: 'my key',
  resave: true,
  //store: sessionStore,
  saveUninitialized: true
}));

// Router object 참조
var router = express.Router();

// Router object 등록
app.use('/',router);

// 404 오류 처리
var errorHandler = expressErrorHandler({
  static: {
    '404': './public/404.html'
  }
});

app.use(expressErrorHandler.httpError(404));
app.use(errorHandler);

http.createServer(app).listen(app.get('port'), function(){
  console.log('서버가 가동 중 입니다. Port('+ app.get('port')+')');
});

console.log("app.js 동작 중");
