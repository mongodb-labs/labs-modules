# Batch Image Classification Demo

### Prerequisite

Please refer to the root README on mongod installation instructions.

After this is done, please run the mock API Docker container:

```sh
docker run -d -p 5001:5001 steve18201/mock-ml-api
```

This should expose port 5001 and the /mock_image_classification endpoint.

NOTE: This is a mock API and does not have an actual image classication model serving it.

### Usage
The first step is generate fake image data. The data will include a fake image URL as well as a tier (free or premium) field.

```sh
mongo --authenticationDatabase admin -u admin -p pass < generate-image-url-data.js
```

Our aggregation pipelines first filters out free tier images and only runs image classification on premium images.


```javascript
const pipeline = [
  { $match : { tier : "premium" } },
  { $external: {name: "get-image-classification"}}
];

db.collection.aggregate(pipeline);
```

### Result

![Alt text](./batch-example.png?raw=true "batch example")
