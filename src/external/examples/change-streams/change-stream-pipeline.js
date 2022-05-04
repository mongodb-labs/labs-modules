use test-stream-db

// Testing basic GET request in change streams
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "get-fraud",
	"endpoint": "http://localhost:5001/mock_fraud",
	"method": "POST",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))

const pipeline = [
  { $external: {name: "get-fraud"}},
	{
		$addFields: {
			 "fullDocument.response": "$response"
		}
 	},
	{ $project: {"response": 0}},
];

const watchCursor = db.collection.watch(pipeline);

while (!watchCursor.isExhausted()){
  if (watchCursor.hasNext()){
     printjson(watchCursor.next());
  }
}