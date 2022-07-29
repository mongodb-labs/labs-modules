use output

const streamName = 'streamD'

const agg = [
    {
        $in: {
            connector: "kafka",
            name: "kafkaUserBehavior",
            connectionConfig: {
                bootstrapServer: "localhost:9092",
                topic: "quickstart",
                format: "json",  // or text
            }
        }
    },
    {$merge: {into: {db: "output", coll: "test"}}}
];

db.createStream(streamName, agg)
