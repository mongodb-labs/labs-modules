use test-db

function createDocument() {
  let id = ''
  let genderOptions = ['MALE', 'FEMALE']
  let ownsCarOptions = [true, false]
  let ownsRealEstateOptions = [true, false]
  let eductionOptions = ["High School", "College", "University", "None"]
  let familyOptions = ["Married", "Single"]

  var characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
  var charactersLength = characters.length
  for ( var i = 0; i < 8; i++ ) {
    id += characters.charAt(Math.floor(Math.random() * charactersLength))
 }

 return {
    clientId: id,
    gender: genderOptions[Math.floor(Math.random() * genderOptions.length)],
    ownsCar: ownsCarOptions[Math.floor(Math.random() * ownsCarOptions.length)],
    ownsRealEstate: ownsRealEstateOptions[Math.floor(Math.random() * ownsRealEstateOptions.length)],
    numberOfChildren: Math.floor(Math.random() * (6)),
    education: eductionOptions[Math.floor(Math.random() * eductionOptions.length)],
    family: familyOptions[Math.floor(Math.random() * familyOptions.length)],
 };
}

function sleep(milliseconds) {
  var start = new Date().getTime();
  for (var i = 0; i < 1e7; i++) {
    if ((new Date().getTime() - start) > milliseconds){
      break;
    }
  }
}

function generate() {
  while(true) {
    // No setTimeout in Mongo JS interpreter
    sleep(1500);
    db.collection.insert(createDocument())
  }
}

generate()