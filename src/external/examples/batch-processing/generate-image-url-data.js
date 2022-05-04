use test-batch-db

db.collection.drop();

function createDocument() {
  let premiumOptions = ["free", "premium"]
  let characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
  let charactersLength = characters.length
  let id = ''
  for ( var i = 0; i < 8; i++ ) {
    id += characters.charAt(Math.floor(Math.random() * charactersLength))
 }

 return {
    tier: premiumOptions[Math.floor(Math.random() * premiumOptions.length)],
    imageUrl: `http://www.example.com/${id}.png`
 };
}

function generate() {
  let documents = []
  for (let i = 0; i < 20; i++) {
    documents.push(createDocument())
  }

  db.collection.insertMany(documents)
}

generate()