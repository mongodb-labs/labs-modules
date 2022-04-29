// Basic tests - this is not actively maintained.
// The tests rely on 3rd party mock APIs that may break.

use test-db

db.collection.drop();

db.collection.insert({
	_id: 0,
	string: "Test Value",
	doubleVal: 819.536,
	int64Val: NumberLong(820),
	int32Val: NumberInt(820),
	dateVal: ISODate("2019-01-30T07:30:10.137Z")
});

// Testing basic GET request
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "get-users",
	"endpoint": "https://reqres.in/api/users",
	"method": "GET",
	"httpHeaders": [],
	"as": "response",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))

let result = db.collection.aggregate([{$external: {name: "get-users"}}]).toArray()[0]

assert.eq(result.hasOwnProperty("response"), true);
assert.gt(Object.keys(result.response).length, 0);

// Testing basic POST request
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "register-user",
	"endpoint": "https://reqres.in/api/register",
	"method": "POST",
	"httpHeaders": [],
	"as": "out",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))


let result2 = db.collection.aggregate([{$external: {name: "register-user"}}]).toArray()[0]

assert.eq(result2.hasOwnProperty("out"), true);
assert.gt(Object.keys(result2.out).length, 0);


// Use Fake API to test GET, POST, PUT w/ auth
// https://documenter.getpostman.com/view/4858910/S1LpZrgg

use fake-api-db

db.collection.drop();

db.collection.insert({
	"external_id": "postman"
});

// Create user to get auth token
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "fake-api-register-user",
	"endpoint": "https://node-fake-api-server.herokuapp.com/",
	"method": "POST",
	"httpHeaders": [
		"Content-Type: application/json",
		"X-FakeAPI-Action: register"
	],
	"as": "out",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))

let result3 = db.collection.aggregate([
	{
		$project: {
			_id: 0,
			external_id: 1
		}
	},
	{
		$external: {
			name: "fake-api-register-user"
		}
	}
]).toArray()[0]

assert.eq(result3.hasOwnProperty("out"), true);
assert.eq(result3.out.hasOwnProperty("auth_token"), true);

const authToken = result3.out.auth_token;

print("authToken", authToken)

// Use auth token in a request
assert.commandWorked(db.adminCommand({
	"registerEndpoint": "fake-api-register-hello-world",
	"endpoint": "https://node-fake-api-server.herokuapp.com/",
	"method": "POST",
	"httpHeaders": [
		"Content-Type: application/json",
		"X-FakeAPI-Action: record",
		`Authorization: Basic ${authToken}`
	],
	"as": "out",
	"requestEncoding": "JSON",
	"responseEncoding": "JSON"
}))


let result4 = db.collection.aggregate([{$external: {name: "fake-api-register-hello-world"}}]).toArray()[0]

assert.eq(result4.hasOwnProperty("out"), true);
assert.gt(Object.keys(result4.out).length, 0);