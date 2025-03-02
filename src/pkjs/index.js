function getTrendingFeeds() {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var response = JSON.parse(xhr.responseText);
    var trending = response.topics.map(function (topic) {
      return topic.topic;
    });
    console.log("Trending: " + JSON.stringify(trending));
    Pebble.sendAppMessage(
      { Count: trending.length, Topics: trending.join(",") },
      function () {
        console.log("Trending sent to Pebble successfully!");
      },
      function (e) {
        console.log("Error sending trending to Pebble");
      }
    );
  };
  xhr.open(
    "GET",
    "https://public.api.bsky.app/xrpc/app.bsky.unspecced.getTrendingTopics"
  );
  xhr.send();
}

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");
  getTrendingFeeds();
});

Pebble.addEventListener("appmessage", function (e) {
  console.log("AppMessage received!");
  getTrendingFeeds();
});
