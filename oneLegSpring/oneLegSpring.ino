/* 1脚バネ */
/* RMDX8クラスを使用する */

#include <M5Stack.h>
#include <mcp_can_m5.h>
#include <SPI.h>
#include <RMDX8_M5.h>

/*SAMD core*/
#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
#define SERIAL SERIALUSB
#else
#define SERIAL Serial
#endif

#define BAUDRATE 115200 //シリアル通信がボトルネックにならないよう，速めに設定しておく
#define LOOPTIME 5     //[ms]
#define ENDTIME 10000   //[ms]
#define TEXTSIZE 2
#define KP 2.0
#define KD 0.01
#define TGT_POS 0

unsigned long timer[3];

const uint16_t MOTOR_ADDRESS0 = 0x141; //0x140 + ID(1~32)
const uint16_t MOTOR_ADDRESS1 = 0x142; //0x140 + ID(1~32)
const int SPI_CS_PIN = 12;

#define CAN0_INT 15          // Set INT to pin 2
MCP_CAN_M5 CAN0(SPI_CS_PIN); // Set CS to pin 10
RMDX8_M5 myMotor0(CAN0, MOTOR_ADDRESS0);    // shoulder
RMDX8_M5 myMotor1(CAN0, MOTOR_ADDRESS1);    // knee

double initial_pos[2], previous_pos[2], present_vel[2];
int32_t target_pos[2], target_cur[2];

void init_can();

void setup()
{
    M5.begin();
    M5.Power.begin();
    SERIAL.begin(BAUDRATE);
    delay(1000);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.setTextSize(TEXTSIZE);

    init_can();
    SERIAL.println("Test CAN...");
    timer[0] = millis();

    myMotor0.readPosition();
    initial_pos[0] = myMotor0.present_angle;
    myMotor1.readPosition();
    initial_pos[1] = myMotor1.present_angle;
}

void loop()
{
    while (millis() - timer[0] < ENDTIME)
    {
        timer[1] = millis();

        // read multi turn angle
        previous_pos[1] = myMotor1.present_angle;
        myMotor1.readPosition();

        // knee motor:      current control (spring)
        // shoulder motor:  position control (spring)

        present_vel[1] = (myMotor1.present_angle - previous_pos[1])*1000/LOOPTIME;
        target_cur[1] = - KP*(myMotor1.present_angle - initial_pos[1]) - KD * present_vel[1];
        target_pos[0] = (-0.5 * (myMotor1.present_angle - initial_pos[1]) + initial_pos[0]) * 600;

        myMotor0.writePosition(target_pos[0]);
        myMotor1.writeCurrent(target_cur[1]);

        M5.update();

        // vel = (myMotor2.present_pos - pos_buf)/(LOOPTIME*0.01);

        // Debug (M5モニタ)(この処理重いので注意)
        // M5.Lcd.setCursor(0, 40);
        // M5.Lcd.printf("TIM:            ");
        // M5.Lcd.setCursor(0, 40);
        // M5.Lcd.printf("TIM: %d", timer[1] - timer[0]);
        // M5.Lcd.setCursor(0, 70);
        // M5.Lcd.printf("POS:            ");
        // M5.Lcd.setCursor(0, 70);
        // M5.Lcd.printf("POS: %d", myMotor2.present_pos);
        // M5.Lcd.setCursor(0, 100);
        // M5.Lcd.printf("HRN:            ");
        // M5.Lcd.setCursor(0, 100);
        // M5.Lcd.printf("HRN: %d", myMotor2.present_angle);
        // M5.Lcd.setCursor(0, 130);
        // M5.Lcd.printf("VEL:            ");
        // M5.Lcd.setCursor(0, 130);
        // M5.Lcd.printf("VEL: %d", vel);

        // Debug (SERIAL)
        SERIAL.print("TIM: ");
        SERIAL.print(timer[1] - timer[0]);
        SERIAL.print(" TGT: ");
        SERIAL.print(initial_pos[1]);
        SERIAL.print(" POS: ");
        SERIAL.print(myMotor1.present_angle);
        SERIAL.print(" VEL: ");
        SERIAL.print(present_vel[1]);
        SERIAL.print(" CUR: ");
        SERIAL.print(target_cur[1]);
        SERIAL.println("");

        timer[2] = millis() - timer[1];
        if (timer[2] < LOOPTIME)
        {
            delay(LOOPTIME - timer[2]);
        }
        else
        {
            SERIAL.print("TIME SHORTAGE");
            SERIAL.println(LOOPTIME - timer[2]);
            // M5.Lcd.printf("TIME SHORTAGE: %d\n", LOOPTIME - timer[2]);
        }
    }
    // finishing
    myMotor0.writePosition(0);
    myMotor1.writePosition(0);
    // stop command
    myMotor0.stop();
    myMotor1.stop();
    delay(500);
    SERIAL.println("Program finish!");
    M5.Lcd.setCursor(0, 160);
    M5.Lcd.printf("Program finish!");
    while (true)
    {
        delay(100);
    }
}

void init_can()
{
    M5.Lcd.setTextSize(TEXTSIZE);
    M5.Lcd.setCursor(0, 10);
    M5.Lcd.printf("CAN Test!\n");

    // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
    if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK)
    {
        SERIAL.println("MCP2515 Initialized Successfully!");
    }
    else
    {
        SERIAL.println("Error Initializing MCP2515...");
    }

    CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmitted
}
