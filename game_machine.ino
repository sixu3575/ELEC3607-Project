/*
MIT License

Copyright (c) [2018] [Siyang Xu]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

* The University of Sydney
* School of Electrical and Information Engineering
* ELEC3607
* Program for 3d accelerometer game machine (Pong)
* Authors: Cheng Zhang 460048837
        Siyang Xu   460277002
*
* 14th June 2018
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define  BALLX      0         // TC channel for ball horizontal
#define  BALLY      1         // TC channel for ball vertical
#define  BRACKET1   2         // TC channel for bracket_one
#define  BRACKET2   0         // TC channel for bracket_two
#define  BALLXID    ID_TC0    // Instance ID for ball horizontal
#define  BALLYID    ID_TC1    // Instance ID for ball vertical
#define  BR1ID      ID_TC2    // Instance ID for bracket_one
#define  BR2ID      ID_TC3    // Instance ID for bracket_two
#define  blueToothSerial Serial1 // Using Serial1 for Bluetooth
#define  RxD         6 // Bluetooth receive
#define  TxD         7 // Bluetooth transmit

typedef struct ball_t {
  int row;
  int col;
  double hvel;
  double vvel;
} ball_t;

typedef struct bracket_t {
  int pos;
  int row;
  int len;
  double vel;
} bracket_t;

// Allocating memory for the structs
ball_t* ball = (ball_t*)malloc(sizeof(int)*2+sizeof(double)*2);
bracket_t* bracket_one = (bracket_t*)malloc(sizeof(int)*3+sizeof(double));
bracket_t* bracket_two = (bracket_t*)malloc(sizeof(int)*3+sizeof(double));

int p1 = 0; // Player 1 score
int p2 = 0; // Player 2 score

// Sets the inital position and velocity for the ball
void initialise_ball(ball_t* ball) {
  ball->row = 9;
  ball->col = 5;
  ball->hvel = 3;
  ball->vvel = 3*pow(-1,p1+p2);
}

// Sets the intial position and velocity for the bracket
void initialise_bracket(bracket_t* bracket) {
  bracket->pos = 5;
  bracket->len = 3;
  bracket->vel = 0;
}

// Turn ball on on LED screen
void ball_on(ball_t* ball) {
  digitalWrite(47-ball->row, HIGH);
  digitalWrite(ball->col+20, LOW);
}

// Turn ball off on LED screen
void ball_off(ball_t* ball) {
  digitalWrite(47-ball->row, LOW);
  digitalWrite(ball->col+20, HIGH);
}

// Turn bracket on on LED screen
void bracket_on(bracket_t* bracket) {
  digitalWrite(bracket->pos+20, LOW);
  digitalWrite(bracket->pos+20-1, LOW);
  digitalWrite(bracket->pos+20+1, LOW);
  digitalWrite(47-bracket->row, HIGH);
}

// Turn bracket off on LED screen
void bracket_off(bracket_t* bracket) {
  digitalWrite(bracket->pos+20, HIGH);
  digitalWrite(bracket->pos+20-1, HIGH);
  digitalWrite(bracket->pos+20+1, HIGH);
  digitalWrite(47-bracket->row, LOW);
}

// When a player fails to catch the ball
void round_over() {
  ball_off(ball);
  initialise_ball(ball);
  bracket_off(bracket_one);
  initialise_bracket(bracket_one);
  bracket_off(bracket_two);
  initialise_bracket(bracket_two);
}

// When a player reaches 5 scores
// The LED screen gets turned off completely
void terminate() {
  for (int i=21;i<29;i++){
    digitalWrite(i,HIGH);
  }
  for (int i=31;i<47;i++){
    digitalWrite(i,LOW);
  }
}

void setup() {
  Serial.begin(9600);

  //Initialise the ball and the brackets
  initialise_ball(ball);
  initialise_bracket(bracket_one);
  initialise_bracket(bracket_two);
  bracket_one->row = 16;
  bracket_two->row = 1;

  // Set the pinmodes
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  for (int i=21;i<29;i++){
    pinMode(i,OUTPUT);
    digitalWrite(i,HIGH);
  }
  for (int i=31;i<47;i++){
    pinMode(i,OUTPUT);
    digitalWrite(i,LOW);
  }

  // Establish Bluetooth connection
  setupBlueToothConnection();
  delay(1000);

  // Set timer counters and handlers
  pmc_set_writeprotect(false);

  // Timer counter for ball horizontal
  pmc_enable_periph_clk(BALLXID);
  TC_Configure(TC0, BALLX, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  TC_SetRC(TC0, BALLX, 21000000);
  TC_Start(TC0, BALLX);
  TC0->TC_CHANNEL[BALLX].TC_IER=TC_IER_CPCS;
  TC0->TC_CHANNEL[BALLX].TC_IDR=~TC_IER_CPCS;
  NVIC_EnableIRQ(TC0_IRQn);

  // Timer counter for ball vertical
  pmc_enable_periph_clk(BALLYID);
  TC_Configure(TC0, BALLY, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  TC_SetRC(TC0, BALLY, 21000000);
  TC_Start(TC0, BALLY);
  TC0->TC_CHANNEL[BALLY].TC_IER=TC_IER_CPCS;
  TC0->TC_CHANNEL[BALLY].TC_IDR=~TC_IER_CPCS;
  NVIC_EnableIRQ(TC1_IRQn);

  // Timer counter for bracket one
  pmc_enable_periph_clk(BR1ID);
  TC_Configure(TC0, BRACKET1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  TC_SetRC(TC0, BRACKET1, 60000000);
  TC_Start(TC0, BRACKET1);
  TC0->TC_CHANNEL[BRACKET1].TC_IER=TC_IER_CPCS;
  TC0->TC_CHANNEL[BRACKET1].TC_IDR=~TC_IER_CPCS;
  NVIC_EnableIRQ(TC2_IRQn);

  // Timer counter for bracket two
  pmc_enable_periph_clk(BR2ID);
  TC_Configure(TC1, BRACKET2, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK1);
  TC_SetRC(TC1, BRACKET2, 60000000);
  TC_Start(TC1, BRACKET2);
  TC1->TC_CHANNEL[BRACKET2].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[BRACKET2].TC_IDR=~TC_IER_CPCS;
  NVIC_EnableIRQ(TC3_IRQn);

}

// Handler for ball horizontal
void TC0_Handler() {
  TC_GetStatus(TC0, 0);
  // When the ball hits the sides
  if (ball->col==8 || ball->col == 1) {
  ball->hvel=-ball->hvel;
  }

  // When the ball hits the ends
  if (ball->row == 2) {
    if (bracket_two->pos != 1 && bracket_two->pos != 8) {
      ball->hvel += bracket_two->vel;
    }
  } else if (ball->row == 15) {
    if (bracket_one->pos != 1 && bracket_one->pos != 8) {
      ball->hvel += bracket_one->vel;
    }
  }

  // Moves the ball one step forward along its horizontal direction
  ball_off(ball);
  ball->col += ball->hvel/abs(ball->hvel);

}

// Handler for ball vertical
void TC1_Handler() {
  TC_GetStatus(TC0, 1);
  // When the ball hits the ends
  if (ball->row == 2) {
    if (ball->col < bracket_two->pos-1 || ball->col > bracket_two->pos+1) {
      p1++; // Player one scores
      print_score();
      if (p1 == 5) { // Player one wins
        blueToothSerial.println("Player 1 Wins!");
        blueToothSerial.println();
        terminate();
      }
      round_over();
      return;
    }
    ball->vvel -= 1; // Absolute value of vertical velocity gets incremented
    ball->vvel=-ball->vvel; // Vertical velocity gets reversed
  } else if (ball->row == 15) {
     if (ball->col < bracket_one->pos-1 || ball->col > bracket_one->pos+1) {
      p2++; // Player two scores
      print_score();
      if (p2 == 5) {
        blueToothSerial.println("Player 2 Wins!");
        blueToothSerial.println();
        terminate();
      }
      round_over();
      return;
    }
    ball->vvel += 1; // Absolute value of vertical velocity gets incremented
    ball->vvel=-ball->vvel; // Vertical velocity gets reversed
  }
  // Moves the ball one step forward along its veritcal direction
  ball_off(ball);
  ball->row += ball->vvel/abs(ball->vvel);
}

// Handler for bracket one
void TC2_Handler() {
  TC_GetStatus(TC0, 2);
  // If the bracket has not reached the sides
  if (!((bracket_one->pos==7 && bracket_one->vel>0) || (bracket_one->pos==2&&bracket_one->vel<0))){
    bracket_off(bracket_one);
    if (bracket_one->vel <0.01 && bracket_one->vel>-0.01) { // If the velocity is 0
      return;
    }
    bracket_one-> pos += bracket_one->vel/abs(bracket_one->vel); // Move bracket one
  }
}

// Handler for bracket two
void TC3_Handler() {
  TC_GetStatus(TC1, 0);
  // If the bracket has not reached the sides
  if (!((bracket_two->pos==7 && bracket_two->vel>0) || (bracket_two->pos==2&&bracket_two->vel<0))){
    bracket_off(bracket_two);
    if (bracket_two->vel <0.01 && bracket_two->vel>-0.01) {// If the velocity is 0
      return;
    }
    bracket_two-> pos += bracket_two->vel/abs(bracket_two->vel); // Move bracket two
  }
}

void print_score() {
  blueToothSerial.println("Current score");
  blueToothSerial.print("Player 1: ");
  blueToothSerial.println(p1);
  blueToothSerial.print("Player 2: ");
  blueToothSerial.println(p2);
  blueToothSerial.println();
}

void loop() {
  if (p1 != 5 && p2 != 5) { // When the game is not over
  // Using persistence of vision to turn on the ball and brackets simultaneously
  bracket_on(bracket_one);
  delay(5);
  bracket_off(bracket_one);
  bracket_on(bracket_two);
  delay(5);
  bracket_off(bracket_two);
  ball_on(ball);
  delay(5);
  ball_off(ball);

  // Classify the reading into several velocity levels
  // For bracket one
  if (analogRead(A0)<=429) {
    if (bracket_one->vel !=-10) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = -10;
  } else if (analogRead(A0)<= 459 && analogRead(A0)>=430) {
    if (bracket_one->vel !=-5) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = -5;
  } else if (analogRead(A0)<= 499 && analogRead(A0)>=460){
    if (bracket_one->vel !=-2) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = -2;
  } else if (analogRead(A0)<= 510 && analogRead(A0)>=500) {
    if (bracket_one->vel !=0) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = 0;
  } else if (analogRead(A0)<= 550 && analogRead(A0)>=511){
    if (bracket_one->vel !=2) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = 2;
  } else if (analogRead(A0)<= 590 && analogRead(A0)>=551) {
    if (bracket_one->vel !=5) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = 5;
  } else if (analogRead(A0)>=591) {
    if (bracket_one->vel !=10) {
      TC_Start(TC0, BRACKET1);
    }
    bracket_one->vel = 10;
  }

  // For bracket two
  if (analogRead(A2)<=429) {
    if (bracket_two->vel !=-10) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = -10;
  } else if (analogRead(A2)<= 459 && analogRead(A2)>=430) {
    if (bracket_two->vel !=-5) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = -5;
  } else if (analogRead(A2)<= 499 && analogRead(A2)>=460){
    if (bracket_two->vel !=-2) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = -2;
  } else if (analogRead(A2)<= 510 && analogRead(A2)>=500) {
    if (bracket_two->vel !=0) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = 0;
  } else if (analogRead(A2)<= 550 && analogRead(A2)>=511){
    if (bracket_two->vel !=2) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = 2;
  } else if (analogRead(A2)<= 590 && analogRead(A2)>=551) {
    if (bracket_two->vel !=5) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = 5;
  } else if (analogRead(A2)>=591) {
    if (bracket_two->vel !=10) {
      TC_Start(TC1, BRACKET2);
    }
    bracket_two->vel = 10;
  }

  // Constantly adjusting RC values according to the velocity values
  TC_SetRC(TC0, BALLX, 84000000/abs(ball->hvel));
  TC_SetRC(TC0, BALLY, 84000000/abs(ball->vvel));
  if (bracket_one->vel <0.01 && bracket_one->vel>-0.01) {
    TC_SetRC(TC0, BRACKET1, 84000000);
  } else {
    TC_SetRC(TC0, BRACKET1, 84000000/abs(bracket_one->vel));
  }
  if (bracket_two->vel <0.01 && bracket_two->vel>-0.01) {
    TC_SetRC(TC1, BRACKET2, 84000000);
  } else {
    TC_SetRC(TC1, BRACKET2, 84000000/abs(bracket_two->vel));
  }

  return;
  }

  // Game is over
  p1 = 0;
  p2 = 0;
  delay(10000);

}

void setupBlueToothConnection()
{
    blueToothSerial.begin(38400);                           // Set BluetoothBee BaudRate to default baud rate 38400
    blueToothSerial.print("\r\n+STWMOD=0\r\n");             // set the bluetooth work in slave mode
    blueToothSerial.print("\r\n+STNA=SiliSili\r\n");        // set the bluetooth name as "SeeedBTSlave"
    blueToothSerial.print("\r\n+STOAUT=1\r\n");             // Permit Paired device to connect me
    blueToothSerial.print("\r\n+STAUTO=0\r\n");             // Auto-connection should be forbidden here
    delay(2000);                                            // This delay is required.
    blueToothSerial.print("\r\n+INQ=1\r\n");                // make the slave bluetooth inquirable
    Serial.println("The slave bluetooth is inquirable!");
    delay(2000);                                            // This delay is required.
    blueToothSerial.flush();
}
