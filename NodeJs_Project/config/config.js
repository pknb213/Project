// express 기본 모듈
var express = require("express");
var http = require("http");
var path = require("path");

var pool = require("../database/mysql.js");
//var pool = {};
// express 미들웨어
var bodyParser = require("body-parser");
var cookieParser = require("cookie-parser");
var staticFolder = require("serve-static");
var errorHandler = require("errorHandler");
var expressErrorHandler = require('express-error-handler');

var expressSession = require("express-session");
var mysqlSession = require("express-mysql-session")(expressSession);
module.exports = http;
module.exports = path;
module.exports = bodyParser;
module.exports = cookieParser;
module.exports = staticFolder;
module.exports = errorHandler;
module.exports = expressErrorHandler;
module.exports = expressSession;
console.log("config.js 동작 완료");
