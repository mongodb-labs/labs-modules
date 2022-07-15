import pymongo as pm

mongo = pm.MongoClient()
db = mongo.get_database("stream_test")

print("Preparing data")

db.test_ob.drop()
db.test_ob.insert_many([{"k": x, "v": x % 2 == 0} for x in range(20)])


pipeline = [
    {"$project": {"_id": 0}},
    {
        "$simpSWindow": {"n": 5, "gap": 3},
    },
    {"$groups": {"_id": "$v", "res": {"$addToSet": "$k"}}},
]

for r in db.test_ob.aggregate(pipeline):
    print(f"{r = }")
