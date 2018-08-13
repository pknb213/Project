var winston = require('winston');
var winstonDaily = require('winston-daily-rotate-file');
var moment = require('moment');

function timeStampFormat(){
  return moment().format('YYYY-MM-DD HH:mm:ss.SSS ZZ');
}

var logger = new (winston.Logger)({
  transports: [
    new (winstonDaily)({
      name: 'info-file',
      filename: './Atom_NodeJS/log/serer',

      // . . .

    })
  ]
  
});
