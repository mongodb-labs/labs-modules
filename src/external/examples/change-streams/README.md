# Credit Card Application Fraud Detection Demo

### Prerequisite

Please refer to the root README on mongod installation instructions.

After this is down, please run the mock API Docker container:

```sh
docker run -d -p 5001:5001 steve18201/mock-ml-api
```

This should expose port 5001 to the /mock_fraud endpoint.

### Usage
The first step is to listen to the change stream for changes. To do this, run the below command.

```sh
mongo --authenticationDatabase admin -u admin -p pass < change-stream-pipeline.js
```

This JS file will register an endpoint and construct an aggregation pipeline for change streams.


```javascript
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "get-fraud",
	"endpoint": "http://localhost:5001/mock_fraud",
	"method": "GET",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))

// Use $external and then basic data reshaping
const pipeline = [
  { $external: {name: "get-fraud"}},
	{
		$addFields: {
			 "fullDocument.response": "$response"
		}
 	},
	{ $project: {"response": 0}},
];
```

After this is successful, we need to generate some data:
```sh
mongo --authenticationDatabase admin -u admin -p pass < generate-credit-card-application-data.js
```


### Result
![Alt text](./change-stream-example.gif?raw=true "change stream example")
