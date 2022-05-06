import random
from flask import Flask

# Sample Flask Project

app = Flask(__name__)

CLASSES = ["car", "horse", "cat", "sheep", "dog", "shoe", "cup", "glasses", "laptop", "cookie"]

@app.route('/mock_fraud', methods=['POST'])
def credit_card_fraud():
  return {
    "isFraud": bool(random.getrandbits(1))
  }

@app.route('/mock_image_classification', methods=['POST'])
def image_classification():
  return {
    "classes": random.sample(CLASSES, random.randrange(4))
  }

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5001, debug=True)