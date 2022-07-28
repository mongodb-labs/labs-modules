use output

const streamName = 'streamG'

// Example stream messages
// { "author" : "dave", "score" : 80, "views" : 100 }
// { "author" : "dave", "score" : 85, "views" : 521 }
// { "author" : "ahn", "score" : 60, "views" : 1000 }
// { "author" : "li", "score" : 55, "views" : 5000 }
// { "author" : "annT", "score" : 60, "views" : 50 }
// { "author" : "li", "score" : 94, "views" : 999 }
// {  "author" : "ty", "score" : 95, "views" : 1000 }

const agg = [
    {
        $in: {
            connector: "kafka",
            name: "kafkaUserBehavior",
            connectionConfig: {
                bootstrapServer: "localhost:9092",
                topic: "json-sample",
                format: "json",  // or text
            }
        }
    },
    {$match: {author: "dave"}},
    {$addFields: {multipleScoreViews: {$multiply: ["$score", "$views"]}}},
    {$project: {author: 1, multipleScoreViews: 1}},
    {$merge: {into: {db: "output", coll: "test"}}}
];

db.createStream(streamName, agg)
