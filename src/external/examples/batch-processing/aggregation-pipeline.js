use test-batch-db

assert.commandWorked(db.adminCommand({
	"registerEndpoint": "get-image-classification",
	"endpoint": "http://localhost:5001/mock_image_classification",
	"method": "POST",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))

const pipeline = [
  { $match : { tier : "premium" } },
  { $external: {name: "get-image-classification"}}
];

db.collection.aggregate(pipeline);