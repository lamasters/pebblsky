function sendMessage(messages) {
  if (messages.length === 0) return;
  message = messages.shift();
  Pebble.sendAppMessage(
    message,
    function () {
      sendMessage(messages);
    },
    function (e) {
      console.log("Error sending trending count to Pebble");
    }
  );
}

function getTrendingFeeds() {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var response = JSON.parse(xhr.responseText);

    var messages = [{ MessageType: "count", Count: response.topics.length }];
    response.topics.forEach(function (topic) {
      messages.push({
        MessageType: "topics",
        TopicName: topic.topic,
        TopicURL: topic.link,
      });
    });
    sendMessage(messages);
    console.log("Trending topics sent to Pebble successfully!");
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
