#!/usr/bin/env nodejs
//https://openclassrooms.com/fr/courses/1056721-des-applications-ultra-rapides-avec-node-js/4348381-gerer-son-application-avec-pm2
//https://devotics.fr/votre-premiere-app-nodejs-express/
//https://www.domoticz.com/wiki/Domoticz_API/JSON_URL%27s
var port	      = process.env.NODE_PORT || 8081;
var hostname  	= process.env.NODE_HOSTNAME || '0.0.0.0';
var domoticzURL	= process.env.DOMOTICZ_HOSTNAME || 'http://domoticz:80';
var app	      	= require('express')();
var request     = require('request');
var bodyParser  = require('body-parser');
////////////////////////////////////////////////////////////////////////////////
// Interface displaying:

app.use(bodyParser.json())
.get('/', function (req, res) {
  //  res.sendFile(req.url, {root: require('path').join(__dirname, '/public')});
  console.log(Date() + ': connected (GET) to ' + req.url + '.');
  //var id = req.param('id');
  //console.log('ID: ' + id + '\n');
  res.send('Hello World!');
})

.post('/', function(req, res) {
  console.log(Date() + ': connected (POST) to ' + req.url + '.');
  var id     = req.body.id;
  var names  = req.body.names;
  var values = req.body.values;
  var msg    = req.body.msg;
  console.log('device-ID=' + id + '&' + 'sensor-IDs=' + names + '&' + 'sensor-values=' + values + '&' + 'msg=\"' + msg + '\"');

  request.get({
      url: domoticzURL+"/json.htm?type=command&param=getlightswitches"
    }, function(error, response, body){
      if (!error && response.statusCode == 200){
        var err=true;    //optimistic version

        // Recherche des IDs capteurs/Search for sensor IDs:
        var idx =new Array();
        var data=JSON.parse(response.body);
        for(var i=0; data.result[i]; i++)
          for(var j=0; names[j]; j++)
            if(data.result[i].Name === id+':'+names[j]){
              idx[j]=data.result[i].idx;
              break;
            }

        for(var i=0; idx[i]; i++){
          var s=domoticzURL+"/json.htm?type=command&param=udevice&idx=" + idx[i] + "&nvalue=" + ((values[i]==1 || values[i]=='true') ?"1" :"0") + "&svalue=" + ((values[i]==1 || values[i]=='true') ?"1" :"0");
          console.log(s + " (" + names[i] + ")");
          request.get({
//            url: domoticzURL+"/json.htm?type=command&param=switchlight&idx=" + idx[i] + "&switchcmd=" + ((values[i]==1 || values[i]=='true') ?"On" :"Off")
              url: s
            }, function(error, response, body){
              if (!error && response.statusCode == 200)
                err=false;
              else
                console.log('Get error ("' + names[i] + '") sensor)');
          });
        }
        res.status(err ?404 : 200).send(err ?'ERR' :'OK');
      }else
        console.log('Get error (cannot contact ' + domoticzURL + ')');
  });
})

.use(function(req, res, next) {
  res.setHeader('Content-Type', 'text/plain');
  res.status(404).send('Page not found!');
})

.listen(port, hostname, function() {
  console.log(Date() + ': domoticzGateway connected to ' + hostname + ":" + port);
});
////////////////////////////////////////////////////////////////////////////////
