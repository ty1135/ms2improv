// PS/2 keyboard library
// https://github.com/ndusart/ps2-keyboard
#include "FidPS2Host.h"


// USB Host Midi library
// https://github.com/felis/USB_Host_Shield_2.0
#include <usbh_midi.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>


USB Usb;
USBH_MIDI  Midi(&Usb);


// PS/2 keyboard scancode mapping
// Domain: 24 keys on ms2 keboard
uint8_t kb_scanCode[24] = {
  0x1A, // z
  0x1B, // s
  0x22, // x
  0x23, // d
  0x21, // c
  0x2A, // v
  0x34, // g
  0x32, // b
  0x33, // h
  0x31, // n
  0x3B, // j
  0x3A, // m
  0x35, // y
  0x3D, // 7
  0x3C, // u
  0x3E, // 8
  0x43, // i
  0x44, // o
  0x45, // 0
  0x4D, // p
  0x4E, // -
  0x54, // [
  0x55, // =
  0x5B, // ] 
};

void shift(vector){
    scanCode = (vector < 0) ? 0x12: 0x59;
    fid_ps2h_write(scanCode);
    fid_ps2h_write(0xF0);
    fid_ps2h_write(scanCode);
}


// 1:1 mapping from 24 ms2 keys: midi notes they hold
// Initial value 0 says that corresponding key has been released
byte kb_note[24] = {0};


// Release a key when midi message(note off) received
void handleNoteOff(byte note){
  // Find possible candidates key
  int key_a = note % 12;
  int key_b = key_a + 12;
  // if key_a holds that note
  if (kb_note[key_a] == note){
    fid_ps2h_write(0xF0);
    fid_ps2h_write(kb_scanCode[key_a]);
    kb_note[key_a] = 0;
  }
  // if key_b holds that note
  if (kb_note[key_b] == note){
    fid_ps2h_write(0xF0);
    fid_ps2h_write(kb_scanCode[key_b]);
    kb_note[key_b] = 0;
  }
}



void handleNoteOn(byte note){
  int key_a = note % 12;
  int key_b = key_a + 12;
  // if key is avaiable ,value kb_note[key] will be 0
  // if both candidate holds keys, force to use low note
  // i.e lower notes take precedence to be replaced.
  int key = (kb_note[key_a] < kb_note[key_b])? key_a: key_b;

  static byte leftMostNote = 48;
  static byte rightMostNote = 71;
        
  // -2 indicates to shift left twice
  // +2 indicates to shift right twice
  int shift_vector = (note - (leftMostNote + key)) / 12;
        
  // Shift to correct postion
  while(shift_vector!=0){
      direction = (shift_vector<0)? -1: 1;
      shift(shift_vector);
      leftMostNote += 12*direction;
      rightMostNote += 12*direction;
      shift_vector += -1*direction
  }
    // Release if the key has been pressed
    if (kb_note[key]=!0){
        fid_ps2h_write(0xF0);
        fid_ps2h_write(kb_scanCode[key]);
        
    }
    // Press the key and update kb_note[24] table
    fid_ps2h_write(kb_scanCode[key]);
    kb_note[key] = note;
    }


// Poll USB MIDI Controler and send to serial MIDI
void MIDI_poll()
{
    uint8_t inBuf[3];
    uint8_t size;
    
    if ( (size = Midi.RecvData(inBuf)) > 0 ){
        // note on
        if(inBuf[0]==0x90){
            handleNoteOn(inBuf[1]);
        }
        // note off
        if(inBuf[0]==0x80){
            handleNoteOff(inBuf[1]);
        }
    }
}

 void setup()
 {
     Serial.begin(115200);
     
     fid_ps2h_init(4,2);
     
     //Workaround for non UHS2.0 Shield
     pinMode(7, OUTPUT);
     digitalWrite(7, HIGH);
     
     if (Usb.Init() == -1) {
         while (1); //halt
     }//if (Usb.Init() == -1...
     delay( 200 );
 }


void loop()
{
    Usb.Task();
    if ( Midi ) {
        MIDI_poll();
    }
}
