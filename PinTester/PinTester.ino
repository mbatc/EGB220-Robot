
int currentPin = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Ready...");
}

void setPin(int num) {
  Serial.print("Activating Pin ");
  Serial.println(num);

  digitalWrite(currentPin, LOW);
  
  pinMode(num, OUTPUT);
  digitalWrite(num, HIGH);
  currentPin = num;
}

void nextPin() {
  setPin(currentPin + 1);
}

void prevPin() {
  setPin(currentPin - 1);
}

void loop() {
  
  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    if (Serial.peek() == 'n') {
      prevPin();
    }

    if (Serial.peek() == 'm') {
      nextPin();
    }
    
    Serial.read();
  }
}
