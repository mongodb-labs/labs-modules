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

wins = []
for r in db.test_ob.aggregate(pipeline):
    print(f"{r = }")
    # if r["type"] == "kAdvanced":
    #     if not wins:
    #         wins.append([])
    #     wins[-1].append(r["data"])
    # elif r["type"] == "kUnblock":
    #     win = list(map(lambda x: [_x["k"] for _x in x], wins))
    #     print(f"{win = }")
    # elif r["type"] == "kPop":
    #     wins.pop(0)
    # elif r["type"] == "kPartial":
    #     wins.append([])

print(f"{wins = }")
