var Clay = require("pebble-clay");
var clayConfig = require("./config.json");
var clay = new Clay(clayConfig);

function sendMessage(messages) {
  if (messages.length === 0) return;
  message = messages.shift();
  Pebble.sendAppMessage(
    message,
    function () {
      sendMessage(messages);
    },
    function (e) {
      console.log("Error sending messages to Pebble");
    }
  );
}

function fetchFeed(feed_id) {
  var session = createSession();
  if (!session) {
    console.log("No session");
    return;
  }
  var token = session.accessJwt;
  var id_to_uri = JSON.parse(localStorage.getItem("pinned_ids_to_uri"));
  if (!id_to_uri) {
    id_to_uri = getPinnedFeeds(false);
  }
  var feed_uri = id_to_uri[feed_id];
  var settings = JSON.parse(localStorage.getItem("clay-settings"));
  var limit = 10;
  if (settings && settings.Limit) {
    limit = settings.Limit;
  }
  var xhr = new XMLHttpRequest();
  xhr.open(
    "GET",
    "https://bsky.social/xrpc/app.bsky.feed.getFeed?limit=" +
      limit +
      "&feed=" +
      feed_uri,
    false
  );
  xhr.setRequestHeader("Authorization", "Bearer " + token);
  xhr.setRequestHeader("Content-Type", "application/json");
  console.log("Fetching feed: " + feed_uri);
  xhr.send();
  var response = JSON.parse(xhr.responseText);
  var messages = [];
  response.feed.forEach(function (item) {
    var secondsSincePost = Math.floor(
      (Date.now() - new Date(item.post.record.createdAt)) / 1000
    );

    // Filter empty posts without any text (just a picture, for example)
    if (item.post.record.text == "") {
      return;
    }

    // Filter out replies:
    if (
      item.reply ||
      (item.post && item.post.record && item.post.record.reply)
    ) {
      // `reply` object exists => this is a reply
      return;
    }

    messages.push({
      MessageType: "posts",
      PostName: item.post.author.displayName,
      PostHandle: item.post.author.handle,
      PostText: item.post.record.text,
      PostTime: secondsSincePost,
    });
  });
  messages.unshift({ MessageType: "post-count", Count: messages.length });
  console.log("Got " + messages.length + " posts from feed");
  if (messages.length > 0) {
    sendMessage(messages);
  }
  console.log("Feed items sent to Pebble successfully!");
}

function fetchFeedGenerator(feed_uri, token) {
  var xhr = new XMLHttpRequest();
  xhr.open(
    "GET",
    "https://bsky.social/xrpc/app.bsky.feed.getFeedGenerator?feed=" + feed_uri,
    false
  );
  xhr.setRequestHeader("Authorization", "Bearer " + token);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.send();
  return JSON.parse(xhr.responseText);
}

function getPinnedFeeds(send_to_watch) {
  var session = createSession();
  if (!session) {
    console.log("No session");
    sendMessage([{ MessageType: "feed-count", Count: 0 }]);
    return;
  }
  var token = session.accessJwt;
  var xhr = new XMLHttpRequest();
  xhr.open(
    "GET",
    "https://bsky.social/xrpc/app.bsky.actor.getPreferences",
    false
  );
  xhr.setRequestHeader("Authorization", "Bearer " + token);
  xhr.setRequestHeader("Content-Type", "application/json");
  console.log("Fetching pinned feeds...");
  xhr.send();

  var response = JSON.parse(xhr.responseText);
  console.log("Got user preferences");
  var pinnedFeeds;
  for (var preference of response.preferences) {
    if (preference.$type === "app.bsky.actor.defs#savedFeedsPrefV2") {
      pinnedFeeds = preference.items.filter(function (item) {
        return item.pinned;
      });
      break;
    }
  }
  console.log("Got feeds: " + JSON.stringify(pinnedFeeds));

  var id_to_uri = {};
  var feedRes;
  var messages = [];
  var numFeeds = 0;
  for (var feed of pinnedFeeds) {
    var feed_uri = encodeURIComponent(feed.value);
    feedRes = fetchFeedGenerator(feed_uri, token);
    if (!feedRes.error) {
      id_to_uri[feed.id] = feed_uri;
      messages.push({
        MessageType: "feeds",
        FeedId: feed.id,
        FeedName: feedRes.view.displayName,
      });
      numFeeds++;
    }
  }
  messages.unshift({ MessageType: "feed-count", Count: numFeeds });
  if (send_to_watch) {
    sendMessage(messages);
  } else {
    return id_to_uri;
  }
  localStorage.setItem("pinned_ids_to_uri", JSON.stringify(id_to_uri));
}

