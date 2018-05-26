var express = require("express"),
    app = express();

var port = process.env.PORT || 8080;
var earthquake = 0;
app.use(express.static(__dirname + '/public'));

app.get("/check", function (request, response) {
  response.end(earthquake.toString());
});

app.get("/earthquake", function (request, response) {
  if (earthquake == 0){
    earthquake = 1;
  } else {
    earthquake = 0;
  }
    response.end("done");
});

app.listen(port);
console.log("Listening on port ", port);

require("cf-deployment-tracker-client").track();
