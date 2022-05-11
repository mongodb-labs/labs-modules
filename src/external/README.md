# $external aggregation stage

### Installation

Please refer to the root README on installation instructions.

### Introduction

The $external stage provides a way for the aggregation pipeline to make calls to a web service. If a POST request is used, the documents from the previous stage is used in the payload of the request.

### Use Cases
- ML model inference
- Sending output of aggregation pipeline to another system
- Any complex computation on documents

### Change Stream Support
Change streams is also supported. Simply register an endpoint (see below) and use it in a change stream pipeline. More information in the examples folder.

Change stream documentation here: https://www.mongodb.com/docs/manual/changeStreams/

```sh
const pipeline = [
   {
      "$external":{
         "name":"get-fraud"
      }
   },
   {
      "$addFields":{
         "fullDocument.response":"$response"
      }
   },
   {
      "$project":{
         "response":0
      }
   }
];

const watchCursor = db.collection.watch(pipeline);

while (!watchCursor.isExhausted()){
  if (watchCursor.hasNext()){
     printjson(watchCursor.next());
  }
}
```

### Usage

To begin, an endpoint must be registered to the endpoint registry. The registry contains all the available endpoints that can be used in the $external stage.

```sh
db.adminCommand({
	"registerEndpoint": "get-users",
	"endpoint": "https://reqres.in/api/users",
	"method": "GET",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
})
```

If successful, you should see the following success message.
```sh
{ "insert" : true, "ok" : 1 }
```

Once an endpoint is registered, you are able to directly use it in the $external aggregation stage.
```sh
db.collection.aggregate([{$external: {name: "get-users"}}])
```

The response:
```sh
{ "_id" : ObjectId("6241776d5b952bd2318ed6fe"), "name" : "John Doe", "trips" : 250, "airline" : 5, "response" : { "page" : 1, "per_page" : 6, "total" : 12, "total_pages" : 2, "data" : [ { "id" : 1, "email" : "george.bluth@reqres.in", "first_name" : "George", "last_name" : "Bluth", "avatar" : "https://reqres.in/img/faces/1-image.jpg" }, { "id" : 2, "email" : "janet.weaver@reqres.in", "first_name" : "Janet", "last_name" : "Weaver", "avatar" : "https://reqres.in/img/faces/2-image.jpg" }, { "id" : 3, "email" : "emma.wong@reqres.in", "first_name" : "Emma", "last_name" : "Wong", "avatar" : "https://reqres.in/img/faces/3-image.jpg" }, { "id" : 4, "email" : "eve.holt@reqres.in", "first_name" : "Eve", "last_name" : "Holt", "avatar" : "https://reqres.in/img/faces/4-image.jpg" }, { "id" : 5, "email" : "charles.morris@reqres.in", "first_name" : "Charles", "last_name" : "Morris", "avatar" : "https://reqres.in/img/faces/5-image.jpg" }, { "id" : 6, "email" : "tracey.ramos@reqres.in", "first_name" : "Tracey", "last_name" : "Ramos", "avatar" : "https://reqres.in/img/faces/6-image.jpg" } ], "support" : { "url" : "https://reqres.in/#support-heading", "text" : "To keep ReqRes free, contributions towards server costs are appreciated!" } } }
```

### Reference
| Field            | Default  | Type          | Description                                                                                                                                          |
|------------------|----------|---------------|------------------------------------------------------------------------------------------------------------------------------------------------------|
| registerEndpoint | N/A      | string        | The name of the endpoint that is used in the registry.                                                                                               |
| endpoint         | N/A      | string        | The endpoint to send the request to.                                                                                                                 |
| method           | GET      | string        | The method to use for the request.<br><br>Available values:<br>GET, POST, PUT                                                                        |
| httpHeaders      | []       | array<string> | The headers to user in the request.<br><br>Example:<br>[<br>  "Content-Type: application/octet-stream",<br>  "Accept: application/octet-stream"<br>] |
| as               | response | string        | Where the response of the request is stored.                                                                                                         |
| requestEncoding  | JSON     | string        | The encoding to use for the request payload. Only JSON is supported at the moment.                                                                   |
| responseEncoding | JSON     | string        | The encoding to use for the response data. Only JSON is supported at the moment.                                                                     |

### Known Limitations
- This is a prototype to and longevity of the module is not guaranteed.