function fetchTrendingFeeds() {
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
    "https://public.api.bsky.app/xrpc/app.bsky.unspecced.getTrendingTopics?limit=10"
  );
  xhr.send();
}

function fetchTrendingFeed(feed_id) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    var response = JSON.parse(xhr.responseText);
    var messages = [];
    response.feed.forEach(function (item) {
      var secondsSincePost = Math.floor(
        (Date.now() - new Date(item.post.record.createdAt)) / 1000
      );

      // Filter empty posts without any text (just a picture, for example)
      if (item.post.record.text == "") {
        return;
      }

      // Filter out replies:
      if (
        item.reply ||
        (item.post && item.post.record && item.post.record.reply)
      ) {
        // `reply` object exists => this is a reply
        return;
      }
      messages.push({
        MessageType: "posts",
        PostName: item.post.author.displayName,
        PostHandle: item.post.author.handle,
        PostText: item.post.record.text,
        PostTime: secondsSincePost,
      });
    });
    messages.unshift({
      MessageType: "post-count",
      Count: messages.length,
    });
    sendMessage(messages);
    console.log("Feed items sent to Pebble successfully!");
  };
  var feed_uri = encodeURIComponent(
    "at://did:plc:qrz3lhbyuxbeilrc6nekdqme/trending.bsky.app/" + feed_id
  );
  var settings = JSON.parse(localStorage.getItem("clay-settings"));
  var limit = 10;
  if (settings && settings.Limit) {
    limit = settings.Limit;
  }
  xhr.open(
    "GET",
    "https://public.api.bsky.app/xrpc/app.bsky.feed.getFeed?limit=" +
      limit +
      "&feed=" +
      feed_uri
  );
  xhr.send();
}

function createSession() {
  var session_url = "https://bsky.social/xrpc/com.atproto.server.createSession";
  var settings = JSON.parse(localStorage.getItem("clay-settings"));
  if (!settings) {
    console.log("No settings");
    return;
  }
  var handle = settings.UserHandle;
  var password = settings.Password;
  if (!handle || !password) {
    console.log("No handle or password");
    return;
  }
  var xhr = new XMLHttpRequest();
  xhr.open("POST", session_url, false);
  xhr.setRequestHeader("Content-Type", "application/json");
  xhr.send(
    JSON.stringify({
      identifier: handle,
      password: password,
    })
  );
  if (!xhr.responseText) {
    console.log("No session created");
    sendMessage([
      { MessageType: "session-error", Error: "Invalid credentials" },
    ]);
    return;
  }
  session = JSON.parse(xhr.responseText);
  localStorage.setItem("bsky_session", xhr.responseText);
  console.log("New session created");
  return session;
}

Pebble.addEventListener("ready", function () {
  console.log("PebbleKit JS ready!");
});

Pebble.addEventListener("appmessage", function (e) {
  console.log("Received " + e.payload.MessageType + " request");
  switch (e.payload.MessageType) {
    case "user-feeds":
      getPinnedFeeds(true);
      break;
    case "feed":
      fetchFeed(e.payload.FeedId);
      break;
    case "topic":
      fetchTrendingFeed(e.payload.TopicId);
      break;
    case "topics":
      fetchTrendingFeeds();
      break;
  }
});
