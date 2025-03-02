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
    var messages = [
      { MessageType: "topic-count", Count: response.topics.length },
    ];
    response.topics.forEach(function (topic) {
      var feed_parts = topic.link.split("/");
      var feed_id = feed_parts[feed_parts.length - 1];
      messages.push({
        MessageType: "topics",
        TopicName: topic.topic,
        TopicId: feed_id,
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

function getFeed(feed_id) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var response = JSON.parse(xhr.responseText);
    var messages = [{ MessageType: "post-count", Count: response.feed.length }];
    response.feed.forEach(function (item) {
      messages.push({
        MessageType: "posts",
        PostHandle: item.post.author.handle,
        PostText: item.post.record.text,
      });
    });
    sendMessage(messages);
    console.log("Feed items sent to Pebble successfully!");
  };
  var feed_uri = encodeURIComponent(
    "at://did:plc:qrz3lhbyuxbeilrc6nekdqme/trending.bsky.app/" + feed_id
  );
  xhr.open(
    "GET",
    "https://public.api.bsky.app/xrpc/app.bsky.feed.getFeed?limit=10&feed=" +
      feed_uri
  );
  xhr.send();
}

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");
  getTrendingFeeds();
});

Pebble.addEventListener("appmessage", function (e) {
  console.log("Received " + e.payload.MessageType + " request");
  if (e.payload.MessageType === "feed" && e.payload.TopicId) {
    getFeed(e.payload.TopicId);
  } else {
    getTrendingFeeds();
  }
});
